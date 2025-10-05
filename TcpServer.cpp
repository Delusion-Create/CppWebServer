#include "TcpServer.h"
#include "Logger.h"
#include "epoll.h"
#include <unistd.h>
#include <iostream>
#include <sys/epoll.h>
#include <cstring>
#include <fcntl.h>

using namespace std;

TcpServer* TcpServer::GetInstance(std::string ip, int port)
{
    static TcpServer instance(ip, port);
    return &instance;
}

TcpServer::TcpServer(std::string ip, int port) : _ip(ip), _port(port), _pool(3)
{
    initialServer();
}

TcpServer::~TcpServer()
{
    close(_sfd);
    close(_epfd);
}

void TcpServer::setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void TcpServer::initialServer()
{
    // 创建socket
    _sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (_sfd < 0)
    { 
        LOG(FATAL, "socket error!");
        exit(1);
    }
    
    // 设置端口复用
    int opt = 1;
    setsockopt(_sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    _sin.sin_family = AF_INET;
    _sin.sin_port = htons(_port);
    _sin.sin_addr.s_addr = inet_addr(_ip.c_str());

    if (bind(_sfd, (sockaddr*)&_sin, sizeof(_sin)) < 0)
    {
        LOG(FATAL, "bind err");
        exit(1);
    }
    
    if (listen(_sfd, 10) < 0)
    {
        LOG(FATAL, "listen err");
        exit(1);
    }
    
    cout << "server running at " << _ip << ":" << _port << endl;
    LOG(INFO, "server initial success");

    // 创建epoll实例
    _epfd = epoll_create1(0);
    if (_epfd < 0)
    {
        LOG(FATAL, "epoll_create1 error!");
        exit(1);
    }

    // 将监听socket添加到epoll
    epoll_event ev;
    ev.data.fd = _sfd;
    ev.events = EPOLLIN | EPOLLET; // 边缘触发模式
    epoll_ctl(_epfd, EPOLL_CTL_ADD, _sfd, &ev);
    
    // 设置监听socket为非阻塞
    setNonBlocking(_sfd);

    // 启动线程池
    _pool.startThreadPool();
    LOG(INFO, "threadPool start");
}

int TcpServer::getSock()
{
    return _sfd;
}

void TcpServer::acceptConnection()
{
    sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    
    while (true)
    {
        int cfd = accept(_sfd, (struct sockaddr*)&client_addr, &client_addr_len);
        if (cfd < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                // 已经接受完所有连接
                break;
            }
            else
            {
                LOG(ERROR, "accept error");
                break;
            }
        }
        
        // 设置客户端socket为非阻塞
        setNonBlocking(cfd);
        
        // 将客户端socket添加到epoll，监听读事件
        epoll_event ev;
        ev.data.fd = cfd;
        ev.events = EPOLLIN | EPOLLET | EPOLLRDHUP; // 边缘触发 + 监听连接关闭
        epoll_ctl(_epfd, EPOLL_CTL_ADD, cfd, &ev);
        
        // 为客户端连接创建锁
        {
            std::lock_guard<std::mutex> lock(_mapMutex);
            _clientLocks[cfd]; // 插入一个空的mutex
        }
        
        LOG(INFO, "新连接建立成功, fd: " + to_string(cfd));
    }
}

void TcpServer::handleEvent(int fd, uint32_t events)
{
    // 处理连接关闭事件
    if (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
    {
        closeConnection(fd);
        return;
    }
    
    // 处理可读事件
    if (events & EPOLLIN)
    {
        // 将读取任务提交到线程池
        _pool.addTask([this, fd]() {
            // 获取该连接的锁，确保同一连接的处理是串行的
            std::lock_guard<std::mutex> lock(_clientLocks[fd]);
            
            char buffer[1024];
            while (true)
            {
                int recv_len = recv(fd, buffer, sizeof(buffer), 0);
                if (recv_len == 0)
                {
                    // 客户端关闭连接
                    closeConnection(fd);
                    break;
                }
                else if (recv_len < 0)
                {
                    if (errno == EAGAIN || errno == EWOULDBLOCK)
                    {
                        // 数据已读取完毕
                        break;
                    }
                    else
                    {
                        // 发生错误，关闭连接
                        LOG(ERROR, "recv error on fd: " + to_string(fd));
                        closeConnection(fd);
                        break;
                    }
                }
                else
                {
                    // 处理接收到的数据
                    cout << "来自客户端 " << fd << " 的消息: " << buffer << endl;
                    
                    // 发送回复（简单回显）
                    if (send(fd, buffer, recv_len, 0) < 0)
                    {
                        LOG(ERROR, "send error on fd: " + to_string(fd));
                        closeConnection(fd);
                        break;
                    }
                    
                    // 清空缓冲区
                    memset(buffer, 0, sizeof(buffer));
                }
            }
        });
    }
}

void TcpServer::closeConnection(int fd)
{
    // 从epoll中移除
    epoll_ctl(_epfd, EPOLL_CTL_DEL, fd, NULL);
    
    // 关闭socket
    close(fd);
    
    // 从连接管理中移除
    {
        std::lock_guard<std::mutex> lock(_mapMutex);
        _clientLocks.erase(fd);
    }
    
    LOG(INFO, "连接关闭, fd: " + to_string(fd));
}

void TcpServer::run()
{
    epoll_event events[1024];
    
    while (true)
    {
        int num = epoll_wait(_epfd, events, 1024, -1);
        if (num < 0)
        {
            LOG(ERROR, "epoll_wait error");
            continue;
        }
        
        for (int i = 0; i < num; i++)
        {
            int fd = events[i].data.fd;
            uint32_t revents = events[i].events;
            
            if (fd == _sfd)
            {
                // 监听socket有新连接
                acceptConnection();
            }
            else
            {
                // 客户端socket有事件
                handleEvent(fd, revents);
            }
        }
    }
}