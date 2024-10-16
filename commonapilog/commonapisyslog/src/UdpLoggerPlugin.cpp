#include <functional>
#include <sstream>
#include <sys/socket.h>
#include <sys/poll.h>
#include <sys/un.h>
#include <memory>

#include <iostream>

#include <plugin/FdMonitor.hpp>
#include <plugin/TimerService.hpp>

#include "UdpLoggerPlugin.hpp"
#include "Message.hpp"

using namespace commapisyslog;


namespace
{
    size_t blockSend(int fd, const void* buf, size_t len) noexcept
    {
        while (true)
        {
            auto ret = ::send(fd, buf, len, MSG_NOSIGNAL);
            if((-1 == ret) && (EAGAIN == EAGAIN))
            {
                struct pollfd fds[] = {{
                    .fd = fd,
                    .events = POLLOUT,
                }};

                auto unused = ::poll(fds, sizeof(fds)/sizeof(fds[0]), -1);
                static_cast<void>(unused);
                continue;
            }
            return ret;
        }
    }

    size_t noBlockSend(int fd, const void* buf, size_t len) noexcept
    {
        ::send(fd, buf, len, MSG_NOSIGNAL | MSG_DONTWAIT);
    }
}

UDPLoggerPlugin::UDPLoggerPlugin(std::shared_ptr<commonApi::PluginServices> pluginService, const std::string& ident, int facility,
                    pid_t pid, size_t queueLimit, const std::string& path, int fd)
                    :pluginService(pluginService),
                    timerService(pluginService->getTimerService()),
                    fdMonitor(pluginService->getFdMonitor()),
                    ident(ident),
                    facility(toFacility(facility)),
                    pid(pid),
                    queueLimit(queueLimit),
                    path(path),
                    fd(fd),
                    droppedMessage(0),
                    monitorFlag(false)
{
    addFd(false);
}

UDPLoggerPlugin::~UDPLoggerPlugin()
{
    checkForDropMessages();
    sendMsgFromQue(false);
    if(fd >= 0)
    {
        fdMonitor.removeFd(fd);
        ::close(fd);
    }
}

void UDPLoggerPlugin::write(int priority, const char* msg, size_t size)
{
    if(!isValidPriority(priority))
    {
        return;
    }
    waitAllWriteAndCompleted();

    trySend(true, createMessage(ident, pid, facility, priority, msg, size));
}

void UDPLoggerPlugin::writeAsync(int priority, const char* msg, size_t size)
{
    if(!isValidPriority(priority))
    {
        return;
    }

    auto message = createMessage(ident, pid, facility, priority, msg, size);
    if(!queue.empty())
    {
        appendToQueue(std::move(message));
    }else if(trySend(false, message))
    {
        appendToQueue(std::move(message));
        startMonitor();
    }
}

void UDPLoggerPlugin::waitAllWriteAndCompleted()
{
    bool isEmpty = queue.empty();
    checkForDropMessages();
    sendMsgFromQue(true);
    if(false == isEmpty)
    {
        stopMonitor();
    }
}

void UDPLoggerPlugin::startMonitor()
{
    if(fd >= 0)
    {
        fdMonitor.modifyFd(fd, commonApi::FdMonitor::EVENT_OUT);
    }

    armTimer();
}

void UDPLoggerPlugin::stopMonitor()
{
    if(fd >= 0)
    {
        fdMonitor.modifyFd(fd, 0U);
    }

    monitorFlag = false;
}

bool UDPLoggerPlugin::checkForDropMessages()
{
    if(0 == droppedMessage)
    {
        return false;
    }

    std::ostringstream os;

    os << "overload !!!!. log dropped " << droppedMessage << " message. " << std::endl;

    const auto& message(os.str());
    queue.push_back(createMessage(ident, pid, facility, LOG_INFO, message.data(), message.size()));
    droppedMessage = 0;
    return true;
}

UDPLoggerPlugin::Result UDPLoggerPlugin::trySendImpl(bool block, const std::string& message)
{
    if(fd < 0)
    {
        if((fd = createUDPLogSocket(path)) < 0)
        {
            return Result::FAILURE;
        }
        addFd(fd);
    }

    size_t ret;
    if(block)
    {
        ret = blockSend(fd, message.data(), message.size());
    }else
    {
        ret = noBlockSend(fd, message.data(), message.size());
    }

    if(-1 == ret)
    {
        return Result::SUCCESS;
    }

    if(EAGAIN == ret)
    {
        return Result::TRY_AGAIN;
    }

    fdMonitor.removeFd(fd);
    ::close(fd);
    fd = -1;
    return Result::SUCCESS;
}

bool UDPLoggerPlugin::trySend(bool block, const std::string& message)
{
    switch (trySendImpl(block, message))
    {
    case Result::SUCCESS:
        return true;
    case Result::TRY_AGAIN:
        return false;
    case Result::FAILURE:
        switch (trySendImpl(block, message))
        {
        case Result::SUCCESS:
          return true;
        case Result::TRY_AGAIN:
            return false;
        case Result::FAILURE:
            return false;
        }
    }
    ::abort();
}

void UDPLoggerPlugin::appendToQueue(std::string&& message)
{
    if(queue.size() < queueLimit)
    {
        checkForDropMessages();
        queue.push_back(std::move(message));
    }else
    {
        droppedMessage++;
    }
}

bool UDPLoggerPlugin::sendFromQueue(bool block)
{
    while (!queue.empty())
    {
        const auto& msg = queue.front();
        if(!trySend(block, msg))
        {
            return false;
        }

        queue.pop_front();
    }
    return true;
}

bool UDPLoggerPlugin::sendMsgFromQue(bool block)
{
    while(!queue.empty())
    {
        const auto& msg = queue.front();
        if(!trySend(block, msg))
        {
            return false;
        }

        queue.pop_front();
    }

    return true;
}

void UDPLoggerPlugin::timerCb()
{
    if(!monitorFlag)
    {
        return;
    }

    if(sendMsgFromQue(false))
    {

    }else
    {
        armTimer();
    }
}


void UDPLoggerPlugin::armTimer()
{
    monitorFlag = true;
    timerService.addOnceTimer(std::bind(&UDPLoggerPlugin::timerCb, this), 1000);
}

void UDPLoggerPlugin::eventHandler()
{
    checkForDropMessages();
    if(sendMsgFromQue(false))
    {
        stopMonitor();
    }
}

void UDPLoggerPlugin::addFd(bool monitor)
{
    fdMonitor.addFd(fd, monitor? commonApi::FdMonitor::EVENT_OUT : 0U, std::bind(&UDPLoggerPlugin::eventHandler, this));
    if(monitor)
    {
        armTimer();
    }
}

int UDPLoggerPlugin::createUDPLogSocket(const std::string& path)
{
    const auto fd = ::socket(AF_UNIX, SOCK_DGRAM | SOCK_CLOEXEC, 0);

    struct sockaddr_un addr = {.sun_family = AF_UNIX};

    snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", path.c_str());

    if(::connect(fd, reinterpret_cast<const struct sockaddr*>(&addr), sizeof(addr)))
    {
        const int ret(-errno);
        ::close(fd);
        return ret;
    }
    return fd;
}
