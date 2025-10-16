#include "groupuser.hpp"
#include "json.hpp"
#include <cstdint>
#include <iostream>
#include <pthread.h>
#include <thread>
#include <string>
#include <strings.h>
#include <vector>
#include <chrono>
#include <ctime>
#include <unordered_map>
using namespace std;
using json = nlohmann::json;

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#include "group.hpp"
#include "user.hpp"
#include "public.hpp"

// 记录当前系统登录的用户信息
User g_currentUser;
// 记录当前登录用户的好友列表信息
vector<User> g_currentUserFriendList;
// 记录当前登录用户的群组列表信息
vector<Group> g_currentUserGroupList;
// 显示当前登陆成功用户的基本信息
void showCurrentUserData();
// 控制聊天页面程序
bool isMainMenuRuning = false;

// 接收线程
void readTaskHandler(int clientfd);
// 获取系统时间(聊天信息需要添加时间信息)
string getCurrentTime();
// 主聊天页面程序
void mainMenu(int clientfd);

// 聊天客户端程序实现，main 线程用作发送线程，子线程用作接收线程
int main(int argc, char **argv)
{
    if (argc < 3)
    {
        cerr << "command invalid example: ./ChatClient 127.0.0.1 6000" << endl;
        exit(-1);
    }
    
    // 解析通过命令行参数传递的 ip 和 port
    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);

    // 创建 client 端的 socket
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == clientfd)
    {
        cerr << "socket create error" << endl;
        exit(-1);
    }

    // 填写 client 需要连接的 server 信息 ip+port
    sockaddr_in server;
    bzero(&server, sizeof(server));

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(ip);
    server.sin_port = htons(port);

    // client 和 server 进行连接
    if (-1 == connect(clientfd, (sockaddr*)&server, sizeof(sockaddr_in)))
    {
        cerr << "connect server error" << endl;
        close(clientfd);
        exit(-1);
    }

    for (;;)
    {
        // 显示首页菜单 登录、注册、退出
        cout << "=================" << endl;
        cout << "1. login" << endl;
        cout << "2. register" << endl;
        cout << "3. loginout" << endl;
        cout << "=================" << endl;
        cout << "choice: ";
        int choice = 0;
        cin >> choice;
        cin.get();

        switch(choice)
        {
        case 1: // login 业务
        {
            int id = 0;
            char pwd[50] = {0};
            cout << "userid: ";
            cin >> id;
            cin.get(); // 去掉缓冲区中的 '\n'
            cout << "user password: ";
            cin.getline(pwd, 50);

            json js;
            js["msgid"] = LOGIN_MSG;
            js["id"] = id;
            js["password"] = pwd;
            string request = js.dump();

            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (-1 == len)
            {
                cerr << "send login msg error: " << request << endl;
            }
            else
            {
                char buffer[1024] = {0};
                len = recv(clientfd, buffer, 1024, 0);
                if (-1 == len)
                {
                    cerr << "recv login response error" << endl;
                }
                else
                {
                    json responsejs = json::parse(buffer);
                    if (0 != responsejs["errno"].get<int>())
                    {
                        cerr << responsejs["errmsg"] << endl;
                    }
                    else // 登陆成功
                    {
                        // 记录当前用户 id 和 name
                        g_currentUser.setId(responsejs["id"].get<int>());
                        g_currentUser.setName(responsejs["name"]);

                        // 记录当前用户的好友列表信息
                        if (responsejs.contains("friends"))
                        {
                            g_currentUserFriendList.clear();

                            vector<string> vec = responsejs["friends"];
                            for (string &str: vec)
                            {
                                json js = json::parse(str);
                                User user;
                                user.setId(js["id"].get<int>());
                                user.setName(js["name"]);
                                user.setState(js["state"]);
                                g_currentUserFriendList.push_back(user);
                            }
                        }

                        // 记录当前用户的群组列表信息
                        if (responsejs.contains("groups"))
                        {
                            g_currentUserGroupList.clear();

                            vector<string> vec1 = responsejs["groups"];
                            for (string &groupstr: vec1)
                            {
                                json grpjs = json::parse(groupstr);
                                Group group;
                                group.setId(grpjs["id"].get<int>());
                                group.setName(grpjs["groupname"]);
                                group.setDesc(grpjs["groupdesc"]);

                                vector<string> vec2 = grpjs["users"];
                                for (string &userstr: vec2)
                                {
                                    GroupUser user;
                                    json js = json::parse(userstr);
                                    user.setId(js["id"].get<int>());
                                    user.setName(js["name"]);
                                    user.setState(js["state"]);
                                    user.setRole(js["role"]);
                                    group.getUsers().push_back(user);
                                }

                                g_currentUserGroupList.push_back(group);
                            }
                        }

                        // 显示登录用户的基本信息
                        showCurrentUserData();

                        // 显示当前用户的离线消息  个人聊天信息或者群组消息
                        if (responsejs.contains("offlinemsg"))
                        {
                            vector<string> vec = responsejs["offlinemsg"];
                            for (string &str: vec)
                            {
                                json js = json::parse(str);        
                                int msgtype = js["msgid"].get<int>();
                                // time + [id] + name + " said:" + xxx
                                if (msgtype == ONE_CHAT_MSG)
                                {
                                    cout << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>()
                                         << " said: " << js["msg"].get<string>() <<endl;
                                    continue;
                                }
                                
                                if (msgtype == GROUP_CHAT_MSG)
                                {
                                    cout << "群消息[" << js["groupid"] << "]" << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>()
                                         << " said: " << js["msg"].get<string>() <<endl;
                                }
                            }
                        }

                        // 登录成功，启动接收线程负责接收数据
                        static int readthreadnumber = 0;
                        if (readthreadnumber == 0) {
                            std::thread readTask(readTaskHandler, clientfd);
                            readTask.detach(); // 与主线程分离并开始工作
                            ++ readthreadnumber;
                        }

                        // 进入聊天主菜单
                        isMainMenuRuning = true;
                        mainMenu(clientfd);
                    }
                }
            }
            break;
        }
        case 2:
        {
            char name[50] = {0};
            char pwd[50] = {0};
            cout << "username: ";
            cin.getline(name, 50);
            cout << "user password: ";
            cin.getline(pwd, 50);

            json js;
            js["msgid"] = REG_MSG;
            js["name"] = name;
            js["password"] = pwd;
            string request = js.dump();

            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (len == -1)
            {
                cerr << "send reg msg error:" << request << endl;
            }
            else
            {
                char buffer[1024] = {0};
                len = recv(clientfd, buffer, 1024, 0);
                if (-1 == len)
                {
                    cerr << "recv reg response error" << endl;
                }
                else
                {
                    json responsejs = json::parse(buffer);
                    if (0 != responsejs["errno"].get<int>())
                    {
                        cerr << name << "is already exist, register error!" << endl;
                    }
                    else // 注册成功
                    {
                        cout << name << " register success, userid is " << responsejs["id"]
                             << ", do not forget it!" << endl;
                    }
                }
            }
            break;
        }
        case 3:
        {
            close(clientfd);
            exit(0);
        }
        default:
        {
            cerr << "invalid input" << endl;
            break;
        }
        }
    }
}

