#include "requestData.h"
#include "util.h"
#include "epoll.h"
#include <sys/epoll.h>
#include <unistd.h>
#include <sys/time.h>
#include <unordered_map>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <queue>
#include <iostream>

using namespace std;

// 全局变量定义
pthread_mutex_t MimeType::lock = PTHREAD_MUTEX_INITIALIZER;
std::unordered_map<std::string, std::string> MimeType::mime;

// 全局定时器优先队列
priority_queue<mytimer*, deque<mytimer*>, timerCmp> myTimerQueue;
pthread_mutex_t qlock = PTHREAD_MUTEX_INITIALIZER;

// ==================== MimeType 类方法实现 ====================

/**
 * @brief 获取文件后缀对应的MIME类型
 * @param suffix 文件后缀名（如".html"、".jpg"）
 * @return 对应的MIME类型字符串
 * @note 采用懒加载模式，首次调用时初始化MIME类型映射表
 * @threadsafe 使用互斥锁保证线程安全
 */
std::string MimeType::getMime(const std::string &suffix)
{
    if (mime.size() == 0)
    {
        pthread_mutex_lock(&lock);
        if (mime.size() == 0)
        {
            // 初始化常见的MIME类型映射
            mime[".html"] = "text/html";
            mime[".avi"] = "video/x-msvideo";
            mime[".bmp"] = "image/bmp";
            mime[".c"] = "text/plain";
            mime[".doc"] = "application/msword";
            mime[".gif"] = "image/gif";
            mime[".gz"] = "application/x-gzip";
            mime[".htm"] = "text/html";
            mime[".ico"] = "application/x-ico";
            mime[".jpg"] = "image/jpeg";
            mime[".png"] = "image/png";
            mime[".txt"] = "text/plain";
            mime[".mp3"] = "audio/mp3";
            mime["default"] = "text/html";
        }
        pthread_mutex_unlock(&lock);
    }
    
    if (mime.find(suffix) == mime.end())
        return mime["default"];
    else
        return mime[suffix];
}

// ==================== requestData 类方法实现 ====================

/**
 * @brief 默认构造函数
 * @details 初始化所有成员变量为安全默认值，创建空的请求对象
 */
requestData::requestData(): 
    againTimes(0), now_read_pos(0), state(STATE_PARSE_URI), h_state(h_start), 
    keep_alive(false), timer(NULL), fd(-1), epollfd(-1), method(METHOD_GET),
    HTTPversion(HTTP_11), isfinish(false)
{
    cout << "requestData default constructed!" << endl;
}

/**
 * @brief 参数化构造函数
 * @param _epollfd 所属epoll实例的文件描述符
 * @param _fd 客户端连接的文件描述符
 * @param _path 请求的路径信息
 * @details 使用传入参数初始化请求对象，准备处理具体请求
 */
requestData::requestData(int _epollfd, int _fd, std::string _path):
    againTimes(0), path(_path), fd(_fd), epollfd(_epollfd),
    now_read_pos(0), state(STATE_PARSE_URI), h_state(h_start), 
    keep_alive(false), timer(NULL), method(METHOD_GET),
    HTTPversion(HTTP_11), isfinish(false)
{
    // 构造函数体可以添加额外的初始化逻辑
}

/**
 * @brief 析构函数
 * @details 负责清理所有资源，包括从epoll移除监控、处理定时器关联、关闭连接等
 * @warning 确保不会出现资源泄漏和重复释放
 */
requestData::~requestData()
{
    cout << "~requestData(): cleaning up resources for fd " << fd << endl;
    
    // 从epoll中移除监控
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
    ev.data.ptr = (void*)this;
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, &ev);
    
    // 处理定时器关联
    if (timer != NULL) {
        timer->clearReq();
        timer = NULL;
    }
    
    // 关闭连接
    if (fd != -1) {
        close(fd);
    }
}

/**
 * @brief 添加定时器关联
 * @param mtimer 要关联的定时器指针
 * @details 建立请求对象与定时器的双向关联，用于超时管理
 */
