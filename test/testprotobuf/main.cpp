#include "test.pb.h"
#include <iostream>
using namespace fixbug;

void test1()
{
    LoginRequest req;
    req.set_name("zhang san");
    req.set_pwd("123");

    std::string s;
    if (req.SerializeToString(&s))
    {
        std::cout << s.c_str() << std::endl;
    }

    LoginRequest reqB;
    if (reqB.ParseFromString(s))
    {
        std::cout << reqB.name() << std::endl;
        std::cout << reqB.pwd() << std::endl;
    }
}

int main()
{
    LoginResponse rsp;
    ResultCode *rc = rsp.mutable_result();
    rc->set_errcode(1);
    rc->set_errmsg("登录成功");

    userList ul;
    User *user1 = ul.add_users();
    user1->set_name("张三");
    user1->set_age(10);

    User *user2 = ul.add_users();
    user2->set_name("李四");
    user2->set_age(33);

    std::cout << ul.users_size() << std::endl;
    std::cout << ul.users(0).age() << std::endl;

    return 0;
}