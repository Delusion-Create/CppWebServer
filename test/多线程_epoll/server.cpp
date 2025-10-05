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

#include<sys/epoll.h>

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
        //创建epoll模型
        int epfd=epoll_create(1);
        if(epfd<0) cout<<"epoll create err"<<endl;

        epoll_event ev;
        ev.data.fd=cfd;
        ev.events=EPOLLIN;

        epoll_ctl(epfd,EPOLL_CTL_ADD,cfd,&ev);

        epoll_event evs[1024];
        int elen=sizeof(evs)/sizeof(epoll_event);


        char buffer[1024];
        while(true)
        {
            int num=epoll_wait(epfd,evs,elen,-1);

            for (int i=0;i<num;i++)
            {
                int curfd=evs[i].data.fd;
                if(curfd==cfd)
                {
                    int recv_len=recv(cfd,buffer,sizeof(buffer),0);
                    if(recv_len==0)
                    {
                        cout<<"客户端断开连接"<<endl;
                        //将该文件描述符从检测模型中删除
                        epoll_ctl(epfd,EPOLL_CTL_DEL,curfd,NULL);
                        //先从epoll树上删除再关闭
                        close(curfd);
                        return;
                    }
                    else if(recv_len<0)
                    {
                        cerr<<"recv err"<<endl;
                        //将该文件描述符从检测模型中删除
                        epoll_ctl(epfd,EPOLL_CTL_DEL,curfd,NULL);
                        //先从epoll树上删除再关闭
                        close(curfd);
                        return;
                    }
                    else
                    {
                        cout<<"来自客户端的消息:"<<buffer<<endl;

                        //发送回去
                        if(send(cfd,buffer,strlen(buffer)+1,0)<0){cerr<<"send err"<<endl;break;}
                    }
                    
                }
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
        int epfd;
        epoll_event ev;
        epoll_event evs[1024];
        int elen=sizeof(evs)/sizeof(epoll_event);

         //添加epoll
        epfd=epoll_create(1);
        if(epfd<0) cout<<"epoll create err"<<endl;
        
        ev.data.fd=sfd;
        ev.events=EPOLLIN;

        epoll_ctl(epfd,EPOLL_CTL_ADD,sfd,&ev);

        while (true)
        {
            int num=epoll_wait(epfd,evs,elen,-1);
            cout<<"epoll_wati num="<<num<<endl;
            for (int i=0;i<num;i++)
            {
                int curfd=evs[i].data.fd;
                if(curfd==sfd)
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
            
        }
        
    }
};

int main()
{
    MyServer server("192.168.247.128",8848,5);

    server.run();

}

