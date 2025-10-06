# 编译器选择：g++ 用于 C++，如需 C 可启用 CC = gcc
CXX := g++

# 最终生成的可执行文件名称
TARGET := webserver

# 列出所有需要编译的源文件
SRCS := main.cpp ThreadPool.cpp TcpServer.cpp Logger.cpp epoll.cpp HttpServer.cpp HttpRequest.cpp HttpResponse.cpp TcpTimer.cpp TimerNode.cpp Util.cpp

# 将源文件名（如 main.cpp）转换为目标文件名（如 main.o）
OBJS := $(SRCS:.cpp=.o)

# 编译选项
# -Wall: 开启所有警告 -Wextra: 额外警告 -O2: 优化级别 -std=c++17: C++标准版本
CXXFLAGS := -Wall -Wextra -O2 -std=c++17

# 链接选项，通常用于指定库，此处留空。如需链接数学库，可添加 -lm
LDFLAGS := 

# 伪目标声明，防止与同名文件冲突
.PHONY: all clean rebuild

# 默认目标：编译整个项目
all: $(TARGET)

# 链接：将多个 .o 文件链接成最终可执行程序
# $^ 代表所有依赖项（即 $(OBJS)），$@ 代表目标（即 $(TARGET)）
$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) $^ -o $@

# $< 代表第一个依赖项（即对应的 .cpp 文件）
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 清理：删除编译过程中生成的中间文件（.o）和最终可执行文件
clean:
	rm -f $(OBJS) $(TARGET)

# 重新构建：先执行 clean，再执行 all
rebuild: clean all