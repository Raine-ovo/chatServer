#include "chatserver.hpp"
#include "chatservice.hpp"

#include <cstdint>
#include <muduo/net/EventLoop.h>
#include <muduo/net/InetAddress.h>

#include <signal.h>
#include <iostream>
using namespace std;

// 处理服务器 ctrl+c 后，重置 user 的状态信息
void resetHandler(int)
{
    ChatService::instance()->reset();
    exit(0);
}

int main (int argc, char **argv)
{
    if (argc < 3)
    {
        cerr << "command invalid example: ./ChatServer 127.0.0.1 6000" << endl;
        exit(-1);
    }

    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);
    
    // signal(SIGINT ...) 主要作用是 自定义程序接收到 ctrl+C 中断信号时的行为，而不是默认直接退出程序
    signal(SIGINT, resetHandler);

    EventLoop loop;
    InetAddress addr(ip, port);
    ChatServer server(&loop, addr, "ChatServer");
    server.start();
    loop.loop();
    return 0;
}