#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_

#include<vector>
#include<queue>

#include<thread>
#include<mutex>
#include<condition_variable>
#include<functional>

class ThreadPool
{
private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex lock_task;
    std::condition_variable cv;
    bool stop;

    size_t _num_threads;

    
public:
    ThreadPool(size_t num_threads);
    ~ThreadPool();

    void startThreadPool();
    void addTask(std::function<void()> task);
};


#endif