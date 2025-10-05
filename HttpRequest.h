#ifndef _HTTPREQUEST_H_
#define _HTTPREQUEST_H_

#include<string>
#include<vector>
#include<unordered_map>


//HTTP请求
class HttpRequest{
    public:
        //HTTP请求内容
        std::string _request_line;                //请求行
        std::vector<std::string> _request_header; //请求报头
        std::string _blank;                       //空行
        std::string _request_body;                //请求正文
 
        //解析结果
        std::string _method;       //请求方法
        std::string _uri;          //URI
        std::string _version;      //版本号
        std::unordered_map<std::string, std::string> _header_kv; //请求报头中的键值对
        int _content_length;       //正文长度
        std::string _path;         //请求资源的路径
        std::string _query_string; //uri中携带的参数
 
        //CGI相关
        bool _cgi; //是否需要使用CGI模式
    public:
        HttpRequest()
            :_content_length(0) //默认请求正文长度为0
            ,_cgi(false)        //默认不使用CGI模式
        {}
        ~HttpRequest()
        {}
};


#endif