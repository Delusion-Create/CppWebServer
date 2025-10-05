#include "TcpServer.h"
#include "HttpServer.h" // 现在可以安全地包含
#include "Logger.h"
#include "epoll.h"
#include <unistd.h>
#include <iostream>
#include <sys/epoll.h>
#include <cstring>
#include <fcntl.h>
#include <memory>

using namespace std;

TcpServer* TcpServer::GetInstance(std::string ip, int port)
{
    static TcpServer instance(ip, port);
    return &instance;
}

TcpServer::TcpServer(std::string ip, int port) : _ip(ip), _port(port), _pool(5)
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
    ev.events = EPOLLIN | EPOLLET;
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
                break;
            }
            else
            {
                LOG(ERROR, "accept error");
                break;
            }
        }
        
        setNonBlocking(cfd);
        
        epoll_event ev;
        ev.data.fd = cfd;
        ev.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
        epoll_ctl(_epfd, EPOLL_CTL_ADD, cfd, &ev);
        
        {
            std::lock_guard<std::mutex> lock(_mapMutex);
            _clientLocks[cfd];
        }
        
        LOG(INFO, "新连接建立成功, fd: " + to_string(cfd));
    }
}

void TcpServer::handleEvent(int fd, uint32_t events)
{
    if (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
    {
        closeConnection(fd);
        return;
    }
    
    if (events & EPOLLIN)
    {
        auto httpServer = std::make_shared<HttpServer>(fd, this);
        
        _pool.addTask([this, fd, httpServer]() {
            std::lock_guard<std::mutex> lock(_clientLocks[fd]);
            httpServer->process();
        });
    }
}

void TcpServer::closeConnection(int fd)
{
    epoll_ctl(_epfd, EPOLL_CTL_DEL, fd, NULL);
    close(fd);
    
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
                acceptConnection();
            }
            else
            {
                handleEvent(fd, revents);
            }
        }
    }
}