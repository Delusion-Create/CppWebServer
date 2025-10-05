#ifndef _TCPSERVER_H_
#define _TCPSERVER_H_

#include "ThreadPool.h"
#include <string>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <map>
#include <mutex>

class TcpServer
{
private:
    std::string _ip;        // ip地址
    int _port;              // 端口号
    int _sfd;               // 服务器监听文件描述符
    sockaddr_in _sin;       // 监听套接字
    int _epfd;              // epoll模型描述符
    ThreadPool _pool;       // 线程池
    
    // 连接管理相关
    std::map<int, std::mutex> _clientLocks; // 客户端锁，确保每个连接线程安全
    std::mutex _mapMutex;   // 保护_clientLocks的互斥锁

private:
    // 构造函数私有
    TcpServer(std::string ip, int port);
    // 将拷贝构造函数和拷贝赋值函数私有或删除（防拷贝）
    TcpServer(const TcpServer&) = delete;
    TcpServer* operator=(const TcpServer&) = delete;

private:
    // 私有方法
    void initialServer();
    void setNonBlocking(int fd);
    int getSock();

public:
    static TcpServer* GetInstance(std::string ip = "127.0.0.1", int port = 8848);
    void run();
    void acceptConnection();
    void handleEvent(int fd, uint32_t events);
    void closeConnection(int fd);
    ~TcpServer();
};

#endif