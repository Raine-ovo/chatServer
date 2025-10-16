#!/bin/bash

# 编译

# 如果没有 build 则创建
if [ ! -d "build" ]; then
    mkdir build
fi

# 进入 build 目录
cd build

# 删除 build 目录下的所有文件
rm -rf *

# 执行 cmake
cmake ..

# 执行 make
make
