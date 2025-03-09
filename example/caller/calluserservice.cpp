#include <iostream>
#include "mprpcapplication.h"
#include "user.pb.h"

int main(int argc, char **argv)
{
    MprpcApplication::Init(argc, argv);

    fixbug::UserServiceRpc_Stub stub(new MprpcChannel());

    fixbug::LoginRequest request;
    request.set_name("zhang san");
    request.set_pwd("123456");

    fixbug::LoginResponse response;
    stub.Login(nullptr, &request, &response, nullptr);

    if (response.result().errcode() == 0)
    {
        std::cout << "rpc login response success" << response.success() << std::endl;
    }
    else
    {
        std::cout << "rpc login response error" << response.result().errcode() << std::endl;
    }

    fixbug::RegisterRequest req;
    req.set_id(999);
    req.set_name("mmppp");
    req.set_pwd("99888");
    fixbug::RegisterResponse rsp;
    stub.Register(nullptr, &req, &rsp, nullptr);
    if (rsp.result().errcode() == 0)
    {
        std::cout << "rpc register response success" << response.success() << std::endl;
    }
    else
    {
        std::cout << "rpc register response error" << response.result().errcode() << std::endl;
    }
    return 0;
}