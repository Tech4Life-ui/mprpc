#include <iostream>
#include <string>
#include "../friend.pb.h"
#include "mprpcapplication.h"
#include "rpcprovider.h"
#include <vector>
#include "logger.h"

class FriendService : public fixbug::FriendServiceRpc
{
public:
    std::vector<std::string> GetFriendsList(uint32_t userid)
    {
        std::cout << "doing local service Login! userid: " << userid << std::endl;
        std::vector<std::string> vec;
        vec.push_back("li si");
        vec.push_back("wang wu");
        vec.push_back("hajimi");
        return vec;
    }

    void GetFriendList(::google::protobuf::RpcController *controller,
                       const ::fixbug::GetFriendsListRequest *request,
                       ::fixbug::GetFriendsListResponse *response,
                       ::google::protobuf::Closure *done)
    {
        uint32_t userid = request->userid();

        auto friendsList = GetFriendsList(userid);
        response->mutable_result()->set_errcode(0);
        response->mutable_result()->set_errmsg("");
        for (auto &name : friendsList)
        {
            std::string *p = response->add_friends();
            *p = name;
        }
        done->Run();
    }
};

int main(int argc, char **argv)
{
    LOG_INFO("logger message!");
    LOG_ERROR("%s:%s:%d", __FILE__, __FUNCTION__, __LINE__);

    MprpcApplication::Init(argc, argv);

    RpcProvider provider;
    provider.NotifyService(new FriendService());

    provider.Run();
    return 0;
}