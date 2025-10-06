#ifndef _LOGGER_H_
#define _LOGGER_H_

#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <fstream>
#include <queue>

enum LogLevel {
    DEBUG = 0,
    INFO,
    WARNING,
    ERROR,
    FATAL
};

class AsyncLogger {
public:
    static AsyncLogger* getInstance();
    
    void init(const std::string& filepath, LogLevel level = INFO, size_t maxQueueSize = 1000);
    void stop();
    
    void log(const std::string& level, const std::string& message, 
             const std::string& file_name, int line);
    
    void setLevel(LogLevel level) { _level = level; }
    LogLevel getLevel() const { return _level; }
    
    ~AsyncLogger();

private:
    AsyncLogger();
    AsyncLogger(const AsyncLogger&) = delete;
    AsyncLogger& operator=(const AsyncLogger&) = delete;
    
    void writeThreadFunc();
    void writeToFile(const std::string& logMsg);
    
    // 获取当前时间的微秒级时间戳
    std::string getCurrentTimeMicros();
    
    std::string _filepath;
    LogLevel _level;
    
    // 双缓冲队列
    std::queue<std::string> _frontBuffer;
    std::queue<std::string> _backBuffer;
    
    std::mutex _frontMutex;
    std::mutex _backMutex;
    
    std::thread _writeThread;
    std::atomic<bool> _running;
    std::condition_variable _cv;
    
    size_t _maxQueueSize;
    std::ofstream _fileStream;
    
    static AsyncLogger* _instance;
    static std::mutex _instanceMutex;
};

// 修改宏定义以使用新的日志系统
#define LOG(level, message) do { \
    AsyncLogger::getInstance()->log(#level, message, __FILE__, __LINE__); \
} while(0)

#endif