#include "redis.hpp"
#include <hiredis/hiredis.h>
#include <hiredis/read.h>
#include <iostream>

Redis::Redis()
    : _publish_context(nullptr), _subscribe_context(nullptr)
{

}

Redis::~Redis()
{
    if (_publish_context != nullptr)
    {
        redisFree(_publish_context);
    }

    if (_subscribe_context != nullptr)
    {
        redisFree(_subscribe_context);
    }
}

bool Redis::connect()
{
    // 负责 publish 发布消息的上下文连接
    _publish_context = redisConnect("127.0.0.1", 6379);
    if (nullptr == _publish_context)
    {
        cerr << "connect redis failed!" << endl;
        return false;
    }

    // 负责 subscribe 订阅消息的上下文连接
    _subscribe_context = redisConnect("127.0.0.1", 6379);
    if (nullptr == _subscribe_context)
    {
        cerr << "connect redis failed!" << endl;
        return false;
    }

    // 创建单独的线程用于监听通道上的事件，有消息给业务层进行上报
    thread t([&]() {
        observe_channel_message();
    });
    t.detach();

    cout << "connect redis-server success!" << endl;

    return true;
}

bool Redis::publish(int channel, string message)
{
    redisReply *reply = (redisReply *) redisCommand(_publish_context, "PUBLISH %d %s", channel, message.c_str());
    if (nullptr == reply)
    {
        cerr << "publish command failed!" << endl;
        return false;
    }
    freeReplyObject(reply);
    return true;
}

// 向 redis 指定的通道 subscribe 订阅消息
bool Redis::subscribe(int channel)
{
    // SUBSCRIBE 命令本身会造成线程阻塞等待通道里面发生消息
    // redisCommand = redisAppendCommand + redisBufferCommand + redisGetReply
    // 线程阻塞是因为 redisGetReply 导致的
    // 因此这里要手动进行 appendCommand + bufferCommand ，只做订阅通道而不接收通道信息
    
    // 通道信息的接收专门在 observer_channel_message 函数中的独立线程中进行
    // 只负责发送命令，不阻塞接收 redis server 响应信息，否则和 notifyMsg 线程抢占响应资源
    if (REDIS_ERR == redisAppendCommand(this->_subscribe_context, "SUBSCRIBE %d", channel))
    {
        cerr << "subscribe command failed!" << endl;
        return false;
    }
    int done = 0;
    while (!done)
    {
        if (REDIS_ERR == redisBufferWrite(this->_subscribe_context, &done))
        {
            cerr << "subscribe command failed!" << endl;
            return false;
        }
    }
    return true;
}

// 向 redis 指定的通道 unsubscribe 取消订阅消息
bool Redis::unsubscribe(int channel)
{
    if (REDIS_ERR == redisAppendCommand(this->_subscribe_context, "UNSUBSCRIBE %d", channel))
    {
        cerr << "unsubscribe command failed!" << endl;
        return false;
    }
    // redisBufferWrite 可以循环发送缓冲区，直到缓冲区数据发送完毕（done 被置为 1）
    int done = 0;
    while (!done)
    {
        if (REDIS_ERR == redisBufferWrite(this->_subscribe_context, &done))
        {
            cerr << "unsubscribe command failed!" << endl;
            return false;
        }
    }
    return true;
}

// 在独立线程中接收订阅通道中的信息
void Redis::observe_channel_message()
{
    redisReply *reply = nullptr;
    while (REDIS_OK == redisGetReply(this->_subscribe_context, (void **)&reply))
    {
        // 订阅通道收到了消息
        if (reply != nullptr && reply->element[2] != nullptr && reply->element[2]->str != nullptr)
        {
            // 使用回调函数，给业务层上报通道上发生的消息
            _notify_message_handler(atoi(reply->element[1]->str), reply->element[2]->str);
        }
        
        freeReplyObject(reply);
    }

    cerr << ">>>>>>>>>>> observer_channel_message quit <<<<<<<<<<<<" << endl;
}


void Redis::init_notify_handler(function<void(int, string)> fn)
{
    this->_notify_message_handler = fn;
}