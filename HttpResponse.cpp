// HttpResponse.cpp
#include "HttpResponse.h"
#include <sstream>

HttpResponse::HttpResponse() : _statusCode(200), _statusMessage("OK"), _version("HTTP/1.1") {}

HttpResponse::~HttpResponse() {}

void HttpResponse::setStatusCode(int code) {
    _statusCode = code;
}

void HttpResponse::setStatusMessage(const std::string& message) {
    _statusMessage = message;
}

void HttpResponse::setVersion(const std::string& version) {
    _version = version;
}

void HttpResponse::addHeader(const std::string& key, const std::string& value) {
    _headers[key] = value;
}

void HttpResponse::setBody(const std::string& body) {
    _body = body;
    addHeader("Content-Length", std::to_string(_body.size()));
}

void HttpResponse::setBody(const char* body, size_t length) {
    _body.assign(body, length);
    addHeader("Content-Length", std::to_string(_body.size()));
}

std::string HttpResponse::toString() const {
    std::ostringstream stream;
    
    // 响应行
    stream << _version << " " << _statusCode << " " << _statusMessage << "\r\n";
    
    // 响应头
    for (const auto& header : _headers) {
        stream << header.first << ": " << header.second << "\r\n";
    }
    
    // 空行
    stream << "\r\n";
    
    // 响应体
    stream << _body;
    
    return stream.str();
}

void HttpResponse::clear() {
    _statusCode = 200;
    _statusMessage = "OK";
    _version = "HTTP/1.1";
    _headers.clear();
    _body.clear();
}