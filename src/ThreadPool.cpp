#include"ThreadPool.h"
#include<iostream>

using namespace std;

ThreadPool::ThreadPool(size_t num_threads):stop(false),_num_threads(num_threads)
{
    cout << "线程池初始化，线程数量: " << _num_threads << endl;
}
ThreadPool::~ThreadPool()
{
    {
        unique_lock<mutex> lock(lock_task);
        stop=true;
    }

    cv.notify_all();
    cout << "线程池析构，实际回收线程数量: " << workers.size() << endl;
    for(auto&  work:workers)
    {
        cout<<"线程:"<<work.get_id()<<"已回收"<<endl;
        work.join();
    }

}

void ThreadPool::startThreadPool()
{
    for (size_t i=0;i<_num_threads;i++)
    {
        workers.emplace_back([this](){
           while(true)
           {
            function<void()> task;
            {
                unique_lock<mutex> lock(lock_task);
                cv.wait(lock,[this](){
                    return stop||!tasks.empty();
                });

                if(stop&&tasks.empty())
                {
                    return;
                }

                task=move(tasks.front());
                tasks.pop();
            }
            task();
           } 
        });
    }
}


void ThreadPool::addTask(function<void()> task)
{
    {
        unique_lock<mutex> lock(lock_task);
        tasks.push(task);
    }
    cv.notify_one();
}