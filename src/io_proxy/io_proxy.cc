#include "io_proxy/io_proxy.h"
#include "io_proxy/io_worker.h"

using namespace std;
using namespace hvs;

namespace hvs {
IOProxyWorker* IOProxy::_get_idle_worker() {
  IOProxyWorker* ret;
  // cause io process is fast, use spin lock here
  while(!idle_list.pop(ret));
  return ret;
}

bool IOProxy::add_idle_worker(IOProxyWorker* woker) {
  // should not wait, idle list max capcity > max number of idle worker
  while(!idle_list.push(woker));
};

// producer op
bool IOProxy::queue_op(std::shared_ptr<OP> op, bool block) {
  pthread_mutex_lock(&m_queue_mutex);
  m_queue_mutex_holder = pthread_self();

  // wait for dispatch to catch up
  if(op_waiting_line.size() > m_max_op && !block)
    return false;
  while (op_waiting_line.size() > m_max_op && block)
    pthread_cond_wait(&m_cond_ioproxy, &m_queue_mutex);

  op_waiting_line.push(op);
  op->op_queued = std::chrono::steady_clock::now();

  pthread_cond_signal(&m_cond_dispatcher);
  m_queue_mutex_holder = 0;
  pthread_mutex_unlock(&m_queue_mutex);
  return true;
}

void* IOProxy::entry() {
  pthread_mutex_lock(&m_queue_mutex);
  m_queue_mutex_holder = pthread_self();
  while (!m_stop) {
    if (!op_waiting_line.empty()) {
      m_queue_mutex_holder = 0;
      pthread_mutex_unlock(&m_queue_mutex);
      _dispatch();
      pthread_mutex_lock(&m_queue_mutex);
      m_queue_mutex_holder = pthread_self();
      continue;
    }
    pthread_cond_wait(&m_cond_dispatcher, &m_queue_mutex);
  }
  m_queue_mutex_holder = 0;
  pthread_mutex_unlock(&m_queue_mutex);
}

// consumer op
void IOProxy::_dispatch() {
  pthread_mutex_lock(&m_dispatch_mutex);
  m_dispatcher_mutex_holder = pthread_self();

  pthread_mutex_lock(&m_queue_mutex);
  m_queue_mutex_holder = pthread_self();
  std::queue<std::shared_ptr<OP>> t;
  t.swap(op_waiting_line);
  pthread_cond_broadcast(&m_cond_ioproxy);
  m_queue_mutex_holder = 0;
  pthread_mutex_unlock(&m_queue_mutex);
  // do dispatch
  _dispatch_unsafe(&t);

  m_dispatcher_mutex_holder = 0;
  pthread_mutex_unlock(&m_dispatch_mutex);
}

void IOProxy::_dispatch_unsafe(std::queue<std::shared_ptr<OP>>* t) {
  std::shared_ptr<OP> op;
  while(!t->empty()) {
    op = t->front();
    t->pop();
    assert(op.get()); // op ptr should not be empty
    auto worker = _get_idle_worker(); // may wait on spin lock
    boost::intrusive_ptr<OpQueued> opq = new OpQueued(op);
    worker->my_scheduler().queue_event(worker->my_handle(), opq);
  }
}

void IOProxy::start() {
  pthread_mutex_lock(&m_queue_mutex);
  m_stop = false;
  // TODO: read configs
  int scher_num = 5;
  for(int i = 0; i < scher_num; i++) {
    auto scher = make_shared<IOProxy_scheduler>(true);
    schedulers.push_back(scher);
    for(int j = 0; j < m_max_worker/scher_num; j++) {
      auto worker = scher->create_processor<IOProxyWorker>();
      scher->initiate_processor(worker);
    }
    boost::thread* t = new boost::thread(boost::bind(&IOProxy_scheduler::operator(), scher.get(), 0));
    worker_threads.push_back(t);
  }
  pthread_mutex_unlock(&m_queue_mutex);
  create("io_proxy");
}

void IOProxy::stop() {
  if (is_started()) {
    pthread_mutex_lock(&m_queue_mutex);
    m_stop = true;
    pthread_cond_signal(&m_cond_dispatcher);
    pthread_cond_broadcast(&m_cond_ioproxy);
    pthread_mutex_unlock(&m_queue_mutex);
    join();
  }
}

hvs::IOProxy* init_ioproxy() {
  hvs::IOProxy *ioproxy = new hvs::IOProxy;
  hvs::HvsContext::get_context()->node = ioproxy;
  ioproxy->start();
  return ioproxy;
}
void destroy_ioproxy(hvs::IOProxy* ioproxy) {
  ioproxy->stop();
//  delete ioproxy;
}
}