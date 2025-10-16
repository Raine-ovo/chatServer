#ifndef GROUP_H
#define GROUP_H

#include "groupuser.hpp"

#include <string>
#include <vector>
using namespace std;

class Group
{
public:
    Group(int id = -1, string name = "", string desc = "");
    
    void setId(int id);
    void setName(string name);
    void setDesc(string desc);

    int getId();
    string getName();
    string getDesc();
    vector<GroupUser> &getUsers();

private:
    int id;
    string name;
    string desc;
    vector<GroupUser> users;
};

#endif