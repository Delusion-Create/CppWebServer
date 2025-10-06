#ifndef _TCPSERVER_H_
#define _TCPSERVER_H_

#include "ThreadPool.h"
#include "TcpTimer.h"
#include <string>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <map>
#include <mutex>
#include <memory>

//epoll模型最大监听事件数
#define MAXEPOLLEVENTS 524287

// 前置声明
class HttpServer;

class TcpServer
{
private:
    std::string _ip;
    int _port;
    int _sfd;
    sockaddr_in _sin;
    int _epfd;
    ThreadPool _pool;
    TcpTimer _timer; // 定时器管理器
    
    std::map<int, std::mutex> _clientLocks;
    std::mutex _mapMutex;

private:
    TcpServer(std::string ip, int port);
    TcpServer(const TcpServer&) = delete;
    TcpServer* operator=(const TcpServer&) = delete;

private:
    void initialServer();
    void setNonBlocking(int fd);
    int getSock();

public:
    static TcpServer* GetInstance(std::string ip = "127.0.0.1", int port = 8848);
    void run();
    void acceptConnection();
    void handleEvent(int fd, uint32_t events);
    void closeConnection(int fd);
    void setupTimer(int fd, int timeout);
    void removeTimer(int fd);            
    ~TcpServer();
};

#endif