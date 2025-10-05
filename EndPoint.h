#ifndef _ENDPOINT_H_
#define _ENDPOINT_H_

#include"HttpRequest.h"
#include"HttpResponse.h"

//服务端EndPoint
class EndPoint{
    private:
        int _sock;                   //通信的套接字
        HttpRequest _http_request;   //HTTP请求
        HttpResponse _http_response; //HTTP响应
    public:
        EndPoint(int sock)
            :_sock(sock)
        {}
        //读取请求
        void RecvHttpRequest()
        {
             RecvHttpRequestLine();    //读取请求行
            RecvHttpRequestHeader();  //读取请求报头和空行
            ParseHttpRequestLine();   //解析请求行
            ParseHttpRequestHeader(); //解析请求报头
            RecvHttpRequestBody();    //读取请求正文
        }
        //处理请求
        void HandlerHttpRequest();
        //构建响应
        void BuildHttpResponse();
        //发送响应
        void SendHttpResponse();
        ~EndPoint()
        {}
};



#endif