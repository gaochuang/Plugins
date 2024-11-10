#ifndef COMMON_API_MESSAGE_ROUTER_HPP_
#define COMMON_API_MESSAGE_ROUTER_HPP_

#include <logger/Logger.hpp>
#include <plugin/PluginServices.hpp>

#include "Configuration.hpp"
#include "LogWriter.hpp"

#include <array>

namespace commonapistdoutlogger
{
    class MessageFormatter;

    class MessageRouter : public commonApi::logger::Logger
    {
    public:
      MessageRouter(std::unique_ptr<MessageFormatter> messageFormatter,
                    const std::string& ident,
                    int facility,
                    pid_t pid,
                    Configuration&& configuration,
                    std::unique_ptr<LogWriter> logger);

     MessageRouter(std::unique_ptr<MessageFormatter> messageFormatter,
                   const std::string& ident,
                   int facility,
                   pid_t pid,
                   Configuration&& configuration,
                   std::unique_ptr<LogWriter> stdoutLogger,
                   std::unique_ptr<LogWriter> stderrLogger);
  

    ~MessageRouter() = default;

    void write(int priority, const char* message, size_t size) override;

    void writeAsync(int priority, const char* message, size_t size) override;

    void waitAllWriteAndCompleted() override;

    enum class MessageTarget
    {
        DROPPED = 0,
        STDERR,
        STDOUT
    };

    using MessageTargets = std::array<MessageTarget, 255>;
    private:
        MessageTargets messageTargets;
        std::unique_ptr<MessageFormatter> messageFormatter;
        const std::string ident;
        const int defaultFacility;
        const pid_t pid;
        Configuration configuration;
        std::unique_ptr<LogWriter> stdoutLogger;
        std::unique_ptr<LogWriter> stderrLogger;

        MessageTarget getMessageTarget(int priority) const noexcept;
        bool isStderrMessage(int messagePriority) const noexcept;
        std::string createMessage(int priority, const char* message, size_t size);
    };
    
}


#endif
