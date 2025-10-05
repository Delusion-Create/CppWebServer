#include "HttpServer.h"
#include "TcpServer.h" // 现在可以安全地包含
#include "Logger.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <sys/socket.h>

HttpServer::HttpServer(int fd, TcpServer* server) : _fd(fd), _server(server) 
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
            // 客户端关闭连接
            LOG(INFO, "客户端关闭连接, fd: " + std::to_string(_fd));
            _server->closeConnection(_fd);
            return;  // 直接返回，不再继续处理
        }
        else if (recv_len < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                break;  // 数据读取完毕，退出循环
            }
            else
            {
                LOG(ERROR, "recv error on fd: " + std::to_string(_fd));
                _server->closeConnection(_fd);
                return;  // 发生错误，关闭连接并返回
            }
        }
        else
        {
            requestData.append(buffer, recv_len);
            memset(buffer, 0, sizeof(buffer));
        }
    }
    
    // 处理HTTP请求
    if (!requestData.empty()) {
        LOG(INFO, "收到HTTP请求数据: " + requestData);
        handleHttpRequest(requestData);
    }
    
    // 处理完成后立即关闭连接
    _server->closeConnection(_fd);
}

void HttpServer::handleHttpRequest(const std::string& requestData) {
    HttpRequest request;
    HttpResponse response;
    
    // 解析HTTP请求
    if (request.parse(requestData.c_str(), requestData.size())) {
        LOG(INFO, "收到HTTP请求: " + request.getMethod() + " " + request.getPath());
        
        // 根据请求方法和路径处理请求
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
            // 处理POST请求
            response.setBody("Received POST data: " + request.getBody());
        } else {
            response.setStatusCode(405);
            response.setStatusMessage("Method Not Allowed");
            response.setBody("405 Method Not Allowed: The requested method is not supported.");
        }
        
        // 设置Content-Type
        response.addHeader("Content-Type", "text/plain");
        
        // 发送响应
        sendHttpResponse(response.toString());
        LOG(INFO, "响应内容------:"+response.toString());
    } else {
        LOG(ERROR, "HTTP请求解析失败, fd: " + std::to_string(_fd));
        
        // 发送错误响应
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
        //在process()中统一关闭
        //_server->closeConnection(_fd);
    }
}