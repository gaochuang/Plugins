#ifndef SYSTEMD_PLUGIN_HPP
#define SYSTEMD_PLUGIN_HPP

#include "controllableProcess/ControllableProcess.hpp"
#include "plugin/PluginServices.hpp"

#include <memory>

namespace systemdPlugin
{

class SystemdPlugin : public std::enable_shared_from_this<SystemdPlugin>,
                      public commonApi::controllableprocess::ControllableProcess
{
public:
    SystemdPlugin(std::shared_ptr<commonApi::PluginServices>);
    ~SystemdPlugin();

    void setTerminateCb(const TerminateCb& cb) override;
    void notifyReady() override;
    void setHeartbeatCb(const HeartbeatCb& cb) override;
    void heartbeatAck() override;

private:
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
    commonApi::TimerService& timerService;

    bool notifyReadyCalled;
    uint64_t heartbeatInterval;
    SignalState signalState;
    TerminateCb terminateCb;
    HeartbeatCb heartbeatCb;

    void startupTimer();
    void startUpHeartbeatTimerIfWatchdogEnabled();
    void handleHeartbeatTimer();
    void handleSigTerm();
    void callTerminateCb();
};

}

#endif
