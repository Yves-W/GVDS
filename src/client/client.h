#pragma once

#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <vector>
#include "gvds_context.h"

#include <pistache/http.h>
#include <pistache/router.h>
#include <boost/lockfree/spsc_queue.hpp>
#include <boost/thread/thread.hpp>
#include "common/Thread.h"
#include "msg/node.h"
#include "msg/op.h"
#include <unordered_map>

namespace gvds {
class ClientModule;
class ClientRpc;
class ClientGraph;
class ClientFuse;
class ClientZone;
class SelectNode;
class ClientUser;
class ClientBufferQueue;
class ClientReadAhead;
class ClientCache;

class Client : public Thread, public Node, public JsonSerializer {
 public:
  Client() : m_stop(false), Node(CLIENT_NODE) {
    // TODO: should read from config file
  }
  void start();
  void stop();
  void registe_module(std::shared_ptr<ClientModule> mod);
  void serialize_impl() override;
  ~Client() override { stop(); };

  inline std::shared_ptr<ClientModule> operator[](const std::string& mod_name) {
    auto mod = modules.find("");
    ASSERT(mod != modules.end(), "mod not registed");
    return mod->second;
  }

 private:
  void* entry() override;

 private:
  // thread saft variables
  bool m_stop;

  public:
  std::string get_manager();

public:
  std::unordered_map<std::string, std::shared_ptr<ClientModule>> modules;
  std::vector<std::shared_ptr<ClientModule>> uninit_modules;
  std::shared_ptr<ClientGraph> graph;
  std::shared_ptr<ClientFuse> fuse;
  std::shared_ptr<ClientRpc> rpc;
  std::shared_ptr<ClientZone> zone;
  std::shared_ptr<SelectNode> optNode;
  std::shared_ptr<ClientUser> user;
  std::shared_ptr<ClientBufferQueue> queue;
  std::shared_ptr<ClientReadAhead> readahead;
  std::shared_ptr<ClientCache> cache;
};

class ClientModule {
 public:
  std::string module_name;
  Client* client;
  bool isThread;

 protected:
  ClientModule(const char* name, Client* cli)
      : module_name(name), isThread(false), client(cli) {}
  // could involk in module starting stage
  virtual void start() {}
  // could involk in module destroying stage
  virtual void stop() {}
  // module doesn't have its own thread

  friend class Client;
};

extern gvds::Client* init_client();
extern void destroy_client(gvds::Client* client);
}  // namespace gvds