#ifndef _TCPTIMER_H_
#define _TCPTIMER_H_

#include "TimerNode.h"
#include <queue>
#include <vector>
#include <mutex>
#include <unordered_map>
#include <set>
#include <memory>

class TcpTimer {
public:
    TcpTimer();
    ~TcpTimer();
    
    // 添加定时器
    void addTimer(int fd, int timeout, TimerCallback cb);
    
    // 更新定时器
    void updateTimer(int fd, int timeout);
    
    // 删除定时器
    void removeTimer(int fd);
    
    // 检查并触发过期定时器
    void checkExpired();
    
    // 获取下一个定时器的剩余时间（毫秒）
    int getNextTimeout() const;
    
    // 获取定时器数量
    size_t size() const;

private:
    // 使用多重集合来存储定时器节点指针，按过期时间排序
    std::multiset<std::shared_ptr<TimerNode>, TimerNodeCompare> _timerSet;
    std::unordered_map<int, std::shared_ptr<TimerNode>> _timerMap; // 用于快速查找定时器
    mutable std::mutex _mutex;
    
    // 内部方法：清理已标记删除的定时器
    void cleanupDeletedTimers();
};

#endif