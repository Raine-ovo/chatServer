cmake_minimum_required(VERSION 3.0)
project(main)

# 配置编译选项
# set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -g) # 可调试

# 配置头文件搜索路径
# include_directories()
# 配置库文件搜索路径
# link_directories()

# 设置需要编译的源文件列表
set(SRC_LIST muduo_server.cpp)

# 把 指定路径下的所有源文件名字放入变量名 SRC_LIST 中
aux_source_directory(. SRC_LIST)

# 设置可执行文件最终存储路径, CMAKE_SOURCE_DIR 是项目根目录
# PROJECT_SOURCE_DIR 是项目根目录，即最后一个使用 project 的 cmake 文件
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)

# 表示生成可执行文件 server，由 SRC_LIST 中的所有源文件编译生成
add_executable(server ${SRC_LIST}) # 生成可执行文件
# 表示 server 这个目标程序，需要连接 muduo_net muduo_base pthread 这三个库
target_link_libraries(server muduo_net muduo_base pthread)
