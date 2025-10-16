/*
muduo 网络库给用户提供两个主要的类
TcpServer: 编写服务器程序
TcpClient: 编写客户端程序

epoll + 线程池
好处：能把网络 I/O 的代码和业务代码区分开
                        用户的连接/断开  用户可读写事件
*/

#include <muduo/base/Timestamp.h>
#include <muduo/net/Callbacks.h>
#include <muduo/net/InetAddress.h>
#include <muduo/net/TcpConnection.h>
#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <iostream>
#include <functional>
#include <string>
using namespace std;
using namespace placeholders;
using namespace muduo;
using namespace muduo::net;

/*
基于muduo网络库开发服务器程序
1. 组合 TcpServer 对象
2. 创建 EventLoop 事件循环对象指针
3. 明确 TcpServer 构造函数需要什么参数，输出 ChatServer 构造函数
4. 在当前服务器类的构造函数中，注册处理连接的回调函数和处理读写的回调函数
5. 设置合适的服务端线程数量，muduo 库会自己分配 I/O 线程和 worker 线程
*/

class ChatServer
{
public:
    ChatServer(EventLoop *loop, // 事件循环
            const InetAddress& listenAddr, // Ip Port
            const string& nameArg) // 服务器名称
            : _server(loop, listenAddr, nameArg), _loop(loop)
    {
        // 给服务器注册用户连接的创建和断开回调
        _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));

        // 给服务器注册用户读写事件回调
        _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));

        // 设置服务器端的线程数量  1 个 I/O 线程  3 个 Worker 线程
        _server.setThreadNum(4);
    }

    void start()
    {
        _server.start();
    }

private:
    // 处理用户的连接创建和断开
    void onConnection(const TcpConnectionPtr &conn)
    {
        if (conn->connected())
        {
            cout << conn->peerAddress().toIpPort() << "->" <<
            conn->localAddress().toIpPort() << "online" << endl;
        }
        else
        {
            cout << conn->peerAddress().toIpPort() << "->" <<
            conn->localAddress().toIpPort() << "offline" << endl;
            conn->shutdown();
            // _loop->quit();
        }
    }

    void onMessage(const TcpConnectionPtr &conn,
                    Buffer *buffer,
                    Timestamp time)
    {
        string buf = buffer->retrieveAllAsString();
        cout << "recv data:" << buf << " time:" << time.toString() << endl;
        conn->send(buf);
    }

    TcpServer _server; // # 1
    EventLoop *_loop; // # 2
};

int main ()
{
    EventLoop loop; // epoll
    InetAddress addr("127.0.0.1", 6000);
    ChatServer server(&loop, addr, "ChatServer");
    server.start(); // listen epoll_ctl => epoll
    loop.loop(); // 开始 I/O 线程 以阻塞方式等待新用户连接，已连接用户的读写事件
    return 0;
}
