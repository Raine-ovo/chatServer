#ifndef REDIS_H
#define REDIS_H

#include <hiredis/hiredis.h>
#include <thread>
#include <functional>
using namespace std;

class Redis
{
public:
    Redis();
    ~Redis();

    // 连接 redis 服务器
    bool connect();

    // 向 redis 指定的通道 channel 发布消息
    bool publish(int channel, string message);

    // 向 redis 指定的通道 subscribe 订阅消息
    bool subscribe(int channel);

    // 向 redis 指定的通道 unsubscribe 取消订阅消息
    bool unsubscribe(int channel);

    // 在独立线程中接收订阅通道中的信息
    void observe_channel_message();

    // 初始化向业务层上报通道消息的回调函数
    void init_notify_handler(function<void(int, string)> fn);

private:
    // hiredis 同步上下文，相当于 redis-cli
    redisContext *_subscribe_context;
    redisContext *_publish_context;

    // 回调操作，收到订阅的信息，给 service 层上报
    function<void(int, string)> _notify_message_handler;
};

#endif