

class Request
{
private:
    int _fd;
    
public:
    int _epfd;
    int _cfd;
    Request()=default;
    ~Request();
    Request(int epfd,int cfd);

    void handleRequest();

    void setFd(int fd);
    int getFd();
};