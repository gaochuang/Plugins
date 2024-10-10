#include <mutex>
#include <csignal>
#include <string>
#include <iostream>
#include <sstream>
#include <thread>
#include <cstdlib>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <cerrno>
#include <cstring>
#include <climits>
#include <memory>

#include "HttpHeartbeatPlugin.hpp"

#include "plugin/CallbackQueueService.hpp"
#include "plugin/FdMonitor.hpp"
#include "plugin/Defines.hpp"
#include "plugin/SignalMonitorService.hpp"
#include "controllableProcess/ControllableProcessPlugin.hpp"

using namespace commapihttpheartbeat;

namespace
{
    constexpr const char* BIND_ADDRESS_ENV_VAR_NAME = "COMMON_API_HTTP_HEARTBEAT_BIND_ADDRESS";
    constexpr const char* TRY_COUNT_ENV_VAR_NAME = "COMMON_API_HTTP_HEARTBEAT_TRY_COUNT";
    constexpr int DEFAULT_TRY_COUNT = 10;
    constexpr const char* TRY_DELAY_ENV_VAR_NAME = "COMMON_API_HTTP_HEARTBEAT_TRY_DELAY";
    constexpr int DEFAULT_TRY_DELAY = 100;

    std::once_flag once;

    void unblockSigTerm() noexcept
    {
        sigset_t sset;
        sigemptyset(&sset);
        sigaddset(&sset, SIGTERM);

        pthread_sigmask(SIG_UNBLOCK, &sset, nullptr);
    }

    void unblockSigTermAtFork() noexcept
    {
        pthread_atfork(nullptr, nullptr, unblockSigTerm);
    }

    SocketAddress createSocketAddr(const std::string& str, int tryCount, int tryDelayMs)
    {
        while (true)
        {
            tryCount--;
            try
            {
                return SocketAddress::create(str);
            }
            catch(const std::exception& e)
            {
                if(tryCount <= 0)
                {
                    throw;
                }
                std::cout << "retry after " << tryDelayMs << " ms..."<<std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(tryDelayMs));
            }
        }
    }

    int getValueFromEnv(const char* envName, int defaultValue)
    {
        const auto str = ::getenv(envName);
        if(nullptr == str)
        {
            std::cout << envName << " is not defined, will used default value: " << defaultValue << std::endl;
            return defaultValue;
        }

        try
        {
            int val = std::stoi(str);
            std::cout << envName << "value is " << val << std::endl;
            return val;
        }

        catch(const std::invalid_argument& e)
        {
            std::cerr << " is defined but not a valid integer, will use default value: " << defaultValue << std::endl;
        }

        catch (const std::out_of_range& e)
        {
            std::cout << envName << " value is out of range, will use default value: " << defaultValue << std::endl;
        }
        return defaultValue;
    }

    void getHostName(std::ostream& os)
    {
        char buffer[HOST_NAME_MAX + 1] = {};
        ::gethostname(buffer, sizeof(buffer) - 1);
        os << buffer;
    }

  ////ip:%h:9000 :  h -> hostname
    std::string expandVariables(const std::string& str)
    {
        std::ostringstream os;
        for(auto i = str.begin(); i != str.end(); i++)
        {
            if(*i == '%')
            {
                i++;
                if(i == str.end())
                {
                    os << '%';
                    break;
                }else if(*i == 'h')
                {
                    getHostName(os);
                }else
                {
                    os << *i;
                }
            }else
            {
                os << *i;
            }
        }

        return os.str();
    }

}

class HttpHeartbeatPlugin::Client
{
public:
    Client(HttpHeartbeatPlugin& parent, int fd, ClientNum num);
    ~Client();

    void heartbeatAck();

    Client(const Client&) = delete;
    Client(Client&&) = delete;

    Client& operator=(const Client&) = delete;
    Client& operator=(Client&&) = delete;
private:
   void eventHandler();
   void sendResponseAndClose();
   void close();

   HttpHeartbeatPlugin& parent;
   int fd;
   ClientNum clientNum;
   bool heartbeatPending;
};

HttpHeartbeatPlugin::Client::Client(HttpHeartbeatPlugin& parent, int fd, ClientNum num)
                            :parent(parent),
                            fd(fd),
                            clientNum(num),
                            heartbeatPending(false)
{
    const int val(1);

    ::setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &val, sizeof(val));

    parent.fdMonitor.addFd(fd, commonApi::FdMonitor::EVENT_IN, std::bind(&Client::eventHandler, this));
}


HttpHeartbeatPlugin::Client::~Client()
{
    if(fd >= 0)
    {
        parent.fdMonitor.removeFd(fd);
        ::close(fd);
    }
}

void HttpHeartbeatPlugin::Client::heartbeatAck()
{
    if((heartbeatPending) && (fd >= 0))
    {
        sendResponseAndClose();
        heartbeatPending = false;
    }
}

void HttpHeartbeatPlugin::Client::eventHandler()
{
    char request[1024];
    ssize_t ssize = ::recv(fd, request, sizeof(request), 0);

    if(-1 == ssize)
    {
        close();
    }else if(parent.heartbeatCb)
    {
        heartbeatPending = true;
        parent.heartbeatCb();
    }else
    {
        sendResponseAndClose();
    }
}

