#ifndef COMMON_API_DAEMON_PLUGIN_HPP
#define COMMON_API_DAEMON_PLUGIN_HPP

#include "comapi/controllableProcess/controllableProcess.hpp"

#include "plugin/pluginServices.hpp"

#include <memory>

namespace commonapidaemon
{

class Plugin : public std::enable_shared_from_this<Plugin>,
               public reactorFramework::controllableprocess::ControllableProcess
{
public:
    Plugin(std::shared_ptr<reactorFramework::PluginServices>);
    ~Plugin();

    void setTerminateCb(const TerminateCb& cb) override;
private:
    void handleSigTerm();
    void callTerminateCb();

    enum class SignalState
    {
        NONE,
        SIGNAL_RECEIVED,
        SIGNAL_HANDLED,
    };

    std::shared_ptr<reactorFramework::PluginServices> pluginServices;
    reactorFramework::FdMonitor& fdMonitor;
    reactorFramework::signalMonitorService& signalMonitor;
    reactorFramework::CallbackQueueService& callbackQueue;

    SignalState signalState;
    TerminateCb terminateCb;
};

}

#endif
