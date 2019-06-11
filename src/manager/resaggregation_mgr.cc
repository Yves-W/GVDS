#include "manager/resaggregation_mgr.h"
#include "datastore/datastore.h"
#include "datastore/couchbase_helper.h"

using namespace hvs;
using namespace std;
using namespace Pistache::Rest;
using namespace Pistache::Http;

void ResAggregation_MGR::start() {
  auto _config = HvsContext::get_context()->_config;
  auto _bn = _config->get<std::string>("couchbase.bucket");
  bucket = _bn.value_or("test");  // use test bucket or predefined
}

void ResAggregation_MGR::stop() {}

void ResAggregation_MGR::router(Router& router) {

  Routes::Post(router, "/resource/register", Routes::bind(&ResAggregation_MGR::add, this));
  Routes::Get(router, "/resource/query", Routes::bind(&ResAggregation_MGR::list, this));
  Routes::Delete(router, "/resource/delete/:id", Routes::bind(&ResAggregation_MGR::del, this));
  Routes::Put(router, "/resource/update", Routes::bind(&ResAggregation_MGR::update, this));
}

bool ResAggregation_MGR::add(const Rest::Request& req, Http::ResponseWriter res) {
  std::shared_ptr<Datastore> dbPtr = DatastoreFactory::create_datastore(
      bucket, hvs::DatastoreType::couchbase, true);
  auto storage_res = parse_request(req);
  // broken request
  if (!storage_res) {
    res.send(Code::Bad_Request);
    return false;
  }
  auto cbd = static_cast<CouchbaseDatastore*>(dbPtr.get());
  auto err = cbd->insert(storage_res->key(), storage_res->json_value());
  usleep(100000); // may take 100ms to be effective
  res.send(Code::Accepted, storage_res->key());
  return true;
}

bool ResAggregation_MGR::list(const Rest::Request& req, Http::ResponseWriter res) {
  std::shared_ptr<Datastore> dbPtr = DatastoreFactory::create_datastore(
      bucket, hvs::DatastoreType::couchbase, true);
  auto cbd = static_cast<CouchbaseDatastore*>(dbPtr.get());
  char query[256];
  snprintf(query, 256,
           "select * from `%s` where SUBSTR(META().id,0,%d) == '%s' order by "
           "META().id",
           bucket.c_str(), StorageResource::prefix().length(),
           StorageResource::prefix().c_str());
  auto [iop_infos, err] = cbd->n1ql(string(query));
  if (!err) {
    res.send(Code::Ok, json_encode(*iop_infos));
  } else {
    res.send(Code::Service_Unavailable);
  }
}

bool ResAggregation_MGR::del(const Rest::Request& req, Http::ResponseWriter res) {
  std::shared_ptr<Datastore> dbPtr = DatastoreFactory::create_datastore(
      bucket, hvs::DatastoreType::couchbase, true);
  auto uuid = req.param(":id").as<std::string>();
  auto err = dbPtr->remove(uuid);
  if (!err)
    res.send(Code::Ok, uuid);
  else
    res.send(Code::Bad_Request, uuid);
  return true;
}

bool ResAggregation_MGR::update(const Rest::Request& req, Http::ResponseWriter res) 
{}

std::shared_ptr<StorageResource> ResAggregation_MGR::parse_request(
    const Rest::Request& req) {
  auto cnt = req.body();
  try {
    auto ion = make_shared<StorageResource>();
    ion->deserialize(cnt);
    return ion;
  } catch (Expectation& e) {
    return nullptr;
  }
}