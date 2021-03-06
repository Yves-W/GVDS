#include "msg/client_session.h"
#include "gvds_context.h"
#include "msg/udt_client.h"

using namespace gvds;
using namespace std;

ClientSession::ClientSession(UDTClient *srv, UDTSOCKET socket)
    : parent(srv), unpacker(), socket_(socket), m_stop(true) {
  unpacker.reserve_buffer(102400);
  seq_n = 0;
  writer = make_shared<UDTWriter>(socket_);
}

void ClientSession::start() {
  m_stop = false;
  writer->start();
  create("client session");
}

void *ClientSession::entry() { do_read(); }

void ClientSession::close() {
  m_stop = true;
  if (writer) {
    writer->close();
    writer.reset();
  }
  // should we close udt socket here ???
  //   write_strand_.post([this]() {
  //     socket_.close();
  //     parent_->close_session(shared_from_base<server_session>());
  //   });
}

int ClientSession::write(ioproxy_rpc_buffer &buffer) {
  if(writer->error_stat()) {
    return -ECONNRESET;
  }
  clmdep_msgpack::sbuffer data;
  buffer.id = seq_n++;
  clmdep_msgpack::pack(data, buffer);
  // move sem, zero copy
  // send the resp back
  promise<std::shared_ptr<ioproxy_rpc_buffer>> ready_promise;

  future<std::shared_ptr<ioproxy_rpc_buffer>> ready_future = ready_promise.get_future();
  session_lock.lock();
  futures[buffer.id] = move(ready_future);
  ready_promises[buffer.id] = move(ready_promise);
  session_lock.unlock();
  writer->write(std::move(data));
  return buffer.id;
}

std::shared_ptr<ioproxy_rpc_buffer> ClientSession::wait_op(int id) {
  session_lock.lock();
  auto it = futures.find(id);
  if (it == futures.end()) {
    session_lock.unlock();
    return nullptr;
  }
  shared_future<std::shared_ptr<ioproxy_rpc_buffer>> ft = it->second;
  futures.erase(id);
  session_lock.unlock();
  auto status = ft.wait_for(chrono::seconds(10));
  if(status != future_status::ready)
    return nullptr;
  else {
    shared_ptr<ioproxy_rpc_buffer> res = ft.get();
    return res;
  }
}

int ClientSession::block_on_op(int id) {
  // if id==-1, wait until last op finished
  if(id == -1)
    id = seq_n-1;
  session_lock.lock();
  auto it = futures.find(id);
  if (it == futures.end()) {
    session_lock.unlock();
    return 0;
  }
  shared_future<std::shared_ptr<ioproxy_rpc_buffer>> ft = it->second;
  session_lock.unlock();
  auto status = ft.wait_for(chrono::seconds(10));
  if(status != future_status::ready)
    return -ETIMEDOUT;
  else {
    return 0;
  }
}

// currently without asio, we use epoll.
// do_read would be called when epoll returned. session should copy buffers
// from UDT and return as quick as possible
void ClientSession::do_read() {
  constexpr std::size_t max_read_bytes = 102400;
  constexpr std::size_t default_buffer_size = max_read_bytes;

  while (!m_stop) {
    unsigned long rs = 0;
    //   UDT::getsockopt(socket_, 0, UDT_RCVDATA, &rcv_size, &var_size);
    //  dout(-1) << "packer cap: "<< unpacker.parsed_size() << dendl;
    if (UDT::ERROR ==
        (rs = UDT::recv(socket_, unpacker.buffer(), default_buffer_size, 0))) {
      dout(10) << "WARNING: recv error:"
               << UDT::getlasterror().getErrorMessage() << dendl;
      // maybe close session?
      m_stop = true;
      return;
    } else {
      // dout(-1) << "udt recved on client" << dendl;
      unpacker.buffer_consumed(rs);
    }

    // function should return immediately, async op instead.
    // assemble the data op call
    clmdep_msgpack::unpacked result;
    while (unpacker.next(result) && !m_stop) {
      auto msg = result.get();

      // use this zone handle to keep memory live time
      // remember to pass it
      //   auto z =
      //   std::shared_ptr<RPCLIB_MSGPACK::zone>(result.zone().release());
      // handle the message
      try {
        auto buf = msg.as<ioproxy_rpc_buffer>();
        lock_guard<mutex> lock(session_lock);
        auto it = ready_promises.find(buf.id);
        if (it == ready_promises.end()) {
          // error duplicated msg?
          continue;
        } else {
          promise<shared_ptr<ioproxy_rpc_buffer>> pm = move(it->second);
          ready_promises.erase(it);
          if(auto_handler.count(buf.id)) {
            auto ft = futures.find(buf.id);
            if(ft != futures.end())
              futures.erase(ft);
            auto callback = handlers.find(buf.id);
            if(callback != handlers.end()) {
              callback->second();
            }
            // TODO: Should we handle the write state inline? such as resend the error write?
            // usually, resend still cause errors. Error should be prevented in open or stat call.
          } else {
            pm.set_value(make_shared<ioproxy_rpc_buffer>(move(buf)));
          }
        }
      } catch (exception &e) {
        // msg corrupt
        // pass
      }
      // after work, such as close session
    }
    if (unpacker.buffer_capacity() < max_read_bytes) {
      unpacker.reserve_buffer(max_read_bytes);
    }
  }
  if (m_stop) {
    // i will code next week
  }
}
