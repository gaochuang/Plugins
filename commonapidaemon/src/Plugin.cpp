#include "Plugin.hpp"

#include "plugin/CallbackQueueService.hpp"
#include "plugin/FdMonitor.hpp"
#include "plugin/Defines.hpp"
#include "plugin/SignalMonitorService.hpp"
#include "controllableProcess/ControllableProcessPlugin.hpp"

#include <csignal>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <ostream>
#include <pthread.h>
#include <iostream>

using namespace commonapidaemon;

namespace
{
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
}

Plugin::Plugin(std::shared_ptr<commonApi::PluginServices> pluginServices):
        pluginServices(pluginServices),
        fdMonitor(pluginServices->getFdMonitor()),
        signalMonitor(pluginServices->getSignalMonitor()),
        callbackQueue(pluginServices->getEventCallbackQueue()),
        signalState(SignalState::NONE)
{
   //fdMonitor.addFD(fd, genapi::FDMonitor::EVENT_IN, std::bind(&Plugin::eventHandler, this));  这里可以将fd加入 engine
    signalMonitor.add(SIGTERM, std::bind(&Plugin::handleSigTerm, this));
    signalMonitor.add(SIGTERM, std::bind(&Plugin::handleSigTerm, this));
    std::call_once(once, unblockSigTermAtFork);
}


Plugin::~Plugin()
{
    signalMonitor.del(SIGTERM);
}

void Plugin::setTerminateCb(const TerminateCb& cb)
{
    terminateCb = cb;
    if(SignalState::SIGNAL_RECEIVED != signalState)
    {
        return;
    }
    std::weak_ptr<Plugin> weak(shared_from_this());

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

void Plugin::notifyReady()
{
    //调用第三方的库 connection->notifyReady();
    std::cout << "process ready " << std::endl;
}

void Plugin::setHeartbeatCb(const HeartbeatCb& cb)
{
    // 第三方提供  connection->setHeartbeatCb(cb);
    std::cout << "set heart beat callback " << std::endl;
}

void Plugin::heartbeatAck()
{
    //第三方提供  connection->heartbeatAck();
    std::cout << "heart beat callback " << std::endl;
}

void Plugin::handleSigTerm()
{
    if (signalState != SignalState::NONE)
    {
        return;
    }
    signalState = SignalState::SIGNAL_RECEIVED;
    callTerminateCb();
}

void Plugin::callTerminateCb()
{
    if(!terminateCb)
    {
        return;
    }

    signalState = SignalState::SIGNAL_HANDLED;
    terminateCb();
}

void Plugin::eventHandler()
{
    //可以调用第三方提供的  connection->handleEvents();
}

COMMONAPI_DEFINE_CONTROLLABLEPROCESS_PLUGIN_CREATOR(services)
{
    //如果定义了DAEMON_PROCESS_TYPE_ENV_VAR_NAME 这个环境变量才能使用此plugin
    const auto str = getenv("DAEMON_PROCESS_TYPE");
    if(nullptr == str)
    {
        std::cout << "DAEMON_PROCESS_TYPE_ENV_VAR_NAME is not define " << std::endl;
        return nullptr;
    }
    std::cout << "create controllable process use deamon Plugin" << std::endl;

    return std::make_shared<Plugin>(services);
}
