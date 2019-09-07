// 管理员 各个 账户池 使用量查询
//
// Created by sy on 09/06/19.
// 北航系统结构所-存储组
//

#include <iostream>
#include <future>
#include <pistache/client.h>
#include "cmdline/CmdLineProxy.h"

// TODO: 添加的新头文件
#include "client/clientuser/ClientUser_struct.h"
#include "ipc/IPCClient.h"
#include "client/ipc_struct.h"



using namespace Pistache;
using namespace hvs;
//bool GetZoneInfo(std::string ip, int port, std::string clientID);
/*
 * zonerename 命令行客户端
 */
//std::unordered_map<std::string, std::string> zonemap;


int main(int argc, char* argv[]){
    // TODO: 1.获取账户登录信息 2.检索区域信息 3. 提交空间重命名申请
    // 1、用户登录
    char* demo1[9] = {const_cast<char *>("adseepool")};
    char* demo2[2] = {const_cast<char *>("adseepool"), const_cast<char *>("--help")};

    // TODO: 提前准备的数据
    

    // TODO: 获取命令行信息
    // CmdLineProxy commandline(9, demo1);
     CmdLineProxy commandline(argc, argv);
//    CmdLineProxy commandline(2, demo2);
    //std::string cmdname = "userlogin_ipc";
    std::string cmdname = argv[0];


    // TODO：设置当前命令行解析函数
    commandline.cmd_desc_func_map[cmdname] =  [](std::shared_ptr<po::options_description> sp_cmdline_options)->void {
        po::options_description command("管理员查询各个账户池使用量情况");
        command.add_options()
                ;
        sp_cmdline_options->add(command); // 添加子模块命令行描述
    };
    // TODO： 解析命令行参数，进行赋值
    commandline.cmd_do_func_map[cmdname] =  [&](std::shared_ptr<po::variables_map> sp_variables_map)->void {
    };
    commandline.start(); //开始解析命令行参数

    //TODO :判断是否有参数，如果没有，则报错
    // if (commandline.argc <= 1) {
    //     std::cerr << "请输入命令参数！" << std::endl;
    //     commandline.print_options();
    //     exit(-1);
    // }
    
    try{
        std::promise<bool> prom;
        auto fu = prom.get_future();
        // TODO:  调用IPC 客户端 进行同行，并获取返回结果
        IPCClient ipcClient("127.0.0.1", 6666);
        ipcClient.set_callback_func([&](IPCMessage msg)->void {
            // 客户端输出服务端发送来的消息
            std::string ipcresult (msg.body(), msg.body_length());
            if(ipcresult == "1"){
                std::cout << "账户池信息查询失败" << std::endl;
            }
            else if(ipcresult == "33"){
                std::cout << "权限不足：无法查询账户池信息" << std::endl;
            }
            else{
                std::cout << ipcresult << std::endl;   //正常信息
            }
        
            prom.set_value(true);
        });
        ipcClient.run(); // 停止的时候调用stop 函数
        std::cout << "正在执行命令..." << std::endl;


         // TODO: 构造请求结构体，并发送；
        IPCreq ipcreq;
        ipcreq.cmdname = "adseepool";
       
      

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