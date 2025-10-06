#ifndef _HTTPSERVER_H_
#define _HTTPSERVER_H_

#include <string>

// 前置声明，避免包含TcpServer.h
class TcpServer;

class HttpServer {
public:
    HttpServer(int fd, TcpServer* server);
    ~HttpServer();
    void process();
    
private:
    int _fd;
    TcpServer* _server;
    bool _keepAlive; // 新增：标记连接是否应该保持
    
    // 处理HTTP请求
    void handleHttpRequest(const std::string& requestData);
    
    // 发送HTTP响应
    void sendHttpResponse(const std::string& responseData);
};

#endif