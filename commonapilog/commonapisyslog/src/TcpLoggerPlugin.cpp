#include <sstream>
#include <sys/socket.h>
#include <sys/poll.h>
#include <sys/un.h>
#include <memory>
#include <algorithm>
#include <cerrno>
#include <climits>
#include <sstream>
#include <functional>

#include "TcpLoggerPlugin.hpp"
#include "Message.hpp"
#include <plugin/FdMonitor.hpp>
#include <plugin/TimerService.hpp>

using namespace commapisyslog;

namespace
{
    ssize_t blockSend(int fd, const struct msghdr* msg) noexcept
    {
        while (true)
        {
            const auto ret = ::sendmsg(fd, msg, MSG_NOSIGNAL);
            if ((ret == -1) && (errno == EAGAIN))
            {
                struct pollfd fds[] = {{
                    .fd = fd,
                    .events = POLLOUT,
                }};
                const auto unused(::poll(fds, sizeof(fds) / sizeof(fds[0]), -1));
                static_cast<void>(unused);
                continue;
            }
            return ret;
        }
    }

    ssize_t nonBlockSend(int fd, const struct msghdr* msg) noexcept
    {
        return ::sendmsg(fd, msg, MSG_NOSIGNAL | MSG_DONTWAIT);
    }
}

TCPLoggerPlugin::TCPLoggerPlugin(std::shared_ptr<commonApi::PluginServices> pluginService, const std::string& ident, int facility, 
                                 pid_t pid, size_t queueLimit, const std::string& path, int fd)
                                 :pluginService(pluginService),
                                 timerService(pluginService->getTimerService()),
                                 fdMonitor(pluginService->getFdMonitor()),
                                 ident(ident),
                                 facility(facility),
                                 pid(pid),
                                 queueLimit(queueLimit),
                                 path(path),
                                 fd(fd),
                                 droppedMessage(0),
                                 monitorFlag(false)
{
    addFd(false);
}

TCPLoggerPlugin::~TCPLoggerPlugin()
{
    if(fd >= 0)
    {
        closeSocket();
    }
}


void TCPLoggerPlugin::write(int priority, const char* msg, size_t size)
{
    if(!isValidPriority(priority))
    {
        return;
    }
     const bool wasEmpty = queue.empty();
    appendToQueue(true, createMessage(ident, pid, facility, priority, msg, size));
    sendFromQueue(true);
    if (!wasEmpty)
    {
        stopMonitor();
    }

}

void TCPLoggerPlugin::writeAsync(int priority, const char* msg, size_t size)
{
    if(!isValidPriority(priority))
    {
        return;
    }
   
    const bool wasEmpty = queue.empty();

    appendToQueue(false, createMessage(ident, pid, facility, priority, msg, size));
    if(wasEmpty && (!sendFromQueue(false)))
    {
        startMonitor();
    }

}
void TCPLoggerPlugin::waitAllWriteAndCompleted()
{
    checkForDroppedMessages();
    if(!queue.empty())
    {
        sendFromQueue(true);
        stopMonitor();
    }
}

void TCPLoggerPlugin::eventHandler()
{
    checkForDroppedMessages();
    if(sendFromQueue(false))
    {
        stopMonitor();
    }
}

void TCPLoggerPlugin::addFd(bool monitor)
{
    fdMonitor.addFd(fd, monitor? commonApi::FdMonitor::EVENT_OUT : 0U, std::bind(&TCPLoggerPlugin::eventHandler, this));
}

void TCPLoggerPlugin::queueToIov()
{
    //IOV_MAX 1024
    size_t ioCount = std::min(static_cast<size_t>(IOV_MAX), queue.size());
    iov.resize(ioCount);

    auto start = queue.begin();

    for (size_t index = 0; index < ioCount; ++index, ++start)
    {
        iov[index].iov_base = const_cast<char*>(start->data());
        iov[index].iov_len = start->size();
    }
}

ssize_t TCPLoggerPlugin::trySendIov(bool block)
{
    struct msghdr msg = {
        nullptr,
        0,
        const_cast<struct iovec*>(iov.data()),
        iov.size(),
        nullptr,
        0,
        0,
    };

    ssize_t ret;
    if (block)
        ret = blockSend(fd, &msg);
    else
        ret = nonBlockSend(fd, &msg);
    if (ret >= 0)
    {
        size_t bytesSent(static_cast<size_t>(ret));
        while (bytesSent > 0U)
        {
            if (bytesSent < queue.front().size())
            {
                queue.front() = queue.front().substr(bytesSent);
                break;
            }
            bytesSent -= queue.front().size();
            queue.pop_front();
        }
    }

    return ret;
}

void TCPLoggerPlugin::appendToQueue(bool force, std::string&& message)
{
    if( force || queue.size() < queueLimit)
    {
        checkForDroppedMessages();
        queue.push_back(std::move(message));
    }else
    {
        droppedMessage++;
    }
}

bool TCPLoggerPlugin::sendFromQueue(bool block)
{
    if((fd < 0) && (!createSocket()))
    {
        queue.clear();
        return true;
    }

    queueToIov();

    for(auto i = 0; i < 2; i++)
    {
        if(trySendIov(block) >= 0)
        {
            return queue.empty();
        }

        if(EAGAIN == errno)
        {
            return false;
        }
        closeSocket();

        if(!createSocket())
        {
            break;
        }
    }

    armTimer();
    queue.clear();

    return true;
}

bool TCPLoggerPlugin::checkForDroppedMessages()
{
    if(droppedMessage == 0)
    {
        return false;
    }

    std::ostringstream os;
    os << "overload logger need drop " << droppedMessage << std::endl;
    
    const auto& message(os.str());
    queue.push_back(commapisyslog::createMessage(ident, pid ,facility, LOG_INFO, message.data(), message.size()));
    droppedMessage = 0;
    return true;
}

void TCPLoggerPlugin::timerCb()
{
    if(fd >= 0) 
    {
        return ;
    }

    if((fd = createTCPLogSocket(path)) < 0)
    {
        armTimer();
    }else
    {
        addFd(!queue.empty());
    }
}

void TCPLoggerPlugin::armTimer()
{
    monitorFlag = true;
    timerService.addOnceTimer(std::bind(&TCPLoggerPlugin::timerCb, this), 1000);
}

void TCPLoggerPlugin::startMonitor()
{
    if(fd >= 0)
    {
        fdMonitor.modifyFd(fd, commonApi::FdMonitor::EVENT_OUT);
        armTimer();
    }
}

void TCPLoggerPlugin::stopMonitor()
{
    if(fd >= 0)
    {
        fdMonitor.modifyFd(fd, 0U);
        monitorFlag = false; 
    }
}

bool TCPLoggerPlugin::createSocket()
{
    if((fd = createTCPLogSocket(path)) < 0)
    {
        return false;
    }
    addFd(false);
    return true;
}

void TCPLoggerPlugin::closeSocket()
{
    fdMonitor.removeFd(fd);
    ::close(fd);
    fd = -1;
}

int TCPLoggerPlugin::createTCPLogSocket(const std::string& path)
{
    const int fd = ::socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
    struct sockaddr_un sun = {.sun_family = AF_UNIX};
    snprintf(sun.sun_path, sizeof(sun.sun_path), "%s", path.c_str());
    if (::connect(fd, reinterpret_cast<const struct sockaddr*>(&sun), sizeof(sun)) == -1)
    {
        const int ret(-errno);
        ::close(fd);
        return ret;
    }
    return fd;
}
