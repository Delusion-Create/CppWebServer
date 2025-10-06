// HttpResponse.h
#ifndef _HTTPRESPONSE_H_
#define _HTTPRESPONSE_H_

#include <string>
#include <map>

class HttpResponse {
public:
    HttpResponse();
    ~HttpResponse();
    
    // 设置状态码
    void setStatusCode(int code);
    
    // 设置状态描述
    void setStatusMessage(const std::string& message);
    
    // 设置HTTP版本
    void setVersion(const std::string& version);
    
    // 添加响应头
    void addHeader(const std::string& key, const std::string& value);
    
    // 设置响应体
    void setBody(const std::string& body);
    
    // 设置响应体（C风格字符串）
    void setBody(const char* body, size_t length);
    
    // 生成HTTP响应字符串
    std::string toString() const;
    
    // 清空响应数据
    void clear();

private:
    int _statusCode;
    std::string _statusMessage;
    std::string _version;
    std::map<std::string, std::string> _headers;
    std::string _body;
};

#endif