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

class MyServer
{
private:
    int sfd;

    vector<thread> workers;
    queue<function<void()>> tasks;
    mutex lock_task;
    condition_variable cv;
    bool stop;


    void startThreadPool(size_t numThread)
    {
        for(int i=0;i<numThread;i++)
        {
            workers.emplace_back([this]{
                while(true)
                {
                    function<void()> task;
                    {
                        unique_lock<mutex> lock(lock_task);
                        cv.wait(lock,[this](){
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

    void addTask(function<void()> task)
    {
        {
            unique_lock<mutex> lock(lock_task);
            tasks.push(task);
        }
        cv.notify_one();
    }

    void handleClient(int cfd,sockaddr_in cin)
    {
        char buffer[1024];
        while(true)
        {
            int recv_len=recv(cfd,buffer,sizeof(buffer),0);
            if(recv_len==0){cout<<"客户端断开连接"<<endl;close(cfd);break;}
            else if(recv_len<0){cerr<<"recv err"<<endl;close(cfd);break;}
            else
            {
                cout<<"来自客户端的消息:"<<buffer<<endl;

                //发送回去
                if(send(cfd,buffer,sizeof(buffer),0)<0){cerr<<"send err"<<endl;break;}
            }
        }
    }

public:
    MyServer(const string& ip,int port,size_t numThread):stop(false)
    {
        sfd=socket(AF_INET,SOCK_STREAM,0);

        sockaddr_in sin;
        sin.sin_family=AF_INET;
        sin.sin_port=htons(port);
        sin.sin_addr.s_addr=inet_addr(ip.c_str());

        if(bind(sfd,(sockaddr*)&sin,sizeof(sin))<0){cerr<<"bind err"<<endl;return;}
        if(listen(sfd,20)<0){cerr<<"listen err"<<endl;return;}

        startThreadPool(numThread);

        cout<<"server start on "<<ip<<": "<<port<<endl;

    }
    ~MyServer()
    {
        {
            unique_lock<mutex> lock(lock_task);
            stop=true;
        }
        cv.notify_all();
        for (auto& worker: workers)
        {
            worker.join();
        }

        close(sfd);
    }
    void run()
    {
        
        

        while (true)
        {
            sockaddr_in cin;
            socklen_t clen=sizeof(cin);

            int cfd=accept(sfd,(sockaddr*)&cin,&clen);
            if(cfd<0){cerr<<"accpet err"<<endl; continue;}

            addTask([this,cfd,cin](){
                this->handleClient(cfd,cin);
            });
            
        }
        
    }
};

int main()
{
    MyServer server("192.168.247.128",8848,3);

    server.run();

}

