#include "Plugin.hpp"

#include "plugin/callbackQueueService.hpp"
#include "plugin/fdMonitor.hpp"
#include "plugin/defines.hpp"
#include "plugin/signalMonitorService.hpp"
#include "comapi/controllableProcess/controllableProcessPlugin.hpp"

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

Plugin::Plugin(std::shared_ptr<reactorFramework::PluginServices> pluginServices):
        pluginServices(pluginServices),
        fdMonitor(pluginServices->getFdMonitor()),
        signalMonitor(pluginServices->getSignalMonitor()),
        callbackQueue(pluginServices->getEventCallbackQueue()),
        signalState(SignalState::NONE)
{

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

COMMONAPI_DEFINE_CONTROLLABLEPROCESS_PLUGIN_CREATOR(services)
{
    std::cout << "create controllable Plugin" << std::endl;
    return std::make_shared<Plugin>(services);
}
