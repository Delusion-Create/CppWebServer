#include"TcpServer.h"
#include"Logger.h"
#include"Util.h"

#include<iostream>

using namespace std;



int main(int argc,const char* argv[])
{
    // 初始化日志系统
    AsyncLogger::getInstance()->init("server.log", INFO, 1000);
    
    //忽略SIGPIPE信号
    Util::handle_for_sigpipe();

    if(argc<3)
    {
        cout<<"用法: "<<argv[0]<<" ip 端口 "<<endl;
        cout<<"不指定ip端口,将默认为 127.0.0.1:8848"<<endl;
        TcpServer* tp=TcpServer::GetInstance("127.0.0.1",8848);
        tp->run();
    }
    else if(argc==3)
    {
        TcpServer* tp=TcpServer::GetInstance("192.168.247.128",8848);
        tp->run();
    }
    else
    {
        cerr<<"参数过多!!!"<<endl;
        cout<<"用法: "<<argv[0]<<" ip 端口 "<<endl;
        cout<<"不指定ip端口,将默认为 127.0.0.1:8848"<<endl;
        return 1;

    }
    
    // 程序退出前停止日志系统
    AsyncLogger::getInstance()->stop();
    
}