# ChatServer
可以工作在 nginx tcp 负载均衡环境中的集群聊天服务器和客户端源码，基于 muduo 、 redis 、mysql 进行开发

使用根目录下的 autobuild.sh 进行编译

需要 nginx 的 tcp 负载均衡环境下使用，同时启动 redis-server 与 mysql 服务
![](imgs/nginx.png)