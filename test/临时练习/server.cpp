#include<iostream>
#include<string>
#include<cstring>
#include<unistd.h>
#include<arpa/inet.h>

#include<vector>
#include<queue>

#include<mutex>
#include<condition_variable>
#include<thread>
#include<functional>

using namespace std;


class Server
{
private:
    vector<thread> workers;
    queue<function<void()>> tasks;
    mutex lock_mutex;
    condition_variable cv;
    bool stop;

    int sfd;

    void addTask(function<void()> task)
    {
        {
            unique_lock<mutex> lock(lock_mutex);
            tasks.push(task);
        }
        cv.notify_one();
    }
    void startThreadPool(size_t numThreads)
    {
        for (int i=0;i<numThreads;i++)
        {
            workers.emplace_back([this]{
                while (true)
                {
                    function<void()> task;
                    {
                        unique_lock<mutex> lock(lock_mutex);
                        cv.wait(lock,[this]{
                            return stop||!tasks.empty();
                        });

                        if(stop&&tasks.empty()) return;

                        task=move(tasks.front());
                        tasks.pop();

                    }
                    task();
                }
                
            });
        }
    }
    void handleClient(int cfd)
    {
        char buffer[1024];
        while(true)
        {
            int recv_len=recv(cfd,buffer,sizeof(buffer),0);
            if(recv_len<0)
            {
                if(errno==EAGAIN) continue;
                else{cerr<<"recv_len err"<<endl;close(cfd);return;}
            }
            else if(recv_len==0){cerr<<"client lost connect"<<endl;close(cfd);return;}
            else
            {
                cout<<"recv: "<<buffer<<endl;
                if(send(cfd,buffer,sizeof(buffer),0)<0) cerr<<"send err"<<endl;
            }
        }
    }


public:
    Server(size_t numThreads):stop(false)
    {
        sfd=socket(AF_INET,SOCK_STREAM,0);

        sockaddr_in sin;
        sin.sin_port=htons(8848);
        sin.sin_family=AF_INET;
        sin.sin_addr.s_addr=inet_addr("192.168.247.128");

        if(bind(sfd,(sockaddr*)&sin,sizeof(sin))<0) cerr<<"bind err"<<endl;

        listen(sfd,numThreads);

        startThreadPool(numThreads);
    }
    ~Server()
    {
        {
            unique_lock<mutex> lock(lock_mutex);
            stop=true;
        }
        cv.notify_all();
        for(auto& worker:workers)
        {
            worker.join();
        }

        close(sfd);
    }
    void run()
    {
        sockaddr_in cin;
        socklen_t  clen=sizeof(cin);
        while (true)
        {
            int cfd=accept(sfd,(sockaddr*)&cin,&clen);
            if(cfd<0) cerr<<"accept err"<<endl;

            addTask([this,cfd]{
                this->handleClient(cfd);
            });

        }
        
    }

};

int main()
{
    Server server(5);
    server.run();
}