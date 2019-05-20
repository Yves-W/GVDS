#pragma once

#include <udt.h>
#include <boost/asio.hpp>
#include <memory>
#include <rpc/msgpack.hpp>
#include <vector>
#include "msg/udt_writer.h"

namespace hvs {

class UDTServer;

class ServerSession {
 public:
  ServerSession(UDTServer *srv, UDTSOCKET socket);
  void close();
  void do_read();

 protected:
  std::shared_ptr<UDTWriter> writer;

 private:
  UDTServer *parent;
  clmdep_msgpack::unpacker unpacker;
  clmdep_msgpack::sbuffer output_buf;
  UDTSOCKET socket_;
  bool m_stop = false;
};
}  // namespace hvs
