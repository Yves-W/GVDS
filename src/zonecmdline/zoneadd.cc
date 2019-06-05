//
// Created by sy on 5/30/19.
// 北航系统结构所-存储组
//

#include <iostream>
#include "manager/space/Space.h"
#include "manager/zone/Zone.h"
#include <future>
#include <pistache/client.h>
#include "cmdline/CmdLineProxy.h"

using namespace Pistache;
using namespace hvs;
bool GetZoneInfo(std::string ip, int port, std::string clientID);
/*
 * zoneadd 命令行客户端
 */
std::unordered_map<std::string, std::string> zonemap;


int main(int argc, char* argv[]){
    // TODO: 1.获取账户登录信息 2.检索区域信息 3. 提交空间重命名申请
    char* demo1[19] = {const_cast<char *>("zoneadd"), const_cast<char *>("--ip"), const_cast<char *>("192.168.10.219"),
                       const_cast<char *>("-p"), const_cast<char *>("55957"), const_cast<char *>("--zonename"),
                       const_cast<char *>("compute-zone"), const_cast<char *>("--id"), const_cast<char *>("000"),
                       const_cast<char *>("--member"), const_cast<char *>("111"), const_cast<char *>("--member"), const_cast<char *>("222"),
                       const_cast<char *>("--center"), const_cast<char *>("beihang"), const_cast<char *>("--storage"), const_cast<char *>("localstorage"),
                       const_cast<char *>("--path"), const_cast<char *>("8ff50e7d-233c-44ef-a939-dcd1337fef61")};
    char* demo2[2] = {const_cast<char *>("zoneadd"), const_cast<char *>("--help")};

    // TODO: 提前准备的数据
    std::string ip ;//= "127.0.0.1";
    int port ;//= 55107;
    std::string zonename ;//= "syremotezone"; 
    //std::string zoneuuid;
    std::string ownID;// = "202"; 
    std::vector<std::string> memID;// memberID
    SpaceMetaData spaceurl;




    // TODO: 获取命令行信息
    CmdLineProxy commandline(19, demo1);
//    CmdLineProxy commandline(2, demo2);
    std::string cmdname = "zoneadd";
    // TODO：设置当前命令行解析函数
    commandline.cmd_desc_func_map[cmdname] =  [](std::shared_ptr<po::options_description> sp_cmdline_options)->void {
        po::options_description command("管理员区域添加模块");
        command.add_options()
                ("ip", po::value<std::string>(), "管理节点IP")
                ("port,p", po::value<int>(), "管理节点端口号")
                ("zonename", po::value<std::string>(), "区域名称")
                ("id", po::value<std::string>(), "主人ID")
                ("member", po::value<std::vector<std::string>>(), "区域成员")
                ("center", po::value<std::string>(), "超算名称")
                ("storage", po::value<std::string>(), "存储资源名称")
                ("path", po::value<std::string>(), "空间路径")
                ;
        sp_cmdline_options->add(command); // 添加子模块命令行描述
    };
    // TODO： 解析命令行参数，进行赋值
    commandline.cmd_do_func_map[cmdname] =  [&](std::shared_ptr<po::variables_map> sp_variables_map)->void {
        if (sp_variables_map->count("ip"))
        {
            ip = (*sp_variables_map)["ip"].as<std::string>();
        }
        if (sp_variables_map->count("port"))
        {
            port = (*sp_variables_map)["port"].as<int>();
        }
        if (sp_variables_map->count("zonename"))
        {
            zonename = (*sp_variables_map)["zonename"].as<std::string>();
        }
        if (sp_variables_map->count("id"))
        {
            ownID = (*sp_variables_map)["id"].as<std::string>();
        }
        if (sp_variables_map->count("member"))
        {
            memID = (*sp_variables_map)["member"].as<std::vector<std::string>>();
        }
        if (sp_variables_map->count("center"))
        {
            spaceurl.hostCenterName = (*sp_variables_map)["center"].as<std::string>();
        }
        if (sp_variables_map->count("storage"))
        {
            spaceurl.storageSrcName = (*sp_variables_map)["storage"].as<std::string>();
        }
        if (sp_variables_map->count("path"))
        {
            spaceurl.spacePath = (*sp_variables_map)["path"].as<std::string>();
        }
    };
    commandline.start(); //开始解析命令行参数


    // TODO: 构造请求
    Http::Client client;
    char url[256];
    snprintf(url, 256, "http://%s:%d/zone/add",ip.c_str(), port);
    auto opts = Http::Client::options().threads(1).maxConnectionsPerHost(8);
    client.init(opts);

    ZoneRegisterReq req;
    req.zoneName = zonename;
    req.ownerID = ownID;
    req.memberID = memID;
    req.spacePathInfo = spaceurl.serialize();


    std::string value = req.serialize();

    // TODO: 发送请求，并输出结果
    auto response = client.post(url).body(value).send();
    std::promise<bool> prom;
    auto fu = prom.get_future();
    response.then(
            [&](Http::Response res) {
                std::cout << res.body() << std::endl; //结果
                prom.set_value(true);
            },Async::IgnoreException);
    fu.get();
    client.shutdown();
    return 0;
}

