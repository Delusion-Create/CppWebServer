#include<iostream>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<arpa/inet.h>
#include<sys/epoll.h>

using namespace std;


int main()
{
    //1.创建服务器插座
    int lfd=socket(AF_INET,SOCK_STREAM,0);
    if (lfd==-1)
    {
        perror("socket创建失败\n");
        exit(0);
    }
    //2.绑定
    struct sockaddr_in addr;
    addr.sin_family=AF_INET;//ipv4
    addr.sin_port=htons(8848);//端口，转为网络字节序
    addr.sin_addr.s_addr=INADDR_ANY;//0地址
    int ret=bind(lfd,(struct sockaddr*)&addr,sizeof(addr));
    if (ret==-1)
    {
        perror("绑定失败\n");
        exit(0);
    }
    //3.设置监听
    ret=listen(lfd,128);
    if (ret==-1)
    {
        perror("监听失败\n");
        exit(0);
    }

    int epfd=epoll_create(1);
    if(epfd<0) cout<<"epoll create err"<<endl;

    epoll_event ev;
    ev.data.fd=lfd;
    ev.events=EPOLLIN;

    epoll_ctl(epfd,EPOLL_CTL_ADD,lfd,&ev);

    epoll_event evs[1024];
    int elen=sizeof(evs)/sizeof(epoll_event);

    while (true)
    {
        int num=epoll_wait(epfd,evs,elen,-1);
        cout<<"num="<<num<<endl;

        for (int i=0;i<num;i++)
        {
            int curfd=evs[i].data.fd;
            if(curfd==lfd)
            {
                int cfd=accept(lfd,NULL,NULL);
                if(cfd<0){cout<<"accept err"<<endl;continue;}

                ev.data.fd=cfd;
                ev.events=EPOLLIN;
                epoll_ctl(epfd,EPOLL_CTL_ADD,cfd,&ev);
                
            }
            else
            {
                char buffer[1024];
                int recv_len=recv(curfd,buffer,sizeof(buffer),0);
                if(recv_len==0)
                {
                    cout<<"客户端断开连接"<<endl;
                    epoll_ctl(epfd,EPOLL_CTL_DEL,curfd,NULL);
                    close(curfd);
                }
                else if(recv_len<0){cout<<"recv err"<<endl;break;}
                else
                {
                    cout<<"来自客户端的数据:"<<buffer<<endl;
                    if(send(curfd,buffer,strlen(buffer)+1,0)<0){cout<<"send err"<<endl;break;}
                    
                }
            }
        }
    }
    
}