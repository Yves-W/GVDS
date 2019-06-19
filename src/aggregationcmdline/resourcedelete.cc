//
// Created by weibing on 6/16/19.
// 北航系统结构所-存储组
//

#include <iostream>
#include "hvs_struct.h"
#include <future>
#include <pistache/client.h>
#include "cmdline/CmdLineProxy.h"

// TODO: 添加的新头文件
#include "client/ipc_struct.h"
#include "ipc/IPCClient.h"

using namespace hvs;

/*
 * resourcedelete 命令行客户端
 */

int main(int argc, char* argv[]){
    
    char* demo1[2] = {const_cast<char *>("resourcedelete"), const_cast<char *>("--ri")}; 
    char* demo2[2] = {const_cast<char *>("resourcedelete"), const_cast<char *>("--help")};
    std::string storage_src_id = "";            // 存储资源UUID

    // TODO: 获取命令行信息
    CmdLineProxy commandline(argc, argv);
    std::string cmdname = argv[0];
    commandline.cmd_desc_func_map[cmdname] =  [](std::shared_ptr<po::options_description> res_cmdline_options)->void {
    po::options_description command("资源删除模块");
    command.add_options()
    ("ri", po::value<std::string>(), "存储资源UUID");
    res_cmdline_options->add(command); // 添加子模块命令行描述
    };
    // TODO： 解析命令行参数，进行赋值
    commandline.cmd_do_func_map[cmdname] =  [&](std::shared_ptr<po::variables_map> res_variables_map)->void {
    if (res_variables_map->count("ri"))
    {
        storage_src_id = (*res_variables_map)["ri"].as<std::string>();
    }};
    commandline.start(); //开始解析命令行参数
    //TODO :判断是否有参数，如果没有，则报错
    if (commandline.argc <= 1) {
        std::cerr << "请输入命令参数！" << std::endl;
        commandline.print_options();
        exit(-1);
    }
    try{
        std::promise<bool> prom;
        auto fu = prom.get_future();
        // TODO:  调用IPC 客户端 进行同行，并获取返回结果
        IPCClient ipcClient("127.0.0.1", 6666);
        ipcClient.set_callback_func([&](IPCMessage msg)->void {
            // 客户端输出服务端发送来的消息
            std::string ipcresult (msg.body(), msg.body_length());
            if(ipcresult == "")std::cout << "delete fail" << std::endl;
            else std::cout << ipcresult << std::endl;
            prom.set_value(true);
        });
        ipcClient.run(); // 停止的时候调用stop 函数
        std::cout << "正在执行命令..." << std::endl;

        // TODO: 构造请求结构体，并发送；
        IPCreq ipcreq;
        ipcreq.cmdname = "resourcedelete";
        ipcreq.storage_src_id = storage_src_id;

        // TODO: 发送
        auto msg = IPCMessage::make_message_by_charstring(ipcreq.serialize().c_str());
        ipcClient.write(*msg); // 传递一个消息；
        fu.get();// TODO: 等待客户端返回结果
        ipcClient.stop();

    } catch (std::exception &e) {
        std::cout << e.what() << std::endl;
    }
    return 0;
}