void showCurrentUserData()
{
    cout << "========================= login user ============================" << endl;
    cout << "current login user => id:" << g_currentUser.getId() << " name" << g_currentUser.getName() << endl;
    cout << "------------------------ friend list ---------------------------" << endl;
    if (!g_currentUserFriendList.empty())
    {
        for (User &user: g_currentUserFriendList)
        {
            cout << user.getId() << " " << user.getName() << " " << user.getState() << endl;
        }
    }
    cout << "----------------------- group list ----------------------------" << endl;
    if (!g_currentUserGroupList.empty())
    {
        for (Group &group: g_currentUserGroupList)
        {
            cout << group.getId() << " " << group.getName() << " " << group.getDesc() << endl;
            for (GroupUser &user: group.getUsers())
            {
                cout << user.getId() << " " << user.getName() << " " << user.getState() << " "
                     << user.getRole() << endl;
            }
        }
    }
    cout << "===============================================================" << endl;
}

void readTaskHandler(int clientfd)
{
    for (;;)
    {
        char buffer[1024] = {0};
        int len = recv(clientfd, buffer, 1024, 0);
        if (len == -1 || len == 0)
        {
            close(clientfd);
            exit(-1);
        }

        // 接收 ChatServer 转发的数据，反序列化生成 json 数据对象
        json js = json::parse(buffer);
        int msgtype = js["msgid"].get<int>();
        if (msgtype == ONE_CHAT_MSG)
        {
            cout << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>()
                 << " said: " << js["msg"].get<string>() <<endl;
            continue;
        }
        
        if (msgtype == GROUP_CHAT_MSG)
        {
            cout << "群消息[" << js["groupid"] << "]" << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>()
                 << " said: " << js["msg"].get<string>() <<endl;
        }
    }
}

void help(int clientfd=0, string command="");
void chat(int, string);
void addfriend(int, string);
void creategroup(int, string);
void addgroup(int, string);
void groupchat(int, string);
void loginout(int, string);