void requestData::addTimer(mytimer *mtimer)
{
    if (timer == NULL) {
        timer = mtimer;
    }
}

/**
 * @brief 分离定时器关联
 * @details 解除请求对象与定时器的关联，通常在正常完成请求时调用
 */
void requestData::seperateTimer()
{
    if (timer) {
        timer->clearReq();
        timer = NULL;
    }
}

/**
 * @brief 获取文件描述符
 * @return 当前关联的文件描述符
 */
int requestData::getFd()
{
    return fd;
}

/**
 * @brief 设置文件描述符
 * @param _fd 新的文件描述符
 * @details 用于连接重用或异常恢复场景
 */
void requestData::setFd(int _fd)
{
    fd = _fd;
}

/**
 * @brief 重置请求状态
 * @details 用于Keep-Alive连接复用，重置所有解析状态但保留连接
 * @note 保持连接打开，准备处理下一个请求
 */
void requestData::reset()
{
    againTimes = 0;
    content.clear();
    file_name.clear();
    path.clear();
    now_read_pos = 0;
    state = STATE_PARSE_URI;
    h_state = h_start;
    headers.clear();
    // 注意：keep_alive标志保持不变，timer需要外部重新设置
}

/**
 * @brief URI解析方法
 * @return 解析结果状态码
 * @retval PARSE_URI_SUCCESS URI解析成功
 * @retval PARSE_URI_AGAIN 需要更多数据继续解析
 * @retval PARSE_URI_ERROR URI格式错误或不受支持
 * @details 解析HTTP请求行，提取请求方法、路径、文件名和HTTP版本信息
 */
int requestData::parse_URI()
{
    string &str = content;
    
    // 查找请求行结束标志（CRLF）
    int pos = str.find('\r', now_read_pos);
    if (pos < 0) {
        return PARSE_URI_AGAIN;
    }
    
    // 提取完整的请求行并更新内容缓冲区
    string request_line = str.substr(0, pos);
    if (str.size() > pos + 1) {
        str = str.substr(pos + 1);
    } else {
        str.clear();
    }

    // 解析请求方法（GET/POST）
    pos = request_line.find("GET");
    if (pos < 0) {
        pos = request_line.find("POST");
        if (pos < 0) {
            return PARSE_URI_ERROR;
        } else {
            method = METHOD_POST;
        }
    } else {
        method = METHOD_GET;
    }

    // 解析请求路径和文件名
    pos = request_line.find("/", pos);
    if (pos < 0) {
        return PARSE_URI_ERROR;
    } else {
        int _pos = request_line.find(' ', pos);
        if (_pos < 0) {
            return PARSE_URI_ERROR;
        } else {
            if (_pos - pos > 1) {
                file_name = request_line.substr(pos + 1, _pos - pos - 1);
                // 处理查询参数（如果有）
                int __pos = file_name.find('?');
                if (__pos >= 0) {
                    file_name = file_name.substr(0, __pos);
                }
            } else {
                file_name = "index.html";
            }
        }
        pos = _pos;
    }

    // 解析HTTP版本
    pos = request_line.find("/", pos);
    if (pos < 0) {
        return PARSE_URI_ERROR;
    } else {
        if (request_line.size() - pos <= 3) {
            return PARSE_URI_ERROR;
        } else {
            string ver = request_line.substr(pos + 1, 3);
            if (ver == "1.0") {
                HTTPversion = HTTP_10;
            } else if (ver == "1.1") {
                HTTPversion = HTTP_11;
            } else {
                return PARSE_URI_ERROR;
            }
        }
    }
    
    state = STATE_PARSE_HEADERS;
    return PARSE_URI_SUCCESS;
}

/**
 * @brief HTTP头部解析方法
 * @return 解析结果状态码
 * @retval PARSE_HEADER_SUCCESS 头部解析成功
 * @retval PARSE_HEADER_AGAIN 需要更多数据继续解析
 * @retval PARSE_HEADER_ERROR 头部格式错误
 * @details 使用精细状态机逐字符解析HTTP请求头部，支持Keep-Alive等关键头部
 */
