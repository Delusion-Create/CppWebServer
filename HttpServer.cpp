#include "HttpServer.h"
#include "TcpServer.h"
#include "Logger.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <sys/socket.h>

HttpServer::HttpServer(int fd, TcpServer* server) : _fd(fd), _server(server), _keepAlive(false) 
{
    LOG(INFO, "HttpServer对象创建, fd: " + std::to_string(_fd));
}

HttpServer::~HttpServer() 
{
    LOG(INFO, "HttpServer对象销毁, fd: " + std::to_string(_fd));
}

void HttpServer::process() 
{
    char buffer[1024];
    std::string requestData;
    
    // 读取请求数据
    while (true)
    {
        int recv_len = recv(_fd, buffer, sizeof(buffer), 0);
        if (recv_len == 0)
        {
            LOG(INFO, "客户端关闭连接, fd: " + std::to_string(_fd));
            _server->closeConnection(_fd);
            return;
        }
        else if (recv_len < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                break;
            }
            else
            {
                LOG(ERROR, "recv error on fd: " + std::to_string(_fd));
                _server->closeConnection(_fd);
                return;
            }
        }
        else
        {
            requestData.append(buffer, recv_len);
            memset(buffer, 0, sizeof(buffer));
        }
    }
    
    if (!requestData.empty()) {
        LOG(INFO, "收到HTTP请求数据: " + requestData);
        handleHttpRequest(requestData);
    }
    
    // 修改：根据Keep-Alive状态决定是否设置定时器
    if (_keepAlive) {
        // 设置30秒超时定时器
        _server->setupTimer(_fd, 30000);
        LOG(INFO, "Keep-Alive连接已建立定时器, fd: " + std::to_string(_fd));
    } else {
        // 关闭连接
        _server->closeConnection(_fd);
    }
}

void HttpServer::handleHttpRequest(const std::string& requestData) {
    HttpRequest request;
    HttpResponse response;
    
    if (request.parse(requestData.c_str(), requestData.size())) {
        LOG(INFO, "收到HTTP请求: " + request.getMethod() + " " + request.getPath());
        
        // 新增：解析Connection头，判断是否为Keep-Alive
        std::string connectionHeader = request.getHeader("Connection");
        _keepAlive = (connectionHeader == "keep-alive" || connectionHeader == "Keep-Alive");
        
        // 在响应中设置对应的Connection头
        if (_keepAlive) {
            response.addHeader("Connection", "keep-alive");
            response.addHeader("Keep-Alive", "timeout=30"); // 告诉客户端超时时间
        } else {
            response.addHeader("Connection", "close");
        }
        
        // 原有的请求处理逻辑保持不变
        if (request.getMethod() == "GET") {
            if (request.getPath() == "/") {
                response.setBody("Hello, World! This is the home page.");
            } else if (request.getPath() == "/about") {
                response.setBody("About Us: This is a simple HTTP server.");
            } else {
                response.setStatusCode(404);
                response.setStatusMessage("Not Found");
                response.setBody("404 Not Found: The requested resource was not found on this server.");
            }
        } else if (request.getMethod() == "POST") {
            response.setBody("Received POST data: " + request.getBody());
        } else {
            response.setStatusCode(405);
            response.setStatusMessage("Method Not Allowed");
            response.setBody("405 Method Not Allowed: The requested method is not supported.");
        }
        
        response.addHeader("Content-Type", "text/plain");
        sendHttpResponse(response.toString());
        LOG(INFO, "响应内容------:"+response.toString());
    } else {
        LOG(ERROR, "HTTP请求解析失败, fd: " + std::to_string(_fd));
        
        response.setStatusCode(400);
        response.setStatusMessage("Bad Request");
        response.setBody("400 Bad Request: The request could not be understood by the server.");
        response.addHeader("Content-Type", "text/plain");
        
        sendHttpResponse(response.toString());
    }
}

void HttpServer::sendHttpResponse(const std::string& responseData) {
    if (send(_fd, responseData.c_str(), responseData.size(), 0) < 0) {
        LOG(ERROR, "send error on fd: " + std::to_string(_fd));
    }
}