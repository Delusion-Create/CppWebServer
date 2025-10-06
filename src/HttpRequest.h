// HttpRequest.h
#ifndef _HTTPREQUEST_H_
#define _HTTPREQUEST_H_

#include <string>
#include <map>

class HttpRequest {
public:
    HttpRequest();
    ~HttpRequest();
    
    // 解析HTTP请求
    bool parse(const char* data, size_t length);
    
    // 获取请求方法
    const std::string& getMethod() const;
    
    // 获取请求路径
    const std::string& getPath() const;
    
    // 获取HTTP版本
    const std::string& getVersion() const;
    
    // 获取查询参数
    const std::string& getQueryParam(const std::string& key) const;
    
    // 获取请求头
    const std::string& getHeader(const std::string& key) const;
    
    // 获取所有请求头
    const std::map<std::string, std::string>& getAllHeaders() const;
    
    // 获取请求体
    const std::string& getBody() const;
    
    // 清空请求数据
    void clear();

private:
    std::string _method;
    std::string _path;
    std::string _version;
    std::map<std::string, std::string> _headers;
    std::map<std::string, std::string> _queryParams;
    std::string _body;
    
    // 解析请求行
    bool parseRequestLine(const std::string& line);
    
    // 解析请求头
    void parseHeader(const std::string& line);
    
    // 解析查询参数
    void parseQueryParams(const std::string& queryStr);
};

#endif