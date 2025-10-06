#include "Request.h"
#include "TcpServer.h" // 现在可以安全地包含
#include "Logger.h"
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <sys/socket.h>

Request::Request(int fd, TcpServer* server) : _fd(fd), _server(server) 
{
    LOG(INFO, "Request对象创建, fd: " + std::to_string(_fd));
}

Request::~Request() 
{
    LOG(INFO, "Request对象销毁, fd: " + std::to_string(_fd));
}

void Request::process() 
{
    char buffer[1024];
    while (true)
    {
        int recv_len = recv(_fd, buffer, sizeof(buffer), 0);
        if (recv_len == 0)
        {
            // 客户端关闭连接
            _server->closeConnection(_fd);
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
                LOG(ERROR, "recv error on fd: " + std::to_string(_fd));
                _server->closeConnection(_fd);
                break;
            }
        }
        else
        {
            // 处理接收到的数据
            std::cout << "来自客户端 " << _fd << " 的消息: " << buffer << std::endl;
            
            // 发送回复（简单回显）
            if (send(_fd, buffer, recv_len, 0) < 0)
            {
                LOG(ERROR, "send error on fd: " + std::to_string(_fd));
                _server->closeConnection(_fd);
                break;
            }
            
            // 清空缓冲区
            memset(buffer, 0, sizeof(buffer));
        }
    }
}