#ifndef UDP_LOGGER_PLUGIN_HPP_
#define UDP_LOGGER_PLUGIN_HPP_

#include <string>
#include <memory>
#include <unistd.h>
#include <sys/types.h>

#include <logger/Logger.hpp>
#include <plugin/PluginServices.hpp>

namespace commapisyslog
{

class UDPLoggerPlugin : public  commonApi::logger::Logger
{
public:
    UDPLoggerPlugin(std::shared_ptr<commonApi::PluginServices> pluginService, const std::string& ident, int facility,
                    pid_t pid, size_t queueLimit, const std::string& path, int fd);

    ~UDPLoggerPlugin();
    
    void write(int priority, const char* msg, size_t size) override;
    void writeAsync(int priority, const char* msg, size_t size) override;
    void waitAllWriteAndCompleted()  override;

    static int createUDPLogSocket(const std::string& path);

private:

    enum class Result
    {
        SUCCESS,
        TRY_AGAIN,
        FAILURE,
    };

    void startMonitor();
    void stopMonitor();
    Result trySendImpl(bool block, const std::string& message);
    void appendToQueue(std::string&& message);
    bool sendFromQueue(bool block);
    bool trySend(bool block, const std::string& message);
    bool sendMsgFromQue(bool block);
    bool checkForDropMessages();
    void armTimer();
    void timerCb();
    void eventHandler();
    void addFd(bool monitor);

    std::shared_ptr<commonApi::PluginServices> pluginService;
    commonApi::TimerService& timerService;
    commonApi::FdMonitor& fdMonitor;
    const std::string ident;
    const int facility;
    const pid_t pid;
    const size_t queueLimit;
    const std::string path;
    int fd;
    std::list<std::string> queue;
    size_t droppedMessage;
    bool monitorFlag;
};

}


#endif
