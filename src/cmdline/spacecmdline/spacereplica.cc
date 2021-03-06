//
// Created by yaowen on 6/11/19.
// 北航系统结构所-存储组
//

#include <future>
#include <iostream>
#include "gvds_struct.h"
//#include <pistache/client.h>
#include "cmdline/CmdLineProxy.h"

// TODO: 添加的新头文件
#include "client/ipc_struct.h"
#include "common/ipc/IPCClient.h"

using namespace gvds;
using namespace std;

/*
 * spacerename 命令行客户端
 */

int main(int argc, char *argv[]) {
  // TODO: 1.获取账户登录信息 2.检索区域信息 3. 提交空间重命名申请
  // ./spacerename_ipc --ip 192.168.5.222 -p 43107 --zonename syremotezone --id
  // 202 -o BIGBOSSSY -n BUAABUAA
  char *demo1[13] = {
      const_cast<char *>("spreplica"), const_cast<char *>("-p"),
      const_cast<char *>("9090"),      const_cast<char *>("--zonename"),
      const_cast<char *>("zonetest"),  const_cast<char *>("--id"),
      const_cast<char *>("127"),       const_cast<char *>("-o"),
      const_cast<char *>("spacetest"), const_cast<char *>("-n"),
      const_cast<char *>("spacetest2")};
  char *demo2[2] = {const_cast<char *>("spacerename"),
                    const_cast<char *>("--help")};

  // TODO: 提前准备的数据
  std::string replicated_id;
  std::string single_id;

  // TODO: 获取命令行信息
  CmdLineProxy commandline(argc, argv);
  std::string cmdname = argv[0];
  // TODO：设置当前命令行解析函数
  commandline.cmd_desc_func_map[cmdname] =
      [](std::shared_ptr<po::options_description> sp_cmdline_options) -> void {
    po::options_description command("空间重命名模块");
    command.add_options()("replicated,r", po::value<std::string>(),
                          "原副本空间")("single,s", po::value<std::string>(),
                                        "目标副本空间");
    sp_cmdline_options->add(command);  // 添加子模块命令行描述
  };
  // TODO： 解析命令行参数，进行赋值
  commandline.cmd_do_func_map[cmdname] =
      [&](std::shared_ptr<po::variables_map> sp_variables_map) -> void {
    if (sp_variables_map->count("replicated")) {
      replicated_id = (*sp_variables_map)["replicated"].as<std::string>();
    } else {
        cerr << "ERROR: Need replicated space ID!" << endl;
        commandline.print_options();
        exit(-1);
    }
    if (sp_variables_map->count("single")) {
      single_id = (*sp_variables_map)["single"].as<std::string>();
    } else {
        cerr << "ERROR: Need single space ID!" << endl;
        commandline.print_options();
        exit(-1);
    }
  };
  commandline.start();  //开始解析命令行参数

  // TODO :判断是否有参数，如果没有，则报错
  if (commandline.argc <= 1) {
    std::cerr << "Please input parameters！" << std::endl;
    commandline.print_options();
    exit(-1);
  }
  try {
    std::promise<bool> prom;
    auto fu = prom.get_future();
    // TODO:  调用IPC 客户端 进行同行，并获取返回结果
    IPCClient ipcClient("127.0.0.1", GVDS_CLIENT_LISTENING_PORT());
    ipcClient.set_callback_func([&](IPCMessage msg) -> void {
      // 客户端输出服务端发送来的消息
      //            char tmp[IPCMessage::max_body_length] = {0};
      //            std::memcpy(tmp, msg.body(), msg.body_length());
      std::string ipcresult(msg.body(), msg.body_length());
      if (ipcresult != "success") {
        // std::cerr << "执行失败，请检查命令参数是否正确！详情请查看日志！" <<
        // std::endl;
        std::cerr << ipcresult << std::endl;  // 执行结果
      } else {
        std::cout << ipcresult << std::endl;
      }
      prom.set_value(true);
    });
    ipcClient.run();  // 停止的时候调用stop 函数
    std::cout << "正在执行命令..." << std::endl;

    // TODO: 构造请求结构体，并发送；
    IPCreq ipcreq;
    ipcreq.cmdname = "spacereplica";
    ipcreq.spacename = replicated_id;
    ipcreq.newspacename = single_id;

    // TODO: 发送
    auto msg =
        IPCMessage::make_message_by_charstring(ipcreq.serialize().c_str());
    ipcClient.write(*msg);  // 传递一个消息；
    // sleep(1); // 等待客户端返回结果 等待1s；
    // fu.get(); // 阻塞等待，客户端命令返回；

    // TODO: 添加延迟，防止命令长时间等待
    auto status = fu.wait_for(std::chrono::seconds(20));
    if (status == std::future_status::timeout) {
      std::cout << "命令行执行20s，超时；请确认当前fuse client进程正在运行！"
                << std::endl;
      exit(-1);
    } else if (status == std::future_status::ready) {
      ipcClient.stop();
    } else {
      // std::future_status::deferred; future<int> result =
      // async(std::launch::deferred, myThread); 这种方式才会触发！ std::cout <<
      // "deferred!" << std::endl;
    }
  } catch (std::exception &e) {
    std::cout << e.what() << std::endl;
  }
  return 0;
}