int requestData::parse_Headers()
{
    string &str = content;
    int key_start = -1, key_end = -1, value_start = -1, value_end = -1;
    int now_read_line_begin = 0;
    bool notFinish = true;
    
    // 使用状态机逐个字符解析头部
    for (int i = 0; i < str.size() && notFinish; ++i) {
        switch(h_state) {
            case h_start:
                if (str[i] == '\n' || str[i] == '\r') break;
                h_state = h_key;
                key_start = i;
                now_read_line_begin = i;
                break;
                
            case h_key:
                if (str[i] == ':') {
                    key_end = i;
                    if (key_end - key_start <= 0) return PARSE_HEADER_ERROR;
                    h_state = h_colon;
                } else if (str[i] == '\n' || str[i] == '\r') {
                    return PARSE_HEADER_ERROR;
                }
                break;
                
            case h_colon:
                if (str[i] == ' ') {
                    h_state = h_spaces_after_colon;
                } else {
                    return PARSE_HEADER_ERROR;
                }
                break;
                
            case h_spaces_after_colon:
                h_state = h_value;
                value_start = i;
                break;
                
            case h_value:
                if (str[i] == '\r') {
                    h_state = h_CR;
                    value_end = i;
                    if (value_end - value_start <= 0) return PARSE_HEADER_ERROR;
                } else if (i - value_start > 255) {
                    return PARSE_HEADER_ERROR;
                }
                break;
                
            case h_CR:
                if (str[i] == '\n') {
                    h_state = h_LF;
                    // 提取完整的键值对并存入headers映射
                    string key(str.begin() + key_start, str.begin() + key_end);
                    string value(str.begin() + value_start, str.begin() + value_end);
                    headers[key] = value;
                    now_read_line_begin = i;
                } else {
                    return PARSE_HEADER_ERROR;
                }
                break;
                
            case h_LF:
                if (str[i] == '\r') {
                    h_state = h_end_CR;
                } else {
                    key_start = i;
                    h_state = h_key;
                }
                break;
                
            case h_end_CR:
                if (str[i] == '\n') {
                    h_state = h_end_LF;
                } else {
                    return PARSE_HEADER_ERROR;
                }
                break;
                
            case h_end_LF:
                notFinish = false;
                key_start = i;
                now_read_line_begin = i;
                break;
        }
    }
    
    if (h_state == h_end_LF) {
        str = str.substr(now_read_line_begin);
        return PARSE_HEADER_SUCCESS;
    }
    str = str.substr(now_read_line_begin);
    return PARSE_HEADER_AGAIN;
}

/**
 * @brief 请求分析方法
 * @return 处理结果状态码
 * @retval ANALYSIS_SUCCESS 请求分析成功并生成响应
 * @retval ANALYSIS_ERROR 请求分析失败
 * @details 根据请求方法（GET/POST）生成相应的HTTP响应，支持文件传输和动态响应
 */
