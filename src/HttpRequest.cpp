// HttpRequest.cpp
#include "HttpRequest.h"
#include <sstream>
#include <iostream>
#include <algorithm>

HttpRequest::HttpRequest() {}

HttpRequest::~HttpRequest() {}

bool HttpRequest::parse(const char* data, size_t length) {
    clear();
    
    std::string requestStr(data, length);
    std::istringstream stream(requestStr);
    std::string line;
    
    // 解析请求行
    if (!std::getline(stream, line) || !parseRequestLine(line)) {
        return false;
    }
    // 解析请求头
    //如果读到的内容是'\r'说明这是请求头与请求体之间的空行内容只有\r\n
    //因为geline读取时忽略\n
    while (std::getline(stream, line) && line != "\r") {
        if (line[line.size()-1] == '\r') {
            // 去除最后的\r
            line = line.substr(0, line.size()-1);
        }
        parseHeader(line);
    }
    
    // 解析请求体
    size_t bodyPos = requestStr.find("\r\n\r\n");
    if (bodyPos != std::string::npos) {
        bodyPos += 4; // 跳过空行
        _body = requestStr.substr(bodyPos);
    }
    
    return true;
}

bool HttpRequest::parseRequestLine(const std::string& line) {
    std::istringstream lineStream(line);
    lineStream >> _method >> _path >> _version;
    
    if (_method.empty() || _path.empty() || _version.empty()) {
        return false;
    }
    
    // 解析查询参数
    size_t queryPos = _path.find('?');
    if (queryPos != std::string::npos) {
        std::string queryStr = _path.substr(queryPos + 1);
        _path = _path.substr(0, queryPos);
        parseQueryParams(queryStr);
    }
    
    return true;
}

void HttpRequest::parseHeader(const std::string& line) {
    size_t colonPos = line.find(':');
    if (colonPos != std::string::npos) {
        std::string key = line.substr(0, colonPos);
        std::string value = line.substr(colonPos + 1);
        
        // 去除首尾空格
        value.erase(0, value.find_first_not_of(" "));
        value.erase(value.find_last_not_of(" ") + 1);
        
        _headers[key] = value;
    }
}

void HttpRequest::parseQueryParams(const std::string& queryStr) {
    std::istringstream queryStream(queryStr);
    std::string pair;
    
    while (std::getline(queryStream, pair, '&')) {
        size_t equalPos = pair.find('=');
        if (equalPos != std::string::npos) {
            std::string key = pair.substr(0, equalPos);
            std::string value = pair.substr(equalPos + 1);
            _queryParams[key] = value;
        }
    }
}

const std::string& HttpRequest::getMethod() const {
    return _method;
}

const std::string& HttpRequest::getPath() const {
    return _path;
}

const std::string& HttpRequest::getVersion() const {
    return _version;
}

const std::string& HttpRequest::getQueryParam(const std::string& key) const {
    static const std::string emptyStr;
    auto it = _queryParams.find(key);
    if (it != _queryParams.end()) {
        return it->second;
    }
    return emptyStr;
}

const std::string& HttpRequest::getHeader(const std::string& key) const {
    static const std::string emptyStr;
    auto it = _headers.find(key);
    if (it != _headers.end()) {
        return it->second;
    }
    return emptyStr;
}

const std::map<std::string, std::string>& HttpRequest::getAllHeaders() const {
    return _headers;
}

const std::string& HttpRequest::getBody() const {
    return _body;
}

void HttpRequest::clear() {
    _method.clear();
    _path.clear();
    _version.clear();
    _headers.clear();
    _queryParams.clear();
    _body.clear();
}