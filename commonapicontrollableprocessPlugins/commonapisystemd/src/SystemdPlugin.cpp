#include "SystemdPlugin.hpp"

#include "plugin/CallbackQueueService.hpp"
#include "plugin/TimerService.hpp"
#include "plugin/FdMonitor.hpp"
#include "plugin/Defines.hpp"
#include "plugin/SignalMonitorService.hpp"
#include "controllableProcess/ControllableProcessPlugin.hpp"

#include <mutex>
#include <csignal>
#include <systemd/sd-daemon.h>
#include <iostream>

using namespace systemdPlugin;

namespace 
{
    std::once_flag once;

    void unblockSignalTerminal() noexcept
    {
        sigset_t sset;
        sigemptyset(&sset);
        sigaddset(&sset, SIGTERM);
        pthread_sigmask(SIG_UNBLOCK, &sset, nullptr);
    }

    void unblockSignalTerminalAtFork()
    {
        pthread_atfork(nullptr, nullptr, unblockSignalTerminal);
    }

    //短于 watchdog 超时时间的一半，避免 systemd 误认为服务未响应
    uint64_t getShorterHeartbeatIntervalThanWatchdogTimeout(uint64_t watchdogTimeout)
    {
        const uint64_t half(watchdogTimeout / 2U);
        if (half > 3000000)
        {
            return 3000000;
        }else
        {
            return half;
        }
    }
}

SystemdPlugin::SystemdPlugin(std::shared_ptr<commonApi::PluginServices> pluginServices):
                pluginServices(pluginServices),
                fdMonitor(pluginServices->getFdMonitor()),
                signalMonitor(pluginServices->getSignalMonitor()),
                callbackQueue(pluginServices->getEventCallbackQueue()),
                timerService(pluginServices->getTimerService()),
                notifyReadyCalled(false)

{
    startUpHeartbeatTimerIfWatchdogEnabled();
    signalMonitor.add(SIGTERM, std::bind(&SystemdPlugin::handleSigTerm, this));
    std::call_once(once, unblockSignalTerminalAtFork);
}

void SystemdPlugin::handleHeartbeatTimer()
{
    startupTimer();
    if(heartbeatCb)
    {
        heartbeatCb();
    }else
    {
        heartbeatAck();
    }
}

void SystemdPlugin::startupTimer()
{
    timerService.addOnceTimer(std::bind(&SystemdPlugin::handleHeartbeatTimer, this), (heartbeatInterval/1000));
}

void SystemdPlugin::startUpHeartbeatTimerIfWatchdogEnabled()
{
    uint64_t usec;
    int ret = sd_watchdog_enabled(false, &usec);

    std::cout << "sd_watchdog_enabled(): " << ret << std::endl;
    if(ret <= 0)
    {
        return;
    }

    heartbeatInterval = getShorterHeartbeatIntervalThanWatchdogTimeout(usec);

    startupTimer();

}

void SystemdPlugin::heartbeatAck()
{
    sd_notify(false, "WATCHDOG=1");
}

void SystemdPlugin::setTerminateCb(const TerminateCb& cb)
{
    terminateCb = cb;
    if(signalState != SignalState::SIGNAL_RECEIVED)
    {
        return;
    }

    std::weak_ptr<SystemdPlugin> weak(shared_from_this());

    callbackQueue.post([weak]()
                    {
                        if(auto plugin = weak.lock())
                        {
                            plugin->callTerminateCb();
                        }
                    });
}

void SystemdPlugin::notifyReady()
{
    if(notifyReadyCalled)
    {
        return;
    }
    int ret = sd_notify(false, "READY=1");
    std::cout << "sd_notify(\"READY=1\"): " << ret << std::endl;
    notifyReadyCalled = true;
}

void SystemdPlugin::setHeartbeatCb(const HeartbeatCb& cb)
{
    heartbeatCb = cb;
}

void SystemdPlugin::callTerminateCb()
{
    if(!terminateCb)
    {
        return;
    }

    signalState = SignalState::SIGNAL_HANDLED;
    terminateCb();
}

void SystemdPlugin::handleSigTerm()
{
    if(signalState != SignalState::NONE)
    {
        return;
    }

    signalState = SignalState::SIGNAL_RECEIVED;

    callTerminateCb();
}

COMMONAPI_DEFINE_CONTROLLABLEPROCESS_PLUGIN_CREATOR(services)
{
    std::cout << "create controllable process use systemd Plugin" << std::endl;
    return std::make_shared<SystemdPlugin>(services);
}
