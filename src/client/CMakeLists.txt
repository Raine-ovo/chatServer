aux_source_directory(. SRC_LIST)

# 将客户端依赖的公共 model 源（如 Group）加入编译
set(CLIENT_EXTRA_SOURCES
    ${PROJECT_SOURCE_DIR}/src/server/model/group.cpp
)

add_executable(ChatClient ${SRC_LIST} ${CLIENT_EXTRA_SOURCES})
# 读 + 写 两个线程
target_link_libraries(ChatClient pthread)