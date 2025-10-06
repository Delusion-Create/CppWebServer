

#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<arpa/inet.h>
#include<thread>
#include<chrono>

int main ()
{
    //1.创建客户端插座
    int cfd=socket(AF_INET,SOCK_STREAM,0);
    if (cfd==-1)
    {
        perror("socket创建失败\n");
        exit(0);
    }
    //2.连接服务器
    struct sockaddr_in addr;
    addr.sin_family=AF_INET;//ipv4
    addr.sin_port=htons(8848);//端口，转为网络字节序
    inet_pton(AF_INET,"192.168.247.128",&addr.sin_addr.s_addr);//ip转换为网络字节序
    int ret=connect(cfd,(struct sockaddr*)&addr,sizeof(addr));
    if (ret==-1)
    {
        perror("连接失败\n");
        exit(0);
    }
    //3.通信
    int num=0;
    while (1)
    {
        //发送数据
        char buf[1024];
        sprintf(buf,"这是来自客户端的数据,%d,......",num++);
        send(cfd,buf,strlen(buf)+1,0);
        //接受数据
        memset(buf,0,sizeof(buf));
        int len=recv(cfd,buf,sizeof(buf),0);//没数据时阻塞在这里
        //判断读写情况
        if (len==0)
        {
            printf("服务器端已断开连接\n");
            break;
        }else if(len>0)
        {
            printf ("收到了服务端的回复，内容为: %s\n",buf);

            int time_wait=35;
            printf("%d秒后在发送消息\n",time_wait);
            std::this_thread::sleep_for(std::chrono::seconds(time_wait));
        }
        else
        {
            perror("接受数据失败\n");
            break;
        }
        sleep(1);

    }

    
    //6.断开连接
    close(cfd);//通信编号
    return 0;
}

