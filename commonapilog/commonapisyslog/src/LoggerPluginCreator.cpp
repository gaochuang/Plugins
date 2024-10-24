#include <logger/Logger.hpp>
#include <logger/LoggerPlugin.hpp>
#include "UdpLoggerPlugin.hpp"
#include "TcpLoggerPlugin.hpp"

#include <string>
#include <iostream>
#include <memory>

using namespace commapisyslog;

namespace
{
    size_t getLogQueueLimit()
    {
        const static size_t DEFAULT_SIZE(128U);
        const auto str = ::getenv("COMMON_API_LOGGER_QUEUE_LIMIT");
        if(nullptr == str)
        {
            return DEFAULT_SIZE;
        }
        try
       {
           return std::stoul(str);
       }
       catch(const std::invalid_argument& e)
       {
            return DEFAULT_SIZE;
       }
    }

    std::string getPath()
    {
        const auto val = ::getenv("COMMON_API_SYSLOG_DEV_PATH");
        if(nullptr != val)
        {
            std::cout << "syslog path is " << val << std::endl;
            return val;
        }
        return "/dev/log";
    }

}

COMMONAPI_DEFINE_LOGGER_PLUGIN_CREATOR(services, params)
{
    const std::string path = getPath();
    auto fd = UDPLoggerPlugin::createUDPLogSocket(path);

    if(fd < 0)
    {
        fd = TCPLoggerPlugin::createTCPLogSocket(path);
        if(fd < 0)
        {
            std::cerr << "unable connect to " << path << std::endl;
            return nullptr;
        }
    }

    const auto size = getLogQueueLimit();

    return std::make_shared<UDPLoggerPlugin>(services, params.indent, params.facility, getpid(), size, path, fd);
}
