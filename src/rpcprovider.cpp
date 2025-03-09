#include "rpcprovider.h"
#include <string>
#include "mprpcapplication.h"
#include <functional>
#include "rpcheader.pb.h"
#include "zookeeperutil.h"
// ---------------------------- 服务注册方法 ----------------------------
/**
 * @brief 注册服务到 RPC 框架
 * @param service 需要注册的 Protobuf 服务对象
 *
 * 工作流程：
 * 1. 获取服务描述符
 * 2. 记录服务名及其所有方法描述符
 * 3. 存储到服务映射表
 */
void RpcProvider::NotifyService(::google::protobuf::Service *service)
{
    ServiceInfo service_info;

    // 获取服务描述符（包含服务名、方法列表等信息）
    const google::protobuf::ServiceDescriptor *pserviceDesc = service->GetDescriptor();
    std::string service_name = pserviceDesc->name();
    int mothodCnt = pserviceDesc->method_count();

    std::cout << "service_name: " << service_name << std::endl;

    for (int i = 0; i < mothodCnt; ++i)
    {
        const google::protobuf::MethodDescriptor *pmethodDesc = pserviceDesc->method(i);
        std::string method_name = pmethodDesc->name();
        service_info.m_methodMap.insert({method_name, pmethodDesc});

        std::cout << "method_name: " << method_name << std::endl;
    }
    service_info.m_service = service;
    m_serviceMap.insert({service_name, service_info});
}

// ---------------------------- 服务启动方法 ----------------------------
/**
 * @brief 启动 RPC 服务端
 *
 * 流程：
 * 1. 读取配置获取监听地址
 * 2. 初始化 muduo 网络库
 * 3. 设置回调函数
 * 4. 启动事件循环
 */
void RpcProvider::Run()
{
    // 从配置中心获取网络参数
    std::string ip = MprpcApplication::getInstance().GetConfig().Load("rpcserverip");
    uint16_t port = atoi(MprpcApplication::getInstance().GetConfig().Load("rpcserverport").c_str());

    // 创建 muduo 网络地址对象
    muduo::net::InetAddress address(ip, port);

    // 创建 TCP 服务器（使用 muduo 库）
    muduo::net::TcpServer server(&m_eventLoop, address, "RpcProvider");

    // 设置连接回调（lambda 表达式绑定）
    server.setConnectionCallback(std::bind(&RpcProvider::OnConnection, this, std::placeholders::_1));

    // 设置消息回调（处理网络数据）
    server.setMessageCallback(std::bind(&RpcProvider::OnMessage, this,
                                        std::placeholders::_1,   // TcpConnectionPtr
                                        std::placeholders::_2,   // Buffer*
                                        std::placeholders::_3)); // Timestamp

    // 设置线程数（I/O 线程与工作线程分离）
    server.setThreadNum(4); // 通常设置为 CPU 核心数

    ZkClient zkCli;
    zkCli.Start();

    for (auto &sp : m_serviceMap)
    {
        std::string service_path = "/" + sp.first;
        zkCli.Create(service_path.c_str(), nullptr, 0);
        for (auto &mp : sp.second.m_methodMap)
        {
            std::string method_path = service_path + "/" + mp.first;
            char method_path_data[128] = {0};
            sprintf(method_path_data, "%s:%d", ip.c_str(), port);
            zkCli.Create(method_path.c_str(), method_path_data, strlen(method_path_data), ZOO_EPHEMERAL);
        }
    }

    // 启动服务
    std::cout << "RpcProvider start service at ip:" << ip << " port:" << port << std::endl;
    server.start();     // 启动监听
    m_eventLoop.loop(); // 进入事件循环
}

// ---------------------------- 网络连接回调 ----------------------------
/**
 * @brief 处理连接状态变化
 * @param conn TCP 连接对象
 *
 * 当连接断开时主动关闭
 */
void RpcProvider::OnConnection(const muduo::net::TcpConnectionPtr &conn)
{
    if (!conn->connected())
    {                     // 连接断开处理
        conn->shutdown(); // 关闭连接（muduo 自动管理资源）
    }
}

