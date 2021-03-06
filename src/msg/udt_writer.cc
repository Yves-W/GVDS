#include "msg/udt_writer.h"
#include "gvds_context.h"
using namespace gvds;
using namespace std;

void UDTWriter::start() {
  m_max_write = 20;
  m_stop = false;
  create("udt session");
}

void *UDTWriter::entry() {
  pthread_mutex_lock(&m_queue_mutex);
  while (!m_stop) {
    if (!m_new.empty()) {
      pthread_mutex_unlock(&m_queue_mutex);
      do_write();
      pthread_mutex_lock(&m_queue_mutex);
      continue;
    }
    pthread_cond_wait(&m_cond_writer, &m_queue_mutex);
  }
  pthread_mutex_unlock(&m_queue_mutex);
  do_write();
  return NULL;
}

void UDTWriter::close() {
  if (!m_stop) {
    m_stop = true;
    pthread_mutex_lock(&m_queue_mutex);
    pthread_cond_signal(&m_cond_writer);
    pthread_cond_broadcast(&m_cond_session);
    pthread_mutex_unlock(&m_queue_mutex);
    join();
  }
}

void UDTWriter::write(clmdep_msgpack::sbuffer &&data) {
  pthread_mutex_lock(&m_queue_mutex);
  while (m_pending_write > m_max_write) {
    // buffer size exceed limit, wait on session's cond
    pthread_cond_wait(&m_cond_session, &m_queue_mutex);
  }
  m_new.push(std::move(data));
  ++m_pending_write;
  pthread_cond_signal(&m_cond_writer);
  pthread_mutex_unlock(&m_queue_mutex);
}

void UDTWriter::do_write() {
  pthread_mutex_lock(&m_writer_mutex);
  pthread_mutex_lock(&m_queue_mutex);
  std::queue<clmdep_msgpack::sbuffer> t;
  t.swap(m_new);
  // buffer consumed.
  pthread_cond_broadcast(&m_cond_session);
  pthread_mutex_unlock(&m_queue_mutex);
  should_close = _write_unsafe(&t);

  pthread_mutex_unlock(&m_writer_mutex);
}

bool UDTWriter::_write_unsafe(std::queue<clmdep_msgpack::sbuffer> *q) {
  if(should_close)
    return true;
  while (!q->empty()) {
    // zero copy with move semantic
    clmdep_msgpack::sbuffer data(std::move(q->front()));
    q->pop();
    --m_pending_write;
    {
      // send circle in case of the lack of udt buffer
      unsigned long sent = 0, total = data.size();
      while (sent < total) {
        long ss = UDT::send(socket_, data.data()+sent, data.size()-sent, 0);
        if (ss == UDT::ERROR) {
          dout(5) << "WARNING: send error occured: "
                  << UDT::getlasterror().getErrorMessage() << dendl;
          break;
        }
        sent += ss;
      }
      if (sent < total) {
        // error occured, close session
        return true;
      }
    }
  }
  return false;
}