#pragma once
#include "mprpcconfig.h"
#include "mprpccontroller.h"
#include "mprpcchannel.h"
#include "rpcprovider.h"

class MprpcApplication
{
public:
    static void Init(int argc, char **argv);
    static MprpcApplication &getInstance();
    static MprpcConfig &GetConfig();

private:
    static MprpcConfig m_config;

    MprpcApplication() {}
    MprpcApplication(const MprpcApplication &) = delete;
    MprpcApplication(MprpcApplication &&) = delete;
};