int requestData::analysisRequest()
{
    // POST请求处理
    if (method == METHOD_POST) {
        char header[MAX_BUFF];
        sprintf(header, "HTTP/1.1 %d %s\r\n", 200, "OK");
        
        // 检查并处理Keep-Alive连接
        if(headers.find("Connection") != headers.end() && headers["Connection"] == "keep-alive") {
            keep_alive = true;
            sprintf(header, "%sConnection: keep-alive\r\n", header);
            sprintf(header, "%sKeep-Alive: timeout=%d\r\n", header, EPOLL_WAIT_TIME);
        }
        
        // 生成响应内容
        char *send_content = "I have received this.";
        sprintf(header, "%sContent-length: %zu\r\n", header, strlen(send_content));
        sprintf(header, "%s\r\n", header);
        
        // 发送HTTP头部
        size_t send_len = (size_t)writen(fd, header, strlen(header));
        if(send_len != strlen(header)) {
            perror("Send header failed");
            return ANALYSIS_ERROR;
        }
        
        // 发送响应内容
        send_len = (size_t)writen(fd, send_content, strlen(send_content));
        if(send_len != strlen(send_content)) {
            perror("Send content failed");
            return ANALYSIS_ERROR;
        }
        
        return ANALYSIS_SUCCESS;
    } 
    // GET请求处理
    else if (method == METHOD_GET) {
        char header[MAX_BUFF];
        sprintf(header, "HTTP/1.1 %d %s\r\n", 200, "OK");
        
        // 检查并处理Keep-Alive连接
        if(headers.find("Connection") != headers.end() && headers["Connection"] == "keep-alive") {
            keep_alive = true;
            sprintf(header, "%sConnection: keep-alive\r\n", header);
            sprintf(header, "%sKeep-Alive: timeout=%d\r\n", header, EPOLL_WAIT_TIME);
        }
        
        // 获取文件MIME类型
        int dot_pos = file_name.find('.');
        const char* filetype;
        if (dot_pos < 0) {
            filetype = MimeType::getMime("default").c_str();
        } else {
            filetype = MimeType::getMime(file_name.substr(dot_pos)).c_str();
        }
        
        // 检查文件是否存在并获取文件信息
        struct stat sbuf;
        if (stat(file_name.c_str(), &sbuf) < 0) {
            handleError(fd, 404, "Not Found!");
            return ANALYSIS_ERROR;
        }

        // 构建完整的HTTP响应头部
        sprintf(header, "%sContent-type: %s\r\n", header, filetype);
        sprintf(header, "%sContent-length: %ld\r\n", header, sbuf.st_size);
        sprintf(header, "%s\r\n", header);
        
        // 发送HTTP头部
        size_t send_len = (size_t)writen(fd, header, strlen(header));
        if(send_len != strlen(header)) {
            perror("Send header failed");
            return ANALYSIS_ERROR;
        }
        
        // 使用mmap零拷贝发送文件内容
        int src_fd = open(file_name.c_str(), O_RDONLY, 0);
        char *src_addr = static_cast<char*>(mmap(NULL, sbuf.st_size, PROT_READ, MAP_PRIVATE, src_fd, 0));
        close(src_fd);
        
        send_len = writen(fd, src_addr, sbuf.st_size);
        if(send_len != sbuf.st_size) {
            perror("Send file failed");
            munmap(src_addr, sbuf.st_size);
            return ANALYSIS_ERROR;
        }
        munmap(src_addr, sbuf.st_size);
        return ANALYSIS_SUCCESS;
    } else {
        return ANALYSIS_ERROR;
    }
}

/**
 * @brief HTTP错误响应生成方法
 * @param fd 发生错误的连接文件描述符
 * @param err_num HTTP错误状态码（如404、500等）
 * @param short_msg 错误简短描述信息
 * @details 生成完整的HTTP错误响应页面，包括标准的HTML错误页面
 */
void requestData::handleError(int fd, int err_num, string short_msg)
{
    short_msg = " " + short_msg;
    char send_buff[MAX_BUFF];
    string body_buff, header_buff;
    
    // 构建HTML错误页面主体
    body_buff += "<html><title>TKeed Error</title>";
    body_buff += "<body bgcolor=\"ffffff\">";
    body_buff += to_string(err_num) + short_msg;
    body_buff += "<hr><em> LinYa's Web Server</em>\n</body></html>";

    // 构建HTTP错误响应头部
    header_buff += "HTTP/1.1 " + to_string(err_num) + short_msg + "\r\n";
    header_buff += "Content-type: text/html\r\n";
    header_buff += "Connection: close\r\n";
    header_buff += "Content-length: " + to_string(body_buff.size()) + "\r\n";
    header_buff += "\r\n";
    
    // 发送错误响应头部和主体
    sprintf(send_buff, "%s", header_buff.c_str());
    writen(fd, send_buff, strlen(send_buff));
    sprintf(send_buff, "%s", body_buff.c_str());
    writen(fd, send_buff, strlen(send_buff));
}

