#ifndef _REQUEST_H_
#define _REQUEST_H_

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
};

#endif