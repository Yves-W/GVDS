#pragma once

#define FUSE_USE_VERSION 31
#include <fuse3/fuse.h>
#include "client.h"
#include "common/Thread.h"
#include "gvds_struct.h"

namespace gvds {
struct ClientFuseData {
  Client* client;
  ClientFuse* fuse_client;
};

class ClientFuse : public ClientModule {
 private:
  virtual void start() override;
  void run();
  virtual void stop() override;

 private:
  char mountpoint[256];
  int fuse_argc;
  char* fuse_argv[16];
  char workers_argv[32];
  ClientFuseData fs_priv;

 public:
  bool use_udt;
  bool async_mode;
  int readahead;

 public:
  ClientFuse(const char* name, Client* cli) : ClientModule(name, cli) {
    isThread = true;
    fuse_argc = 0;
    use_udt = false;
    readahead = 0;
  }
  friend class Client;
};

extern struct fuse_operations gvdsfs_oper;
}  // namespace gvds