// ---------------------------- 核心消息处理 ----------------------------
/**
 * @brief 处理接收到的 RPC 请求
 * @param conn TCP 连接对象
 * @param buffer 接收缓冲区
 * @param 时间戳（未使用）
 *
 * 协议格式：
 * [4字节头部长度] [RPC头部] [参数数据]
 */
void RpcProvider::OnMessage(const muduo::net::TcpConnectionPtr &conn,
                            muduo::net::Buffer *buffer,
                            muduo::Timestamp)
{
    // 获取完整数据包
    std::string recv_buf = buffer->retrieveAllAsString();

    // 解析协议头部长度（存在字节序问题，见改进建议）
    uint32_t header_size = 0;
    recv_buf.copy((char *)&header_size, 4, 0); // 前4字节为头部长度

    // 解析 RPC 协议头
    std::string rpc_header_str = recv_buf.substr(4, header_size);
    mprpc::RpcHeader rpcHeader;
    std::string service_name, method_name;
    uint32_t args_size;

    if (!rpcHeader.ParseFromString(rpc_header_str))
    { // 反序列化协议头
        std::cout << "rpc_header_str parse error: " << rpc_header_str << std::endl;
        return; // 协议错误直接返回
    }
    service_name = rpcHeader.service_name();
    method_name = rpcHeader.method_name();
    args_size = rpcHeader.args_size();

    // 提取参数数据
    std::string args_str = recv_buf.substr(4 + header_size, args_size);

    // 调试输出（建议改为日志级别控制）
    std::cout << "=============== RPC 请求 ===============" << std::endl;
    std::cout << "header_size: " << header_size << std::endl;
    std::cout << "service_name: " << service_name << std::endl;
    std::cout << "method_name: " << method_name << std::endl;
    std::cout << "args_size: " << args_size << std::endl;
    std::cout << "========================================" << std::endl;

    // 服务查找验证
    auto sit = m_serviceMap.find(service_name);
    if (sit == m_serviceMap.end())
    {
        std::cout << "Service not found: " << service_name << std::endl;
        return;
    }

    // 方法查找验证
    auto mit = sit->second.m_methodMap.find(method_name);
    if (mit == sit->second.m_methodMap.end())
    {
        std::cout << "Method not found: " << service_name << ":" << method_name << std::endl;
        return;
    }

    // 准备请求/响应对象
    google::protobuf::Service *service = sit->second.m_service;
    const google::protobuf::MethodDescriptor *method = mit->second;

    // 创建动态消息对象（需要手动管理内存，见改进建议）
    google::protobuf::Message *request = service->GetRequestPrototype(method).New();
    if (!request->ParseFromString(args_str))
    { // 反序列化参数
        std::cout << "Request parse error: " << args_str << std::endl;
        delete request; // 防止内存泄漏
        return;
    }

    google::protobuf::Message *response = service->GetResponsePrototype(method).New();

    // 创建回调闭包（使用 NewCallback 绑定响应发送方法）
    google::protobuf::Closure *done = google::protobuf::NewCallback<RpcProvider,
                                                                    const muduo::net::TcpConnectionPtr &,
                                                                    google::protobuf::Message *>(
        this,
        &RpcProvider::SendRpcResponse, // 回调方法
        conn,                          // 保持连接引用
        response                       // 响应对象所有权转移
    );

    // 调用服务方法（异步处理）
    service->CallMethod(method, nullptr, request, response, done);
}

// ---------------------------- 响应发送方法 ----------------------------
/**
 * @brief 发送 RPC 响应
 * @param conn TCP 连接
 * @param response 响应消息对象
 *
 * 注意：此方法在服务方法执行完成后由闭包触发
 */
void RpcProvider::SendRpcResponse(const muduo::net::TcpConnectionPtr &conn,
                                  google::protobuf::Message *response)
{
    std::string response_str;
    if (response->SerializeToString(&response_str))
    {                             // 序列化响应
        conn->send(response_str); // 通过 muduo 发送数据
    }
    else
    {
        std::cout << "Serialize response failed!" << std::endl;
    }
    conn->shutdown(); // 短连接模式，关闭连接
}
