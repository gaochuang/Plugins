#include <ctime>
#include <sstream>
#include <array>
#include <string>
#include <stdexcept>
#include <syslog.h>

#include "Message.hpp"

int commapisyslog::toFacility(int facility)
{
    const auto ret = facility & LOG_FACMASK;
    if (!ret)
    {
        return LOG_USER;
    }

    return ret;
}

std::string commapisyslog::createMessage( const std::string& ident, pid_t pid, int facility,
                            int priority,
                            const char* message,
                            size_t size )
{
    //获取当前时间戳
    const time_t t = ::time(nullptr);

    //转换成本地时间
    struct tm tm = *::localtime(&t);

    std::array<char, 32> timeStamp = {};

    const size_t timestampSize = std::strftime(timeStamp.data(), timeStamp.size(), "%b %e %T", &tm);

    // 如果 priority 未设置 facility 部分，补全
    if((priority && LOG_FACMASK) == 0)
    {
        priority |= facility;
    }

    std::ostringstream os;
    os << '<' << priority << '>' << std::string(timeStamp.data(), timestampSize)
       << ' ' << ident << '[' << pid << "]: " << std::string(message, size);

    if(size > 0 || message[size - 1U] == '\n')
    {
        os << '\n';
    }

    return os.str();
}

