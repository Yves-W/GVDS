//
// Created by lbq on 5/30/19.
// 北航系统结构所-存储组
//

#include <iostream>
#include <future>
#include <pistache/client.h>
#include "cmdline/CmdLineProxy.h"

// TODO: 添加的新头文件
#include "client/clientuser/ClientUser_struct.h"
#include "common/ipc/IPCClient.h"
#include "client/ipc_struct.h"



using namespace Pistache;
using namespace gvds;
//bool GetZoneInfo(std::string ip, int port, std::string clientID);
/*
 * zonerename 命令行客户端
 */
//std::unordered_map<std::string, std::string> zonemap;


int main(int argc, char* argv[]){
    // TODO: 1.获取账户登录信息 2.检索区域信息 3. 提交空间重命名申请
    // 1、用户登录
    char* demo1[9] = {const_cast<char *>("deletecenter"),
                       const_cast<char *>("--centerID"), const_cast<char *>("6")};
    char* demo2[2] = {const_cast<char *>("deletecenter"), const_cast<char *>("--help")};

    // TODO: 提前准备的数据
    std::string centerID;
  


    // TODO: 获取命令行信息
    // CmdLineProxy commandline(9, demo1);
     CmdLineProxy commandline(argc, argv);
//    CmdLineProxy commandline(2, demo2);
    //std::string cmdname = "userlogin_ipc";
    std::string cmdname = argv[0];


    // TODO：设置当前命令行解析函数
    commandline.cmd_desc_func_map[cmdname] =  [](std::shared_ptr<po::options_description> sp_cmdline_options)->void {
        po::options_description command("删除超算信息");
        command.add_options()
                ("centerID", po::value<std::string>(), "超算ID")
                ;
        sp_cmdline_options->add(command); // 添加子模块命令行描述
    };
    // TODO： 解析命令行参数，进行赋值
    commandline.cmd_do_func_map[cmdname] =  [&](std::shared_ptr<po::variables_map> sp_variables_map)->void {
        if (sp_variables_map->count("centerID"))
        {
            centerID = (*sp_variables_map)["centerID"].as<std::string>();
        }
    };
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
        IPCClient ipcClient("127.0.0.1", GVDS_CLIENT_LISTENING_PORT());
        ipcClient.set_callback_func([&](IPCMessage msg)->void {
            // 客户端输出服务端发送来的消息
            std::string ipcresult (msg.body(), msg.body_length());
            if(ipcresult == "fail"){
                std::cout << "delete fail" << std::endl;
            }
            else{  //"0"
                std::cout << "delete success" << std::endl;
            }
            prom.set_value(true);
        });
        ipcClient.run(); // 停止的时候调用stop 函数
        std::cout << "正在执行命令..." << std::endl;


         // TODO: 构造请求结构体，并发送；
        IPCreq ipcreq;
        ipcreq.cmdname = "deletecenter";
       
        std::cout << "centerID: "<< centerID << std::endl;
        ipcreq.centerID = centerID; //账户名
        

        // TODO: 发送
        auto msg = IPCMessage::make_message_by_charstring(ipcreq.serialize().c_str());
        ipcClient.write(*msg); // 传递一个消息；
        fu.get();              // 等待客户端返回结果
        // sleep(1); // TODO: 等待客户端返回结果
        ipcClient.stop();

    } catch (std::exception &e) {
        std::cout << e.what() << std::endl;
    }
}