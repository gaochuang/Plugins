#ifndef TCP_LOGGER_PLUGIN_HPP_
#define TCP_LOGGER_PLUGIN_HPP_
#include <string>
#include <memory>
#include <unistd.h>
#include <sys/types.h>
#include <vector>

#include <logger/Logger.hpp>
#include <plugin/PluginServices.hpp>

namespace commapisyslog
{

class TCPLoggerPlugin : public commonApi::logger::Logger
{
public:
    TCPLoggerPlugin(std::shared_ptr<commonApi::PluginServices> pluginService, const std::string& ident, int facility,
                    pid_t pid, size_t queueLimit, const std::string& path, int fd);
    ~TCPLoggerPlugin();

    void write(int priority, const char* msg, size_t size) override;
    void writeAsync(int priority, const char* msg, size_t size) override;
    void waitAllWriteAndCompleted()  override;

    static int createTCPLogSocket(const std::string& path);
private:
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
    std::vector<struct iovec> iov;
    size_t droppedMessage;
    bool monitorFlag;

    void addFd(bool monitor);
    void eventHandler();
    void queueToIov();
    ssize_t trySendIov(bool block);
    void appendToQueue(bool force, std::string&& message);
    bool sendFromQueue(bool block);
    bool checkForDroppedMessages();
    void timerCb();
    void armTimer();
    void startMonitor();
    void stopMonitor();
    bool createSocket();
    void closeSocket();
};

}

#endif