void HttpHeartbeatPlugin::Client::close()
{
    parent.fdMonitor.removeFd(fd);
    ::close(fd);
    fd = -1;
    parent.callbackQueue.post(std::bind(&HttpHeartbeatPlugin::removeClient, &parent, clientNum));
}

void HttpHeartbeatPlugin::Client::sendResponseAndClose()
{
    const char response[] = "HTTP/1.0 200 OK\r\n\r\n";
    const auto ssize = ::send(fd, response, sizeof(response) - 1, MSG_NOSIGNAL);

    static_cast<void>(ssize);
    close();
}

HttpHeartbeatPlugin::HttpHeartbeatPlugin(std::shared_ptr<commonApi::PluginServices> pluginServices, const SocketAddress& socketAddress)
                    :pluginServices(pluginServices),
                    fdMonitor(pluginServices->getFdMonitor()),
                    signalMonitor(pluginServices->getSignalMonitor()),
                    callbackQueue(pluginServices->getEventCallbackQueue()),
                    fd(::socket(socketAddress.getAddress()->sa_family,
                    SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP)),
                    notifyReadyCalled(false),
                    signalState(SignalState::NONE),
                    nextClient(0U)
{
    const int val(1);
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    if (-1 == ::bind(fd, socketAddress.getAddress(), socketAddress.getSize()))
    {
        std::ostringstream oss;

        oss << "bind failed " << std::strerror(errno) << std::endl;
        throw oss;
    }

    ::getsockname(fd, boundAddr.getAddress(), boundAddr.getSizePointer());

    std::cout << "Bound fd " <<  static_cast<int>(fd) << std::endl;

    signalMonitor.add(SIGTERM, std::bind(&HttpHeartbeatPlugin::handleSigTerm, this));

    std::call_once(once, unblockSigTermAtFork);
}

HttpHeartbeatPlugin::~HttpHeartbeatPlugin()
{
    clients.clear();
    signalMonitor.del(SIGTERM);
    if(notifyReadyCalled)
    {
        fdMonitor.removeFd(fd);
    }
    ::close(fd);
}

void HttpHeartbeatPlugin::notifyReady()
{
    if(!notifyReadyCalled)
    {
       if(-1 == ::listen(fd, 10))
       {
            std::ostringstream oss;
            oss << "listen failed " << std::strerror(errno) << std::endl;
            throw oss;
       }

       fdMonitor.addFd(fd, commonApi::FdMonitor::EVENT_IN, std::bind(&HttpHeartbeatPlugin::eventHandler, this));
    }

    notifyReadyCalled = true;
}

void HttpHeartbeatPlugin::setTerminateCb(const TerminateCb& cb)
{
    terminalCb = cb;

    if(signalState != SignalState::SIGNAL_RECEIVED)
    {
        return;
    }

    std::weak_ptr<HttpHeartbeatPlugin> weak(shared_from_this());

    callbackQueue.post(
        [weak]()
        {
            if(auto plugin = weak.lock())
            {
                plugin->callTerminateCb();
            }
        }
    );
}

void HttpHeartbeatPlugin::setHeartbeatCb(const HeartbeatCb& cb)
{
    heartbeatCb = cb;
}

void HttpHeartbeatPlugin::heartbeatAck()
{
    for(auto &i : clients)
    {
        i.second.heartbeatAck();;
    }
}

void HttpHeartbeatPlugin::eventHandler()
{
    while (true)
    {
        const auto socketFd = ::accept4(fd, nullptr, nullptr, SOCK_NONBLOCK | SOCK_CLOEXEC);
        if(socketFd >= 0)
        {
            const auto clientNum = nextClient++;

            clients.emplace(std::piecewise_construct, std::forward_as_tuple(clientNum), std::forward_as_tuple(*this, socketFd, clientNum));
        }else
        {
            break;
        }
    }
}

void HttpHeartbeatPlugin::handleSigTerm()
{
    if(signalState != SignalState::NONE)
    {
        return;
    }

    signalState = SignalState::SIGNAL_RECEIVED;
    callTerminateCb();
}


void HttpHeartbeatPlugin::callTerminateCb()
{
    if(nullptr == terminalCb)
    {
        return;
    }
    signalState = SignalState::SIGNAL_HANDLED;
    terminalCb();
}

void HttpHeartbeatPlugin::removeClient(ClientNum index)
{
    clients.erase(index);
}

COMMONAPI_DEFINE_CONTROLLABLEPROCESS_PLUGIN_CREATOR(services)
{
    const auto str = ::getenv(BIND_ADDRESS_ENV_VAR_NAME);
    if(nullptr == str)
    {
        std::cout << BIND_ADDRESS_ENV_VAR_NAME << " is not defined " << std::endl;
        return nullptr;
    }

    const auto socketAddress = createSocketAddr(expandVariables(str), getValueFromEnv(TRY_COUNT_ENV_VAR_NAME, DEFAULT_TRY_COUNT), getValueFromEnv(TRY_DELAY_ENV_VAR_NAME, DEFAULT_TRY_DELAY));

    return std::make_shared<HttpHeartbeatPlugin>(services, socketAddress);
}
