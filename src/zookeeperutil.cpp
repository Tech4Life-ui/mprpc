#include "zookeeperutil.h"
#include "mprpcapplication.h"
#include <iostream>

void global_watcher(zhandle_t *zh, int type, int state, const char *path, void *watcherCtx)
{
    if (type == ZOO_SESSION_EVENT)
    {
        if (state == ZOO_CONNECTED_STATE)
        {
            sem_t *sem = (sem_t *)zoo_get_context(zh);
            sem_post(sem);
        }
    }
}

ZkClient::ZkClient() : m_zhandle(nullptr)
{
}

ZkClient::~ZkClient()
{
    if (m_zhandle == nullptr)
        zookeeper_close(m_zhandle);
}

void ZkClient::Start()
{
    std::string host = MprpcApplication::getInstance().GetConfig().Load("zookeeperip");
    std::string port = MprpcApplication::getInstance().GetConfig().Load("zookeeperport");
    std::string connstr = host + ":" + port;

    m_zhandle = zookeeper_init(connstr.c_str(), global_watcher, 30000, nullptr, nullptr, 0);
    if (m_zhandle == nullptr)
    {
        std::cout << "zookeeper_init error" << std::endl;
        exit(EXIT_FAILURE);
    }

    sem_t sem;
    sem_init(&sem, 0, 0);
    zoo_set_context(m_zhandle, &sem);

    sem_wait(&sem);
    std::cout << "zookeeper_init success!!!" << std::endl;
}

void ZkClient::Create(const char *path, const char *data, int datalen, int state)
{
    char path_buffer[128];
    int bufferlen = sizeof(path_buffer);
    int flag = zoo_exists(m_zhandle, path, 0, nullptr);
    if (flag == ZNONODE) // 表示节点不存在
    {
        // 创建指定的path的znode节点
        flag = zoo_create(m_zhandle, path, data, datalen, &ZOO_OPEN_ACL_UNSAFE, state, path_buffer, bufferlen);
        if (flag == ZOK)
        {
            std::cout << "znode create success... path:" << path << std::endl;
        }
        else
        {
            std::cout << "znode create error... path:" << path << std::endl;
            exit(EXIT_FAILURE);
        }
    }
}

std::string ZkClient::GetData(const char *path)
{
    char buf[64];
    int bufferlen = sizeof(buf);
    int flag = zoo_get(m_zhandle, path, 0, buf, &bufferlen, nullptr);
    if (flag != ZOK)
    {
        std::cout << "zoo_get error" << std::endl;

        return "";
    }
    else
    {
        return buf;
    }
    return "";
}
