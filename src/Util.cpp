#include "Util.h"
#include <cstring>
#include <signal.h>
#include <unistd.h> // 用于 write 系统调用
#include <iostream>



void Util::handle_for_sigpipe()
{
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = SIG_IGN; // 设置信号处理方式为忽略

    // 初始化 sa_mask，确保不阻塞任何额外信号
    if (sigemptyset(&sa.sa_mask) == -1) {
        perror("sigemptyset failed");
        return;
    }

    // 设置标志，尤其包含 SA_RESTART 以使被中断的系统调用自动重启
    sa.sa_flags = SA_RESTART;

    // 调用 sigaction，并正确检查返回值（成功返回0，失败返回-1）
    if (sigaction(SIGPIPE, &sa, NULL) == -1) {
        // 使用 perror 输出错误信息，它基于全局变量 errno 给出可读的描述
        perror("sigaction for SIGPIPE failed");
        // 也可以考虑使用更底层的 write 来避免标准流的不确定性
        // const char msg[] = "set ignore sigpipe failed\n";
        // write(STDERR_FILENO, msg, sizeof(msg) - 1);
    }
    // 如果成功，则安静地返回即可
}