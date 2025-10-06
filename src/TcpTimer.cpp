#include "TcpTimer.h"
#include "Logger.h"
#include <chrono>
#include <algorithm>

TcpTimer::TcpTimer() {
    //LOG(DEBUG, "TcpTimer对象创建");
}

TcpTimer::~TcpTimer() {
    //LOG(INFO, "TcpTimer对象销毁开始");
    std::lock_guard<std::mutex> lock(_mutex);
    
    // 标记所有定时器为已删除
    for (auto& pair : _timerMap) {
        pair.second->markDeleted();
    }
    
    _timerMap.clear();
    _timerSet.clear();
    //LOG(INFO, "TcpTimer对象销毁完成");
}

void TcpTimer::addTimer(int fd, int timeout, TimerCallback cb) {
    LOG(DEBUG, "添加定时器开始, fd: " + std::to_string(fd));
    std::lock_guard<std::mutex> lock(_mutex);
    
    // 如果已存在相同fd的定时器，先删除
    if (_timerMap.find(fd) != _timerMap.end()) {
        LOG(WARNING, "添加定时器时发现已存在相同fd的定时器, fd: " + std::to_string(fd));
        removeTimer(fd);
    }
    
    // 创建新定时器节点
    auto node = std::make_shared<TimerNode>(fd, timeout, cb);
    _timerSet.insert(node);
    _timerMap[fd] = node;
    
    LOG(INFO, "添加定时器成功, fd: " + std::to_string(fd) + 
              ", 超时时间: " + std::to_string(timeout) + "ms");
}

void TcpTimer::updateTimer(int fd, int timeout) {
    LOG(DEBUG, "更新定时器开始, fd: " + std::to_string(fd));
    std::lock_guard<std::mutex> lock(_mutex);
    
    auto it = _timerMap.find(fd);
    if (it != _timerMap.end()) {
        // 标记旧定时器为已删除
        it->second->markDeleted();
        
        // 创建新定时器
        auto newNode = std::make_shared<TimerNode>(fd, timeout, it->second->getCallback());
        _timerSet.insert(newNode);
        _timerMap[fd] = newNode;
        
        LOG(INFO, "更新定时器成功, fd: " + std::to_string(fd));
    } else {
        LOG(WARNING, "尝试更新不存在的定时器, fd: " + std::to_string(fd));
    }
}

void TcpTimer::removeTimer(int fd) {
    LOG(DEBUG, "移除定时器开始, fd: " + std::to_string(fd));
    std::lock_guard<std::mutex> lock(_mutex);
    
    auto it = _timerMap.find(fd);
    if (it != _timerMap.end()) {
        // 标记定时器为已删除
        it->second->markDeleted();
        
        // 从映射中移除
        _timerMap.erase(it);
        
        LOG(INFO, "移除定时器成功, fd: " + std::to_string(fd));
    } else {
        LOG(WARNING, "尝试移除不存在的定时器, fd: " + std::to_string(fd));
    }
}

void TcpTimer::checkExpired() {
    LOG(DEBUG, "检查过期定时器开始");
    std::vector<std::shared_ptr<TimerNode>> expiredNodes;
    
    {
        std::lock_guard<std::mutex> lock(_mutex);
        //auto now = std::chrono::steady_clock::now();
        
        // 收集所有过期的定时器
        for (auto it = _timerSet.begin(); it != _timerSet.end(); ) {
            auto node = *it;
            if (node->isExpired() && !node->isDeleted()) {
                expiredNodes.push_back(node);
                it = _timerSet.erase(it);
            } else if (node->isDeleted()) {
                // 清理已标记删除的定时器
                it = _timerSet.erase(it);
            } else {
                break; // 定时器按时间排序，后面的都不会过期
            }
        }
    }
    
    // 在锁外触发回调，避免死锁
    for (auto& node : expiredNodes) {
        // 从映射中移除
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _timerMap.erase(node->getFd());
        }
        
        // 触发回调
        node->triggerCallback();
    }
    
    LOG(INFO, "处理了 " + std::to_string(expiredNodes.size()) + " 个过期定时器");
}

int TcpTimer::getNextTimeout() const {
    LOG(DEBUG, "获取下一个超时时间开始");
    std::lock_guard<std::mutex> lock(_mutex);
    
    if (_timerSet.empty()) {
        LOG(DEBUG, "没有定时器，返回-1");
        return -1; // 没有定时器
    }
    
    // 跳过已标记删除的定时器
    auto it = _timerSet.begin();
    while (it != _timerSet.end() && (*it)->isDeleted()) {
        ++it;
    }
    
    if (it == _timerSet.end()) {
        LOG(DEBUG, "所有定时器都已标记删除，返回-1");
        return -1;
    }
    
    auto now = std::chrono::steady_clock::now();
    auto nextExpire = (*it)->getExpireTime();
    
    if (nextExpire <= now) {
        LOG(DEBUG, "有定时器已过期,返回0");
        return 0; // 立即触发
    }
    
    auto timeout = std::chrono::duration_cast<std::chrono::milliseconds>(
        nextExpire - now).count();
    
    LOG(DEBUG, "下一个超时时间: " + std::to_string(timeout) + "ms");
    return timeout;
}

size_t TcpTimer::size() const {
    std::lock_guard<std::mutex> lock(_mutex);
    size_t count = _timerMap.size();
    LOG(DEBUG, "当前定时器数量: " + std::to_string(count));
    return count;
}

void TcpTimer::cleanupDeletedTimers() {
    LOG(DEBUG, "清理已删除定时器开始");
    std::lock_guard<std::mutex> lock(_mutex);
    
    // 从集合中移除已标记删除的定时器
    for (auto it = _timerSet.begin(); it != _timerSet.end(); ) {
        if ((*it)->isDeleted()) {
            it = _timerSet.erase(it);
        } else {
            ++it;
        }
    }
    
    LOG(DEBUG, "清理已删除定时器完成");
}