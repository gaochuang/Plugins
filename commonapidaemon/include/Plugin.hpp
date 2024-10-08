#ifndef COMMON_API_DAEMON_PLUGIN_HPP
#define COMMON_API_DAEMON_PLUGIN_HPP

#include "controllableProcess/ControllableProcess.hpp"

#include "plugin/PluginServices.hpp"

#include <memory>

namespace commonapidaemon
{

class Plugin : public std::enable_shared_from_this<Plugin>,
               public commonApi::controllableprocess::ControllableProcess
{
public:
    Plugin(std::shared_ptr<commonApi::PluginServices> pluginServices);
    ~Plugin();

    void setTerminateCb(const TerminateCb& cb) override;
    void notifyReady() override;
    void setHeartbeatCb(const HeartbeatCb& cb) override;
    void heartbeatAck() override;
private:
    void handleSigTerm();
    void callTerminateCb();
    void eventHandler();

    enum class SignalState
    {
        NONE,
        SIGNAL_RECEIVED,
        SIGNAL_HANDLED,
    };

    std::shared_ptr<commonApi::PluginServices> pluginServices;
    commonApi::FdMonitor& fdMonitor;
    commonApi::SignalMonitorService& signalMonitor;
    commonApi::CallbackQueueService& callbackQueue;

    SignalState signalState;
    TerminateCb terminateCb;
};

}

#endif
