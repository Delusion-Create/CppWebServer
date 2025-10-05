#ifndef REQUESTDATA
#define REQUESTDATA
#include <string>
#include <unordered_map>
#include <sys/epoll.h>

/**
 * @brief HTTP请求方法枚举定义
 * @details 定义支持的HTTP请求方法类型
 */
enum METHOD {
    METHOD_GET = 0,    ///< GET请求方法，用于获取资源
    METHOD_POST       ///< POST请求方法，用于提交数据
};

/**
 * @brief HTTP协议版本枚举
 * @details 定义支持的HTTP协议版本
 */
enum HTTPVERSION {
    HTTP_10 = 0,      ///< HTTP/1.0版本
    HTTP_11          ///< HTTP/1.1版本，支持持久连接等特性
};

/**
 * @brief 请求处理状态枚举
 * @details 定义HTTP请求处理的状态机状态
 */
enum STATE {
    STATE_PARSE_URI = 0,    ///< 正在解析请求行（URI）
    STATE_PARSE_HEADERS,     ///< 正在解析请求头部  
    STATE_RECV_BODY,         ///< 正在接收请求体（POST请求）
    STATE_ANALYSIS,          ///< 正在分析请求并生成响应
    STATE_FINISH            ///< 请求处理完成
};

/**
 * @brief HTTP请求数据处理类
 * @details 负责解析HTTP请求、生成响应、管理连接生命周期
 * 该类使用状态机模式逐步处理HTTP请求，支持GET和POST方法
 */
struct requestData {
private:
    // === 核心数据成员 ===
    int againTimes;          ///< EAGAIN错误重试计数器，防止无限重试
    std::string path;        ///< 请求的资源路径
    int fd;                  ///< 客户端连接的文件描述符
    int epollfd;            ///< 所属epoll实例的文件描述符
    
    // 请求内容相关
    std::string content;    ///< 存储接收到的原始HTTP请求数据
    int method;             ///< 请求方法（GET/POST）
    int HTTPversion;        ///< HTTP协议版本（1.0/1.1）
    std::string file_name;  ///< 请求的文件名（从URI中解析得到）
    
    // 解析状态跟踪
    int now_read_pos;       ///< 当前读取位置（在content中的偏移量）
    int state;              ///< 当前请求处理状态（见STATE枚举）
    int h_state;            ///< 头部解析状态机状态
    bool isfinish;          ///< 请求是否处理完成标志
    bool keep_alive;        ///< 是否保持连接（HTTP Keep-Alive）
    
    // HTTP头部存储
    std::unordered_map<std::string, std::string> headers;  ///< 解析出的HTTP头部键值对
    
    // 定时器关联
    mytimer *timer;         ///< 关联的超时定时器，用于请求超时管理

private:
    // === 私有处理函数 ===
    
    /**
     * @brief URI解析函数
     * @return 解析结果：PARSE_URI_SUCCESS/AGAIN/ERROR
     * @details 从请求行中解析方法、路径、版本等信息
     */
    int parse_URI();
    
    /**
     * @brief HTTP头部解析函数
     * @return 解析结果：PARSE_HEADER_SUCCESS/AGAIN/ERROR  
     * @details 使用精细状态机解析HTTP请求头部
     */
    int parse_Headers();
    
    /**
     * @brief 请求分析函数
     * @return 分析结果：ANALYSIS_SUCCESS/ERROR
     * @details 根据请求类型（GET/POST）生成相应的HTTP响应
     */
    int analysisRequest();

public:
    // === 构造函数和析构函数 ===
    
    /**
     * @brief 默认构造函数
     * @details 初始化所有成员变量为默认值
     */
    requestData();
    
    /**
     * @brief 参数化构造函数
     * @param _epollfd epoll实例文件描述符
     * @param _fd 客户端连接文件描述符  
     * @param _path 请求路径
     */
    requestData(int _epollfd, int _fd, std::string _path);
    
    /**
     * @brief 析构函数
     * @details 清理资源，关闭连接，移除定时器关联
     */
    ~requestData();

    // === 定时器管理方法 ===
    
    /**
     * @brief 添加定时器关联
     * @param mtimer 要关联的定时器指针
     */
    void addTimer(mytimer *mtimer);
    
    /**
     * @brief 分离定时器（解除关联）
     * @details 在定时器超时前手动分离，避免重复删除
     */
    void seperateTimer();

    // === 连接管理方法 ===
    
    /**
     * @brief 获取文件描述符
     * @return 当前关联的文件描述符
     */
    int getFd();
    
    /**
     * @brief 设置文件描述符
     * @param _fd 新的文件描述符
     */
    void setFd(int _fd);
    
    /**
     * @brief 重置请求状态
     * @details 用于Keep-Alive连接复用，重置解析状态但保留连接
     */
    void reset();

    // === 核心业务方法 ===
    
    /**
     * @brief 主请求处理函数
     * @details 协调整个请求处理流程，按状态机顺序处理HTTP请求
     */
    void handleRequest();
    
    /**
     * @brief 错误处理函数
     * @param fd 发生错误的文件描述符
     * @param err_num HTTP错误码（如404、500等）
     * @param short_msg 错误简短描述
     * @details 生成标准的HTTP错误响应页面
     */
    void handleError(int fd, int err_num, std::string short_msg);
};

#endif