/**
 * @brief 主请求处理协调方法
 * @details 协调整个HTTP请求处理流程的状态机，包括读取数据、解析、分析和响应生成
 * @note 这是整个请求处理的核心入口点，采用非阻塞式渐进处理模式
 */
void requestData::handleRequest()
{
    char buff[MAX_BUFF];
    bool isError = false;
    
    // 主处理循环：按照状态机顺序逐步处理请求
    while (true) {
        // 从连接读取数据
        int read_num = readn(fd, buff, MAX_BUFF);
        
        if (read_num < 0) {
            perror("read error");
            isError = true;
            break;
        } else if (read_num == 0) {
            // 处理连接关闭或数据结束情况
            if (errno == EAGAIN) {
                if (againTimes > AGAIN_MAX_TIMES) {
                    isError = true;
                } else {
                    ++againTimes;
                }
            } else if (errno != 0) {
                isError = true;
            }
            break;
        }
        
        // 将新读取的数据追加到内容缓冲区
        string now_read(buff, buff + read_num);
        content += now_read;

        // === 状态机处理流程 ===
        
        // 状态1: 解析URI（请求行）
        if (state == STATE_PARSE_URI) {
            int flag = this->parse_URI();
            if (flag == PARSE_URI_AGAIN) {
                break;
            } else if (flag == PARSE_URI_ERROR) {
                perror("URI parse error");
                isError = true;
                break;
            }
        }
        
        // 状态2: 解析头部
        if (state == STATE_PARSE_HEADERS) {
            int flag = this->parse_Headers();
            if (flag == PARSE_HEADER_AGAIN) {
                break;
            } else if (flag == PARSE_HEADER_ERROR) {
                perror("header parse error");
                isError = true;
                break;
            }
            
            // 根据请求方法决定下一步状态
            if(method == METHOD_POST) {
                // 检查POST请求必须包含Content-Length头部
                if (headers.find("Content-length") != headers.end()) {
                    state = STATE_RECV_BODY;
                } else {
                    isError = true;
                    break;
                }
            } else {
                state = STATE_ANALYSIS;
            }
        }
        
        // 状态3: 接收请求体（POST请求专用）
        if (state == STATE_RECV_BODY) {
            int content_length = -1;
            if (headers.find("Content-length") != headers.end()) {
                content_length = stoi(headers["Content-length"]);
            } else {
                isError = true;
                break;
            }
            
            // 检查是否已接收完整请求体
            if (content.size() < content_length) {
                continue;  // 需要继续读取数据
            }
            state = STATE_ANALYSIS;
        }
        
        // 状态4: 分析请求并生成响应
        if (state == STATE_ANALYSIS) {
            int flag = this->analysisRequest();
            if (flag < 0) {
                isError = true;
                break;
            } else if (flag == ANALYSIS_SUCCESS) {
                state = STATE_FINISH;
                break;
            } else {
                isError = true;
                break;
            }
        }
    }

    // === 后续处理和资源管理 ===
    if (isError) {
        // 发生错误，清理资源
        delete this;
        return;
    }
    
    // 成功处理完成
    if (state == STATE_FINISH) {
        if (keep_alive) {
            // Keep-Alive连接，重置状态准备处理下一个请求
            printf("Keep-Alive connection maintained\n");
            this->reset();
        } else {
            // 非Keep-Alive连接，关闭资源
            delete this;
            return;
        }
    }
    
    // 重新注册到epoll继续监控后续事件
    __uint32_t _epo_event = EPOLLIN | EPOLLET | EPOLLONESHOT;
    int ret = epoll_mod(epollfd, fd, static_cast<void*>(this), _epo_event);
    if (ret < 0) {
        delete this;
        return;
    }
    
    // 添加或更新定时器
    pthread_mutex_lock(&qlock);
    if (timer != NULL) {
        timer->update(500);  // 更新超时时间
    } else {
        timer = new mytimer(this, 500);
        myTimerQueue.push(timer);
    }
    pthread_mutex_unlock(&qlock);
}