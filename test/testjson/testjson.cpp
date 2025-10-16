#include "json.hpp"
using json = nlohmann::json;

#include <iostream>
#include <vector>
#include <map>
#include <string>
using namespace std;

// json 序列化 示例 1
string func1()
{
    json js;
    js["msg_type"] = 2;
    js["from"] = "zhang san";
    js["to"] = "li si";
    js["msg"] = "hello, what are you doing now?";

    string sendBuf = js.dump();
    cout << sendBuf.c_str() << endl;

    return sendBuf;
}

// json 序列化 示例 2
string func2()
{
    json js;
    // 添加数组
    js["id"] = {1, 2, 3, 4, 5};
    // 添加 key value
    js["name"] = "zhang san";
    // 添加对象
    js["msg"]["zhang san"] = "hello world";
    js["msg"]["liu shuo"] = "hello china";
    // 上面等同于下面这句一次性添加数组对象
    js["msg"] = {{"zhang san", "hello world"}, {"li si", "hello china"}};

    cout << js << endl;

    return js.dump();
}

// json 序列化 示例 3
void func3()
{
    json js;

    // 直接序列化一个 vector 容器
    vector<int> vec;
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);

    js["list"] = vec;

    // 直接序列化一个 map 容器
    map<int, string> m;
    m.insert({1, "黄山"});
    m.insert({2, "华山"});
    m.insert({3, "泰山"});

    js["path"] = m;

    cout << js << endl;

    string sendBuf = js.dump(); // json 数据对象 => 序列化为 string 字符串
    cout << sendBuf << endl;
}

int main ()
{
    func1();

    func2();

    func3();

    string recvBuf = func1();
    // 数据反序列化  json 字符串 => 反序列化 数据对象，方便访问对象
    json jsbuf = json::parse(recvBuf);
    cout << jsbuf["msg_type"] << endl;
    cout << jsbuf["from"] << endl;
    cout << jsbuf["to"] << endl;
    cout << jsbuf["msg"] << endl;

    recvBuf = func2();
    jsbuf = json::parse(recvBuf);
    cout << jsbuf["id"] << endl;
    auto arr = jsbuf["id"];
    cout << arr[2] << endl;

    return 0;
}
