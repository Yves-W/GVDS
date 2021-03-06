#!/bin/bash

# 获取当前Linux操作系统发行版 类型
Get_OS_Type()
{
    if grep -Eqii "CentOS" /etc/issue || grep -Eq "CentOS" /etc/*-release; then
        OSType='CentOS'
        PM='yum'
    elif grep -Eqi "Red Hat Enterprise Linux Server" /etc/issue || grep -Eq "Red Hat Enterprise Linux Server" /etc/*-release; then
        OSType='RHEL'
        PM='yum'
    elif grep -Eqi "Aliyun" /etc/issue || grep -Eq "Aliyun" /etc/*-release; then
        OSType='Aliyun'
        PM='yum'
    elif grep -Eqi "Fedora" /etc/issue || grep -Eq "Fedora" /etc/*-release; then
        OSType='Fedora'
        PM='yum'
    elif grep -Eqi "Debian" /etc/issue || grep -Eq "Debian" /etc/*-release; then
        OSType='Debian'
        PM='apt'
    elif grep -Eqi "Ubuntu" /etc/issue || grep -Eq "Ubuntu" /etc/*-release; then
        OSType='Ubuntu'
        PM='apt'
    elif grep -Eqi "Raspbian" /etc/issue || grep -Eq "Raspbian" /etc/*-release; then
        OSType='Raspbian'
        PM='apt'
    else
        OSType='unknow'
    fi
}

# 检测当前类型是否是Centos/Fedora, 是否安装了依赖库，如果没有，则使用使用 yun 源 安装配置文件依赖库
INSTALL_CentOS_Fedora(){
    echo "检测到当前系统类型为: $OSType "
    # Yaoxu - 2019年03月04日17:28:09
    sudo yum install -y libconfig libconfig-devel 
}

INSTALL_Ubuntu(){
    echo "检测到当前系统类型为: $OSType "
    # Yaoxu - 2019年03月04日17:28:09
    sudo apt install -y libconfig-dev libgtest-dev libcouchbase-dev \
        doxygen 
}

Get_OS_Type

#判断系统发行版，安装依赖。
if [[ ${OSType} =  "Fedora" ]] || [[ ${OSType} =  "CentOS" ]];then
    INSTALL_CentOS_Fedora
elif [[ ${OSType} =  "Ubuntu" ]]; then
    INSTALL_Ubuntu
else
    echo "当前脚本支持 CentOS, Ubuntu 和 Fedora 64bit 系统, 当前系统为 $OSType"
fi
