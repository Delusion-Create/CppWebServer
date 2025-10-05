#include"Request.h"

#include<arpa/inet.h>
#include<sys/socket.h>
#include<unistd.h>
#include<iostream>
#include<cstring>
#include<sys/epoll.h>

#include<thread>

using namespace std;

Request::Request(int epfd,int cfd):_epfd(epfd),_cfd(cfd)
{

}

Request::~Request()
{
    cout<<"Request 对象析构"<<endl;
}

void Request::handleRequest()
{
    cout<<"线程: "<<this_thread::get_id()<<" 正在处理请求"<<endl;
    //从_cfd中读取数据
    char buffer[1024];
    int recv_len=recv(_cfd,buffer,sizeof(buffer),0);
    if(recv_len==0)
    {
        cout<<"客户端断开连接"<<endl;
        return;
        
    }
    else if(recv_len<0)
    {
        cerr<<"recv err"<<endl;
        return;
        
    }
    else
    {
        cout<<"来自客户端的消息:"<<buffer<<endl;
        
        //发送回去
        if(send(_cfd,buffer,strlen(buffer)+1,0)<0){cerr<<"send err"<<endl;return;}
        return;
    }


}

void Request::setFd(int fd)
{
    _fd=fd;
}

int Request::getFd()
{
    return _fd;
}