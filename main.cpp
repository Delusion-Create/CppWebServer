#include"TcpServer.h"
#include"ThreadPool.h"

#include<iostream>
#include<cstring>
#include<signal.h>


using namespace std;



void handle_for_sigpipe()
{
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    if(sigaction(SIGPIPE, &sa, NULL))
        return;
}

int main()
{
    handle_for_sigpipe();
    TcpServer* tp=TcpServer::GetInstance("192.168.247.128",8848);
    tp->run();

    // ThreadPool mypool(3);
    // for (int i=1;i<=20;i++)
    // {
    //     mypool.addTask([i](){
    //         cout<<"任务"<<i<<" 正在执行,tid="<<this_thread::get_id()<<endl;

    //         this_thread::sleep_for(chrono::milliseconds(500));
    //     });
    // }

    // this_thread::sleep_for(chrono::seconds(5));
    
}