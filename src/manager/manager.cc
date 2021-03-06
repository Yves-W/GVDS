#include "manager/manager.h"
#include "manager/ioproxy_mgr.h"
#include "manager/usermodel/UserModelServer.h"
#include "manager/mconf/mconf.h"
#include "manager/resaggregation_mgr.h"
#include "zone/ZoneServer.h"
#include "manager/rpc_mod.h"

using namespace gvds;
using namespace std;
using namespace Pistache::Rest;

bool Manager::start() {
  // init rest server
  auto _config = HvsContext::get_context()->_config;
  auto rest_port = _config->get<int>("manager.port");
  auto rest_thread = _config->get<int>("manager.thread_num");
  auto ip = _config->get<std::string>("ip");
  auto _uuid = _config->get<std::string>("manager.uid");
  auto cid = _config->get<std::string>("manager.cid");
  if (!_uuid.has_value()) {
    dout(-1) << "ERROR: Manager UUID not found! Please use linux command UUID "
                "to generate IO proxy's UUID and insert it into config file."
             << dendl;
    return false;
  }
  uuid = _uuid.value();
  if (!rest_port) {
    std::cerr << "restserver error: invalid port." << std::endl;
  }
  if (!rest_thread) {
    std::cerr << "restserver error: invalid thread num." << std::endl;
  }
  if (!ip) {
    std::cerr << "restserver warning: invalid ip, turning to use 0.0.0.0"
              << std::endl;
    ip = "0.0.0.0";
  }
  if (!cid) {
    std::cerr << "ERROR: cannot found center_id in config file."
              << std::endl;
    return false;
  }
  center_id = cid.value();

  Port port(*rest_port);
  Address addr("0.0.0.0", port);

  restserver = make_unique<RestServer>(addr);
  restserver->init(*rest_thread);  //[原始函数]
  route(restserver->router);
  // init modules
  for (auto mod : modules) {
    mod.second->mgr = this;
    mod.second->start();
    // set router in modules
    mod.second->router(restserver->router);
  }
  m_stop = false;
  restserver->start();
  create("Manager");
  return true;
}

void Manager::stop() {
  // stop rest server
  if (restserver) stop_rest(restserver.release());
  // stop modules
  for (auto mod : modules) {
    mod.second->stop();
  }
  m_stop = true;
}

void* Manager::entry() {
  // we may listen on unix socket after midterm, but currently manager thread do
  // nothing
  if (restserver) {
    restserver->join();
  }
}

void Manager::route(Router& router) {
  Routes::Get(router, "/manager", Routes::bind(&Manager::manager_info, this));
}

void Manager::serialize_impl() {
  std::vector<std::string> mod_name;
  for (auto mod : modules) {
    mod_name.push_back(mod.second->module_name);
  }
  std::string ip = addr.to_string();
  int port = restserver->getPort();
  put("ip", ip);
  put("rest_port", port);
  put("name", Node::name);
  put("modules", mod_name);
}

void Manager::manager_info(const Rest::Request& req, Http::ResponseWriter res) {
  res.send(Pistache::Http::Code::Ok, serialize());
}

void Manager::registe_module(std::shared_ptr<ManagerModule> mod) {
  modules[mod->module_name] = mod;
  mod->mgr = this;
}

std::shared_ptr<ManagerModule> Manager::get_module(
    const std::string& mod_name) {
  auto it = modules.find(mod_name);
  if (it != modules.end()) {
    return modules[mod_name];
  } else {
    return nullptr;
  }
}

namespace gvds {
gvds::Manager* init_manager() {
  auto _config = HvsContext::get_context()->_config;
  auto ip = _config->get<std::string>("ip");
  if (!ip) {
    std::cerr << "restserver warning: invalid ip, turning to use 0.0.0.0"
              << std::endl;
    ip = "0.0.0.0";
  }
  auto mgr = new Manager();
  mgr->addr.from_string(*ip);
  // registe modlues in manager node
  mgr->rpc = std::make_shared<ManagerRpc>("rpc");
  mgr->registe_module(mgr->rpc);
  mgr->registe_module(std::make_shared<IOProxy_MGR>("ioproxy manager"));
  mgr->registe_module(
      std::make_shared<ResAggregation_MGR>("resaggregation manager"));

  mgr->registe_module(std::make_shared<ZoneServer>());
  mgr->registe_module(std::make_shared<SpaceServer>());

  mgr->registe_module(std::make_shared<UserModelServer>());
  mgr->registe_module(std::make_shared<Mconf>());

  gvds::HvsContext::get_context()->node = mgr;
  if(mgr->start()) {
    // TODO: use condition or mutex to wait the init stage
    return mgr;
  }
  else {
    delete mgr;
    return nullptr;
  }
}

void destroy_manager(gvds::Manager* mgr) {
  mgr->stop();
  delete mgr;
}
}  // namespace gvds