// 系统支持的客户端命令列表
unordered_map<string, string> commandMap = {
    { "help", "显示所有支持的命令, 格式 help" },
    { "chat", "一对一聊天, 格式 chat:friendid:message" },
    { "addfriend", "添加好友, 格式 addfriend:friendid" },
    { "creategroup", "创建群组, 格式 creategroup:groupname:groupdesc" },
    { "addgroup", "加入群组, 格式 addgroup:groupid" },
    { "groupchat", "群聊, 格式 groupchat:groupid:message" },
    { "loginout", "注销, 格式 loginout" }
};

// 注册系统支持的客户端命令
unordered_map<string, function<void(int, string)>> commandHandlerMap = {
    { "help", help },
    { "chat", chat },
    { "addfriend", addfriend },
    { "creategroup", creategroup },
    { "addgroup", addgroup },
    { "groupchat", groupchat },
    { "loginout", loginout }
};

void mainMenu(int clientfd)
{
    help();

    char buffer[1024] = {0};
    while (isMainMenuRuning)
    {
        cin.getline(buffer, 1024);
        string commandbuf(buffer);
        string command; // 存储命令
        int idx = commandbuf.find(":");
        if (-1 == idx)
        {
            command = commandbuf;
        }
        else
        {
            command = commandbuf.substr(0, idx);
        }
        auto it = commandHandlerMap.find(command);
        if (it == commandHandlerMap.end())
        {
            cerr << "invalid input command!" << endl;
            continue;
        }

        // 调用相应命令的处理函数, mainMenu 对修改封闭, 添加新功能不需要修改该函数
        it->second(clientfd, commandbuf.substr(idx + 1));
    }
}

void help(int, string)
{
    cout << "show command list >>> " << endl;
    for (auto &p: commandMap)
    {
        cout << p.first << " : " << p.second << endl;
    }
    cout << endl;
}

void addfriend(int clientfd, string command)
{
    int friendid = atoi(command.c_str());
    json js;
    js["msgid"] = ADD_FRIEND_MSG;
    js["id"] = g_currentUser.getId();
    js["friendid"] = friendid;
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "send addfriend msg error -> " << buffer << endl;
    }
}

void chat(int clientfd, string command)
{
    int idx = command.find(":");
    if (-1 == idx)
    {
        cerr << "chat comand invalid!" << endl;
        return ;
    }

    int friendid = stoi(command.substr(0, idx));
    string message = command.substr(idx + 1);

    json js;
    js["msgid"] = ONE_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["toid"] = friendid;
    js["msg"] = message;

    js["time"] = getCurrentTime();
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);

    if (-1 == len)
    {
        cerr << "send chat msg error -> " << buffer << endl;
    }
}

// creategroup:groupname:groupdesc
void creategroup(int clientfd, string command)
{
    int idx = command.find(':');
    if (-1 == idx)
    {
        cerr << "creategroup command invalid!" << endl;
        return ;
    }

    string groupname = command.substr(0, idx);
    string groupdesc = command.substr(idx + 1);

    json js;
    js["msgid"] = CREATE_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupname"] = groupname;
    js["groupdesc"] = groupdesc;
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);

    if (-1 == len)
    {
        cerr << "send creategroup msg error -> " << buffer << endl;
    }
}

// addgroup:groupid
void addgroup(int clientfd, string command)
{
    int groupid = atoi(command.c_str());
    json js;
    js["msgid"] = ADD_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupid"] = groupid;
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "send addgroup msg error -> " << buffer << endl;
    }
}

// groupchat:groupid:message
void groupchat(int clientfd, string command)
{
    int idx = command.find(':');
    if (-1 == idx)
    {
        cerr << "groupchat command invalid!" << endl;
        return ;
    }

    int groupid = stoi(command.substr(0, idx));
    string message = command.substr(idx + 1);

    json js;
    js["msgid"] = GROUP_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["groupid"] = groupid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);

    if (-1 == len)
    {
        cerr << "send creategroup msg error -> " << buffer << endl;
    }
}

// logout
void loginout(int clientfd, string command)
{
    json js;
    js["msgid"] = LOGINOUT_MSG;
    js["id"] = g_currentUser.getId();
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "send loginout msg error -> " << buffer << endl;
    }
    else
    {
        isMainMenuRuning = false;
    }
}

string getCurrentTime()
{
    auto now = chrono::system_clock::now();
    time_t t = chrono::system_clock::to_time_t(now);
    char buf[64] = {0};
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&t)); // 格式化时间
    return buf;
}