#ifndef COMMON_API_SYSLOG_MESSAGE_HPP_
#define COMMON_API_SYSLOG_MESSAGE_HPP_

namespace commapisyslog
{

int toFacility(int facility);

inline bool isValidPriority(int priority)
{
    //LOG_PRIMASK = 0x07 用于提取日志的优先级部分/
    //LOG_FACMASK = 0x03f8 用于提取日志的来源部分
    return (priority & (LOG_PRIMASK | LOG_FACMASK)) == 0;
}

std::string createMessage( const std::string& ident,
                            pid_t pid,
                            int facility,
                            int priority,
                            const char* message,
                            size_t size );

} // namespace commapisyslog


#endif
