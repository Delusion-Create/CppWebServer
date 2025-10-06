#include "TimerNode.h"
#include <iostream>
#include "Logger.h"

TimerNode::TimerNode(int fd, int timeout, TimerCallback cb)
    : _fd(fd), _callback(cb), _deleted(false) {
    update(timeout);
    //LOG(DEBUG, "TimerNode创建, fd: " + std::to_string(fd));
}

TimerNode::~TimerNode() {
    //LOG(DEBUG, "TimerNode销毁, fd: " + std::to_string(_fd));
}

std::chrono::steady_clock::time_point TimerNode::getExpireTime() const {
    return _expireTime;
}

int TimerNode::getFd() const {
    return _fd;
}

bool TimerNode::isExpired() const {
    return std::chrono::steady_clock::now() > _expireTime;
}

void TimerNode::triggerCallback() {
    if (_callback && !_deleted) {
        //LOG(DEBUG, "触发定时器回调, fd: " + std::to_string(_fd));
        _callback();
    }
}

TimerCallback TimerNode::getCallback() const {
    return _callback;
}

void TimerNode::update(int timeout) {
    _expireTime = std::chrono::steady_clock::now() + 
                  std::chrono::milliseconds(timeout);
    LOG(DEBUG, "更新定时器, fd: " + std::to_string(_fd) + 
               ", 新超时时间: " + std::to_string(timeout) + "ms");
}

bool TimerNode::operator<(const TimerNode& other) const {
    return _expireTime < other._expireTime;
}

void TimerNode::markDeleted() {
    _deleted = true;
    //LOG(DEBUG, "标记定时器为已删除, fd: " + std::to_string(_fd));
}

bool TimerNode::isDeleted() const {
    return _deleted;
}

bool TimerNodeCompare::operator()(const std::shared_ptr<TimerNode>& a, 
                                 const std::shared_ptr<TimerNode>& b) const {
    return a->getExpireTime() < b->getExpireTime();
}