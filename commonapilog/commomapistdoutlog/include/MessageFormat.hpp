#ifndef COMMON_API_MESSAGE_FORMAT_HPP_
#define COMMON_API_MESSAGE_FORMAT_HPP_

#include <string>
#include <syslog.h>
#include <string_view>

namespace commonapistdoutlogger
{

    inline bool isValidPriority(int priority)
    {
        return (priority & ~(LOG_PRIMASK | LOG_FACMASK)) == 0; //(LOG_PRIMASK | LOG_FACMASK) 组合出所有合法的 priority 位掩码。
    }

    class MessageFormatter
    {
    public:
        MessageFormatter(const std::string& prefixFormat);
        
        virtual ~MessageFormatter();

        virtual std::string createMessage(const std::string& indent, pid_t pid, int facility, int priority, const char* message, size_t size);

        MessageFormatter(const MessageFormatter&) = delete;
        MessageFormatter(MessageFormatter&&) = delete;
        MessageFormatter operator=(const MessageFormatter&) = delete;
        MessageFormatter operator=(MessageFormatter&&) = delete;
    private:
        const std::string prefixFormat;

        std::string formatPrefix(int priority, const std::string& ident, pid_t pid, const struct timeval& t, const struct tm& tm);
    };
}

#endif
