#ifndef HTTP_HEART_BEAT_PLUGIN_HPP
#define HTTP_HEART_BEAT_PLUGIN_HPP

#include "controllableProcess/ControllableProcess.hpp"
#include "plugin/PluginServices.hpp"
#include "SocketAddress.hpp"

#include <unordered_map>
#include <memory>

namespace commapihttpheartbeat
{

class HttpHeartbeatPlugin : public std::enable_shared_from_this<HttpHeartbeatPlugin>,
                            public commonApi::controllableprocess::ControllableProcess
{
public:
    HttpHeartbeatPlugin(std::shared_ptr<commonApi::PluginServices> pluginServices, const SocketAddress& socketAddress);
    ~HttpHeartbeatPlugin();

    //当该服务结束，需要调用这个回调
     void setTerminateCb(const TerminateCb& cb) override;
    //通知其它服务本服务已经ready，可以进行后续，比如跳服务等
     void notifyReady() override;
    //设置心跳的回调函数
    void setHeartbeatCb(const HeartbeatCb& cb) override;
    //心跳回复
    void heartbeatAck() override;
private:
    class Client;
    friend class Client;

    using ClientNum = unsigned int;

    void eventHandler();
    void handleSigTerm();
    void callTerminateCb();
    void removeClient(ClientNum index);

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

    int fd;
    SocketAddress boundAddr;
    bool notifyReadyCalled;
    SignalState signalState;
    TerminateCb terminalCb;
    HeartbeatCb heartbeatCb;
    ClientNum nextClient;
    std::unordered_map<ClientNum, Client> clients;
};

}

#endif
