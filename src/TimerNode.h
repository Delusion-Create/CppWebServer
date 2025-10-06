#ifndef _TIMERNODE_H_
#define _TIMERNODE_H_

#include <functional>
#include <chrono>
#include <memory>
#include <atomic>

// 定时器回调函数类型
using TimerCallback = std::function<void()>;

class TimerNode : public std::enable_shared_from_this<TimerNode> {
public:
    TimerNode(int fd, int timeout, TimerCallback cb);
    ~TimerNode();
    
    // 获取过期时间
    std::chrono::steady_clock::time_point getExpireTime() const;
    
    // 获取关联的文件描述符
    int getFd() const;
    
    // 检查是否过期
    bool isExpired() const;
    
    // 触发回调函数
    void triggerCallback();
    
    // 获取回调函数
    TimerCallback getCallback() const;
    
    // 更新过期时间
    void update(int timeout);
    
    // 比较运算符重载（用于multiset）
    bool operator<(const TimerNode& other) const;
    
    // 标记为已删除
    void markDeleted();
    
    // 检查是否已标记删除
    bool isDeleted() const;

private:
    int _fd;                                // 关联的文件描述符
    std::chrono::steady_clock::time_point _expireTime; // 过期时间点
    TimerCallback _callback;                // 回调函数
    std::atomic<bool> _deleted;             // 标记是否已删除
};

// 用于multiset的比较函数对象
struct TimerNodeCompare {
    bool operator()(const std::shared_ptr<TimerNode>& a, 
                   const std::shared_ptr<TimerNode>& b) const;
};

#endif