#include "io_proxy/proxy_op.h"
#include "context.h"
#include "msg/stat.h"

using namespace hvs;

namespace hvs {

void prepare_op(std::shared_ptr<OP> op) {
  if (!op->should_prepare) return;
  switch (op->type) {
    case IO_PROXY_METADATA: {
      static_cast<IOProxyMetadataOP*>(op.get())->buf = (struct stat *)malloc(sizeof(struct stat));
      break;
    }
    case IO_PROXY_DATA: {
      IOProxyDataOP* dataop = static_cast<IOProxyDataOP*>(op.get());
      dataop->obuf = (char *)malloc(dataop->size);
      break;
    }
    default:
      break;
  }
}

// TODO: this is an sync example. we should transfer it to aync
void do_op(std::shared_ptr<OP> op, boost::function0<void> callback) {
  switch (op->type) {
    case IO_PROXY_METADATA: {
      IOProxyMetadataOP* metaop = static_cast<IOProxyMetadataOP*>(op.get());
      ioproxy_do_metadata_op(metaop);
      break;
    }
    case IO_PROXY_DATA: {
      IOProxyDataOP* dataop = static_cast<IOProxyDataOP*>(op.get());
      ioproxy_do_data_op(dataop);
      break;
    }
    default:
      break;
  }
  callback();
}

void async_do_op(std::shared_ptr<OP> op, boost::function0<void> callback) {
  // not implemented yet!
  callback();
}

int ioproxy_do_metadata_op(IOProxyMetadataOP* op) {
  //  dout(5) << "async do op: " << op->id << dendl;
  op->error_code = stat(op->path, op->buf);
  return op->error_code;
}

int ioproxy_do_data_op(IOProxyDataOP* op) {
  //  dout(5) << "async do op: " << op->id << dendl;
  // op->stat_buf = get_stat(op->path);
  switch (op->operation) {
    case IOProxyDataOP::read: {
      auto fd = open(op->path, O_RDONLY);
      if (fd == -1) {
        op->error_code = errno;
        break;
      }
      int n = read(fd, op->obuf, op->size);
      if (n == -1) {
        op->error_code = n;
      } else {
        op->size = n;
      }
      close(fd);
      break;
    }
    case IOProxyDataOP::write: {
      auto fd = open(op->path, O_RDWR);
      if (fd == -1) {
        op->error_code = errno;
        break;
      }
      int n = write(fd, op->ibuf, op->size);
      if (n == -1) {
        op->error_code = n;
      } else {
        op->size = n;
      }
      close(fd);
      break;
    }

    default:
      break;
  }
  return op->error_code;
}
}  // namespace hvs