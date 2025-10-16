#include "group.hpp"


Group::Group(int id, string name, string desc)
{
    this->id = id;
    this->name = name;
    this->desc = desc;
}
    
void Group::setId(int id)
{
    this->id = id;
}
void Group::setName(string name)
{
    this->name = name;
}
void Group::setDesc(string desc)
{
    this->desc = desc;
}

int Group::getId()
{
    return this->id;
}
string Group::getName()
{
    return this->name;
}
string Group::getDesc()
{
    return this->desc;
}
vector<GroupUser>& Group::getUsers()
{
    return this->users;
}