#ifndef _HTTPRESPONSE_H_
#define _HTTPRESPONSE_H_

#define HTTP_VERSION "HTTP/1.0"
#define LINE_END "\r\n"

#define OK 200
#define BAD_REQUEST 400
#define NOT_FOUND 404
#define INTERNAL_SERVER_ERROR 500
 
#define PAGE_400 "400.html"
#define PAGE_404 "404.html"
#define PAGE_500 "500.html"

#include<string>
#include<vector>


//HTTP响应
class HttpResponse{
    public:
        //HTTP响应内容
        std::string _status_line;                  //状态行
        std::vector<std::string> _response_header; //响应报头
        std::string _blank;                        //空行
        std::string _response_body;                //响应正文（CGI相关）
 
        //所需数据
        int _status_code;    //状态码
        int _fd;             //响应文件的fd  （非CGI相关）
        int _size;           //响应文件的大小（非CGI相关）
        std::string _suffix; //响应文件的后缀（非CGI相关）
    public:
        HttpResponse()
            :_blank(LINE_END) //设置空行
            ,_status_code(OK) //状态码默认为200
            ,_fd(-1)          //响应文件的fd初始化为-1
            ,_size(0)         //响应文件的大小默认为0
        {}
        ~HttpResponse()
        {}
};


#endif