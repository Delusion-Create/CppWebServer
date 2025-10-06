# 设置 CMake 的最低版本要求
cmake_minimum_required(VERSION 3.10)

# 定义项目名称和语言
project(webserver VERSION 1.0.0 LANGUAGES CXX)

# 设置 C++ 编译标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置编译选项
add_compile_options(-Wall -Wextra -O2)

# 设置可执行文件的输出目录为 out
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/out)

# 自动收集 src 目录下的所有 .cpp 源文件
aux_source_directory(src SRC_FILES)

# 生成可执行文件
add_executable(${PROJECT_NAME} ${SRC_FILES})