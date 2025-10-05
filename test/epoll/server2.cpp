#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<arpa/inet.h>
#include<sys/epoll.h>
int main ()
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
    //4.创建epoll模型
    int epfd = epoll_create(100);//随便写个数就行，只要大于0
    if (epfd == -1)
    {
        perror("创建epoll模型失败\n");
        exit(0);
    }
    //5.将要检测的节点添加到epoll模型中
    struct epoll_event ev;
    ev.events = EPOLLIN;  //检测lfd的读缓存区，所以设置为读模式
    ev.data.fd=lfd;       //要检测的文件描述符
    //第一个参数表示epoll模型的根节点，第二个参数表示添加节点
    //第三个：要检测的文件描述符,第四个:节点信息，包含了一个文件描述符和它的检测模式
    ret = epoll_ctl(epfd,EPOLL_CTL_ADD,lfd,&ev);
    if (ret == -1)
    {
        perror("操作epoll模型失败\n");
        exit(0);
    }
    //5.不停地委托内核检测epoll模型中的文件描述符状态
    //将检测到的节点信息保存到这个数组中
    struct epoll_event evs[1024];
    int size = sizeof(evs)/sizeof(evs[0]);
    while(1)
    {
        //第二个参数为传出参数，存着将检测到的节点信息,size表示这个数组的大小
        //-1将该函数设置为默认阻塞，除非检测到了有文件描述符变化,num返回有多少文符状态发生了变化
        int num=epoll_wait(epfd,evs,size,-1);
        printf("num = %d\n",num);
        //遍历evs数组,个数就是返回值
        for (int i=0;i<num;++i)
        {
            //从当前节点信息中取出文件描述符
            int curfd=evs[i].data.fd;
            //如果lfd在evs的数组中存在，说明lfd的读缓存区发生了变化,也就是有新的连接
            if(curfd==lfd)
            {
                //建立新连接,这次调用绝对不阻塞
                int cfd= accept(lfd,NULL,NULL); //不需要客户端信息，后两个参数写NULL
                //得到了与客户端通信的文件描述符，将其添加到epoll模型中
                ev.events=EPOLLIN;
                ev.data.fd=cfd;
                //为什么这里可以用之前的ev的地址？因为之前是从ev的地址拷贝ev的值到epoll模型中
                //所以继续在这个地址上操作是不会影响到模型的,本质是赋值拷贝
                epoll_ctl(epfd,EPOLL_CTL_ADD,cfd,&ev);
            }
            else
            {
                //剩下的都是用于通信文件描述符
                //直接读数据，肯定不阻塞
                char buf[1024];
                memset(buf,0,sizeof(buf));
                int len=recv(curfd,buf,sizeof(buf),0);//没数据时阻塞在这里
                //判断读写情况
                if (len==0)
                {
                 printf("客户端已断开连接\n");
                 //将该文件描述符从检测模型中删除
                 epoll_ctl(epfd,EPOLL_CTL_DEL,curfd,NULL);
                 //先从epoll树上删除再关闭
                 close(curfd);
                }else if(len>0)
                {
                    printf ("recv buf: %s\n",buf);
                    //回复数据
                    send(curfd,buf,strlen(buf)+1,0);//+1是为了带上\0
                }
                else
                {
                    perror("接受数据失败\n");
                    break;
                }
                    
                
            }
        }

       
    }
    //6.断开连接
    close(lfd);//监听编号
    return 0;
}
