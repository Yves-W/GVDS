#include "manager/ioproxy_mgr.h"
#include "datastore/couchbase_helper.h"
#include "datastore/datastore.h"
#include "manager/rpc_mod.h"

using namespace hvs;
using namespace std;
using namespace Pistache::Rest;
using namespace Pistache::Http;

void IOProxy_MGR::start() {
  auto _config = HvsContext::get_context()->_config;
  auto _bn = _config->get<std::string>("couchbase.bucket");
  bucket = _bn.value_or("test");  // use test bucket or predefined
  init_ioproxy_list();
  m_stop = false;
  create("ioproxy-mgr-module");
}

void IOProxy_MGR::stop() { m_stop = true; }

void* IOProxy_MGR::entry() {
  while (!m_stop) {
    std::this_thread::sleep_for(std::chrono::seconds(10));
    dout(15) << "ioproxy mgr check heart beat." << dendl;
    for (auto iop_iter : live_ioproxy) {
      auto iop = iop_iter.second;
      try {
        auto res = mgr->rpc->call(iop, "ioproxy_heartbeat");
        if (res) {
          int status = res->as<int>();
          iop->status = IOProxyNode::Running;
        } else {
          iop->status = IOProxyNode::Stopped;
        }
      } catch (exception& e) {
        dout(10) << "ERROR: IOProxy {" << iop->uuid << "} stopped work."
                 << dendl;
        iop->status = IOProxyNode::Stopped;
      }
    }
  }
}

void IOProxy_MGR::router(Router& router) {
  Routes::Get(router, "/ioproxy", Routes::bind(&IOProxy_MGR::list, this));
  Routes::Get(router, "/ioproxy/:id", Routes::bind(&IOProxy_MGR::list, this));
  Routes::Post(router, "/ioproxy", Routes::bind(&IOProxy_MGR::add, this));
  Routes::Delete(router, "/ioproxy/:id", Routes::bind(&IOProxy_MGR::del, this));
  Routes::Put(router, "/ioproxy/:id", Routes::bind(&IOProxy_MGR::update, this));
}

bool IOProxy_MGR::add(const Rest::Request& req, Http::ResponseWriter res) {
  auto iop = parse_request(req);
  // broken request, cannot assemble a ioproxy_node
  if (!iop) {
    res.send(Code::Bad_Request);
    return false;
  }
  // refresh live ioproxy list in memory
  iop->status = IOProxyNode::Running;
  live_ioproxy[iop->uuid] = iop;

  std::shared_ptr<Datastore> dbPtr = DatastoreFactory::create_datastore(
      bucket, hvs::DatastoreType::couchbase, true);
  auto cbd = static_cast<CouchbaseDatastore*>(dbPtr.get());
  auto err = cbd->upsert(IOProxyNode::prefix() + iop->uuid, iop->json_value());
  res.send(Code::Accepted, iop->uuid);
  return true;
}

bool IOProxy_MGR::list(const Rest::Request& req, Http::ResponseWriter res) {
    for(auto iop : live_ioproxy) {
      iop.second->json_value();
    }
    res.send(Code::Ok, json_encode(live_ioproxy));
    return true;
}

bool IOProxy_MGR::del(const Rest::Request& req, Http::ResponseWriter res) {
  std::shared_ptr<Datastore> dbPtr = DatastoreFactory::create_datastore(
      bucket, hvs::DatastoreType::couchbase, true);
  auto uuid = req.param(":id").as<std::string>();
  auto err = dbPtr->remove(IOProxyNode::prefix() + uuid);
  if (!err)
    res.send(Code::Ok, uuid);
  else
    res.send(Code::Bad_Request, uuid);
  live_ioproxy.erase(uuid);
  return true;
}

bool IOProxy_MGR::update(const Rest::Request& req, Http::ResponseWriter res) {}

std::shared_ptr<IOProxyNode> IOProxy_MGR::parse_request(
    const Rest::Request& req) {
  auto cnt = req.body();
  try {
    auto ion = make_shared<IOProxyNode>();
    ion->deserialize(cnt);
    return ion;
  } catch (Expectation& e) {
    return nullptr;
  }
}

void IOProxy_MGR::init_ioproxy_list() {
  std::map<std::string, shared_ptr<IOProxyNode>> live_ioproxy_tmp;
  std::shared_ptr<Datastore> dbPtr = DatastoreFactory::create_datastore(
      bucket, hvs::DatastoreType::couchbase, true);
  auto cbd = static_cast<CouchbaseDatastore*>(dbPtr.get());
  char query[256];
  snprintf(query, 256,
           "select uuid,data_port,ip,name,rpc_port from `%s` where SUBSTR(META().id,0,%d) == '%s' order by "
           "META().id",
           bucket.c_str(), IOProxyNode::prefix().length(),
           IOProxyNode::prefix().c_str());
  auto [iop_infos_p, err] = cbd->n1ql(string(query));
  for(auto info : *iop_infos_p) {
    try {
      auto ion = make_shared<IOProxyNode>();
      ion->deserialize(info);
      live_ioproxy_tmp[ion->uuid] = ion;
    } catch (Expectation& e) {
      continue;
    }
  }
  live_ioproxy.swap(live_ioproxy_tmp);
}
