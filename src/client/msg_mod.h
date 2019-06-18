#pragma once

#include <pistache/client.h>
#include <cerrno>
#include <shared_mutex>
#include <unordered_map>
#include "client.h"
#include "common/Thread.h"
#include "hvs_struct.h"
#include "msg/node.h"
#include "msg/udt_client.h"

namespace hvs {
class ClientRpc : public ClientModule {
 private:
  virtual void start() override;
  virtual void stop() override;

 private:
  UDTClient udt_client;
  std::shared_mutex rpc_mutex;
  std::unordered_map<std::string, std::shared_ptr<RpcClient>> rpc_clients;
  std::unordered_map<std::string, std::shared_ptr<ClientSession>> udt_clients;
  std::unordered_map<std::string, std::shared_ptr<Pistache::Http::Client>> rest_clients;
  std::shared_ptr<RpcClient> rpc_channel(std::shared_ptr<IOProxyNode> node);
  std::shared_ptr<ClientSession> udt_channel(std::shared_ptr<IOProxyNode> node);
  std::shared_ptr<Pistache::Http::Client> rest_channel(std::string endpoint);
  std::unordered_map<std::string, Pistache::Http::CookieJar> rest_cookies;

 public:
  ClientRpc(const char* name, Client* cli) : ClientModule(name, cli) {
    isThread = true;
  }

  template <typename... Args>
  std::shared_ptr<RPCLIB_MSGPACK::object_handle> call(
      std::shared_ptr<IOProxyNode> node, std::string const& func_name,
      Args... args);

  int write_data(std::shared_ptr<IOProxyNode> node, ioproxy_rpc_buffer& buf);
  std::unique_ptr<ioproxy_rpc_buffer> read_data(
      std::shared_ptr<IOProxyNode> node, ioproxy_rpc_buffer& buf);
  // WARNNING: the return result may be empty if request failed
  std::string post_request(const std::string& endpoint, const std::string& url,
                           const std::string& data = "");
  std::string get_request(const std::string& endpoint, const std::string& url);
  std::string delete_request(const std::string& endpoint, const std::string& url);

  friend class Client;
};

template <typename... Args>
std::shared_ptr<RPCLIB_MSGPACK::object_handle> ClientRpc::call(
    std::shared_ptr<IOProxyNode> node, std::string const& func_name,
    Args... args) {
  // TODO: We assume RpcClient can concurently call
  auto rpcc = rpc_channel(node);
  auto res = rpcc->call(func_name, args...);
  if (!res) {
    return nullptr;
  } else {
    return std::make_shared<RPCLIB_MSGPACK::object_handle>(std::move(*res));
    ;
  }
}
}  // namespace hvs