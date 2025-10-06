#include "Logger.h"

#include <iostream>   
#include <chrono>     
#include <sstream>    
#include <iomanip>    
#include <ctime>      

AsyncLogger* AsyncLogger::_instance = nullptr;
std::mutex AsyncLogger::_instanceMutex;

AsyncLogger* AsyncLogger::getInstance() {
    if (_instance == nullptr) {
        std::lock_guard<std::mutex> lock(_instanceMutex);
        if (_instance == nullptr) {
            _instance = new AsyncLogger();
        }
    }
    return _instance;
}

AsyncLogger::AsyncLogger() 
    : _level(INFO), _running(false), _maxQueueSize(1000) {
}

AsyncLogger::~AsyncLogger() {
    stop();
}

void AsyncLogger::init(const std::string& filepath, LogLevel level, size_t maxQueueSize) {
    _filepath = filepath;
    _level = level;
    _maxQueueSize = maxQueueSize;
    
    // 打开日志文件
    _fileStream.open(_filepath, std::ios::out | std::ios::app);
    if (!_fileStream.is_open()) {
        std::cerr << "Failed to open log file: " << _filepath << std::endl;
        return;
    }
    
    _running = true;
    _writeThread = std::thread(&AsyncLogger::writeThreadFunc, this);
}

void AsyncLogger::stop() {
    if (_running) {
        _running = false;
        _cv.notify_all();
        if (_writeThread.joinable()) {
            _writeThread.join();
        }
        
        // 刷新剩余日志
        std::lock_guard<std::mutex> frontLock(_frontMutex);
        while (!_frontBuffer.empty()) {
            writeToFile(_frontBuffer.front());
            _frontBuffer.pop();
        }
        
        if (_fileStream.is_open()) {
            _fileStream.close();
        }
    }
}

std::string AsyncLogger::getCurrentTimeMicros() {
    auto now = std::chrono::system_clock::now();
    auto now_time = std::chrono::system_clock::to_time_t(now);
    
    // 获取微秒部分
    auto now_us = std::chrono::duration_cast<std::chrono::microseconds>(
        now.time_since_epoch()) % 1000000;
    
    std::tm* now_tm = std::localtime(&now_time);
    
    // 格式化时间字符串，精确到微秒
    std::ostringstream time_ss;
    time_ss << std::put_time(now_tm, "%Y-%m-%d %H:%M:%S") 
            << '.' << std::setfill('0') << std::setw(6) << now_us.count();
    
    return time_ss.str();
}

void AsyncLogger::log(const std::string& level, const std::string& message, 
                     const std::string& file_name, int line) {
    // 获取当前时间的微秒级时间戳
    std::string timestamp = getCurrentTimeMicros();
    
    // 构建日志消息
    std::ostringstream log_ss;
    log_ss << "[" << level << "][" << timestamp << "][" 
           << message << "][" << file_name << "][" << line << "]";
    
    std::string logMsg = log_ss.str();
    
    // 将日志添加到前台缓冲区
    {
        std::lock_guard<std::mutex> lock(_frontMutex);
        _frontBuffer.push(logMsg);
        
        // 如果缓冲区满了，通知写入线程
        if (_frontBuffer.size() >= _maxQueueSize) {
            _cv.notify_one();
        }
    }
    
    // 除了写入文件，也输出到控制台（可选）
    std::cout << logMsg << std::endl;
}

void AsyncLogger::writeThreadFunc() {
    while (_running) {
        // 等待通知或超时
        std::unique_lock<std::mutex> lock(_backMutex);
        _cv.wait_for(lock, std::chrono::seconds(1), [this]() {
            return !_running || !_frontBuffer.empty();
        });
        
        // 交换前后台缓冲区
        {
            std::lock_guard<std::mutex> frontLock(_frontMutex);
            std::swap(_frontBuffer, _backBuffer);
        }
        
        // 写入后台缓冲区中的所有日志
        while (!_backBuffer.empty()) {
            writeToFile(_backBuffer.front());
            _backBuffer.pop();
        }
        
        // 刷新文件流
        if (_fileStream.is_open()) {
            _fileStream.flush();
        }
    }
}

void AsyncLogger::writeToFile(const std::string& logMsg) {
    if (_fileStream.is_open()) {
        _fileStream << logMsg << std::endl;
        //使用\n换行效率更高，因为endl会触发一次文件流刷新
        //但使用endl更可靠,避免意外情况导致日志错误
        //_fileStream << logMsg << '\n';
    }
}