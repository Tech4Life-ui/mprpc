#include "mprpcchannel.h"
#include <string>
#include "rpcheader.pb.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <error.h>
#include "mprpcapplication.h"
#include "mprpccontroller.h"
#include <unistd.h>
#include "zookeeperutil.h"

void MprpcChannel::CallMethod(const google::protobuf::MethodDescriptor *method, google::protobuf::RpcController *controller, const google::protobuf::Message *request, google::protobuf::Message *response, google::protobuf::Closure *done)
{
    const google::protobuf::ServiceDescriptor *sd = method->service();
    std::string service_name = sd->name();
    std::string method_name = method->name();

    uint32_t args_size = 0;
    std::string args_str;

    if (request->SerializeToString(&args_str))
    {
        args_size = args_str.size();
    }
    else
    {
        // std::cout << "serialize request error!" << std::endl;
        controller->SetFailed("serialize request error!");
        return;
    }

    mprpc::RpcHeader rpcHeader;
    rpcHeader.set_service_name(service_name);
    rpcHeader.set_method_name(method_name);
    rpcHeader.set_args_size(args_size);

    uint32_t header_size = 0;
    std::string rpc_header_str;
    if (rpcHeader.SerializeToString(&rpc_header_str))
    {
        header_size = rpc_header_str.size();
    }
    else
    {
        // std::cout << "serialize rpc header error!" << std::endl;
        controller->SetFailed("serialize rpc header error!");
        return;
    }

    std::string send_rpc_str;
    send_rpc_str.insert(0, std::string((char *)&header_size, 4));
    send_rpc_str += rpc_header_str;
    send_rpc_str += args_str;

    // 打印调试信息
    std::cout << "==================================" << std::endl;
    std::cout << "header_size: " << header_size << std::endl;       // RPC头的大小（字节）
    std::cout << "rpc_header_str: " << rpc_header_str << std::endl; // RPC头的大小（字节）
    std::cout << "service_name: " << service_name << std::endl;     // 被调用的服务名，如" login"
    std::cout << "method_name: " << method_name << std::endl;       // 被调用的方法名，如 " Login"
    std::cout << "args_str: " << args_str << std::endl;             // 参数的大小（字节）
    std::cout << "==================================" << std::endl;

    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if (clientfd == -1)
    {
        // std::cout << "create socket error! errno: " << errno << std::endl;
        char errtxt[512] = {0};
        sprintf(errtxt, "create socket error! errno: %d", errno);
        controller->SetFailed(errtxt);
        exit(EXIT_FAILURE);
    }

    // std::string ip = MprpcApplication::getInstance().GetConfig().Load("rpcserverip");
    // uint16_t port = atoi(MprpcApplication::getInstance().GetConfig().Load("rpcserverport").c_str());、

    ZkClient zkCli;
    zkCli.Start();

    std::string method_path = "/" + service_name + "/" + method_name;
    std::string host_data = zkCli.GetData(method_path.c_str());
    if (host_data == "")
    {
        controller->SetFailed(method_path + " is not exist!!");
        return;
    }
    int idx = host_data.find(":");
    if (idx == -1)
    {
        controller->SetFailed(method_path + " address is invalid!!");
        return;
    }
    std::string ip = host_data.substr(0, idx);
    uint16_t port = atoi(host_data.substr(idx + 1, host_data.size() - idx).c_str());

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip.c_str());

    if (connect(clientfd, (struct sockaddr *)&server_addr, sizeof(server_addr)))
    {
        // std::cout << "connect error! errno: " << errno << std::endl;
        close(clientfd);
        char errtxt[512] = {0};
        sprintf(errtxt, "connect socket error! errno: %d", errno);
        controller->SetFailed(errtxt);

        exit(EXIT_FAILURE);
    }

    if (send(clientfd, send_rpc_str.c_str(), send_rpc_str.size(), 0) == -1)
    {
        // std::cout << "send error! errno: " << errno << std::endl;
        close(clientfd);
        char errtxt[512] = {0};
        sprintf(errtxt, "send socket error! errno: %d", errno);
        controller->SetFailed(errtxt);
        return;
    }

    char recv_buf[1024] = {0};
    int recv_size = 0;
    if ((recv_size = recv(clientfd, recv_buf, 1024, 0)) == -1)
    {
        // std::cout << "recv error! errno: " << errno << std::endl;
        close(clientfd);
        char errtxt[512] = {0};
        sprintf(errtxt, "recv socket error! errno: %d", errno);
        controller->SetFailed(errtxt);
        return;
    }
    // std::string response_str(recv_buf, 0, recv_size);
    // if (!response->ParseFromString(response_str))
    if (!response->ParseFromArray(recv_buf, recv_size))
    {
        // std::cout << "parse error! response_str: " << recv_buf << std::endl;
        close(clientfd);
        char errtxt[512] = {0};
        int ret = snprintf(errtxt, sizeof(errtxt), "parse error! errno: %s", recv_buf);
        controller->SetFailed(errtxt);
        return;
    }
    close(clientfd);
}