#pragma once
#define _DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR

#include "sign.h"
#include "../include/cpp-httplib/httplib.h"

class Server
{
private:
    httplib::Server svr;
    std::atomic<uint64_t> counter = 0;

private:
    std::string GetSign(const std::string_view &cmd, const std::string_view &src, const int seq);

public:
    void Init();
    bool Run(const std::string &ip, int port);
};