#include <sys/time.h>
#include <cstdio>
#include <iomanip>
#include <time.h>
#include <unistd.h>

#include "MessageFormat.hpp"
#include "Utils.hpp"

using namespace commonapistdoutlogger;

namespace
{

    bool endsWithNewLine(const char* message, size_t size) noexcept
    {
        return ((size > 0) && (message[size - 1U] == '\n'));
    }


    std::string facilityToName(int facility) noexcept
    {
        switch (facility & LOG_FACMASK)
        {
        case LOG_AUTH:
            return "auth";
        case LOG_AUTHPRIV:
            return "authpriv";
        case LOG_CRON:
            return "cron";
        case LOG_DAEMON:
            return "daemon";
        case LOG_FTP:
            return "ftp";
        case LOG_KERN:
            return "kern";
        case LOG_LOCAL0:
            return "local0";
        case LOG_LOCAL1:
            return "local1";
        case LOG_LOCAL2:
            return "local2";
        case LOG_LOCAL3:
            return "local3";
        case LOG_LOCAL4:
            return "local4";
        case LOG_LOCAL5:
            return "local5";
        case LOG_LOCAL6:
            return "local6";
        case LOG_LOCAL7:
            return "local7";
        case LOG_LPR:
            return "lpr";
        case LOG_MAIL:
            return "mail";
        case LOG_NEWS:
            return "news";
        case LOG_SYSLOG:
            return "syslog";
        case LOG_USER:
            return "user";
        case LOG_UUCP:
            return "uucp";
        }

        return std::to_string(facility);
    }

    std::string levelToName(int level) noexcept
    {
        switch (LOG_PRI(level))
        {
            case LOG_EMERG:
                return "emerg";
            case LOG_ALERT:
                return "alert";
            case LOG_CRIT:
                return "crit";
            case LOG_ERR:
                return "err";
            case LOG_WARNING:
                return "warning";
            case LOG_NOTICE:
                return "notice";
            case LOG_INFO:
                return "info";
            case LOG_DEBUG:
                return "debug";
        }

        ::dprintf(STDERR_FILENO,  "missing switch-case for: %d ",  LOG_PRI(level));
        ::abort();
    }
}

MessageFormatter::MessageFormatter(const std::string& prefixFormat):prefixFormat(prefixFormat)
{

}

MessageFormatter::~MessageFormatter()
{
}

std::string MessageFormatter::formatPrefix(int priority, const std::string& ident, pid_t pid, const struct timeval& t, const struct tm& tm)
{
    std::ostringstream preformattedPrefix;

    for(auto i = prefixFormat.begin(); i != prefixFormat.end(); i++)
    {
        if(*i == '$')
        {
            i++;
            if(i == prefixFormat.end())
            {
                throw " Invalid prefix format " ;
            }

            switch (*i)
            {
            case 'f': preformattedPrefix << (priority & LOG_FACMASK); break;
            case 'F': preformattedPrefix << facilityToName(priority); break;
            case 'l': preformattedPrefix << LOG_PRI(priority); break;
            case 'L': preformattedPrefix << levelToName(priority); break;
            case 'r': preformattedPrefix << priority; break;
            case 'h': preformattedPrefix << getLogHostname(); break;
            case 'H': preformattedPrefix << getLogFqd(); break;
            case 'i': preformattedPrefix << ident; break;
            case 'p': preformattedPrefix << pid; break;
            case 'z':
            {
                char tz[7] = {0};
                ::strftime(tz, sizeof(tz), "%z", &tm);
                preformattedPrefix << tz[0] << tz[1] << tz[2] << ':' << tz[3] << tz[4];
                break;
            }
            case '3': preformattedPrefix << std::setfill('0') << std::setw(3) << (t.tv_usec / 1000); break;
            case '6': preformattedPrefix << std::setfill('0') << std::setw(6) << t.tv_usec; break;
            case '$': preformattedPrefix << '$'; break;
            default: throw " Invalid prefix format " ;
            }
        }else
        {
            preformattedPrefix << *i;
        }
    }

    return preformattedPrefix.str();
}

std::string MessageFormatter::createMessage(const std::string& ident, pid_t pid, int facility, int priority, const char* message, size_t size)
{
    if((priority & LOG_FACMASK) == 0)
    {
        priority |= facility;
    }

    struct timeval t;
    ::gettimeofday(&t, nullptr);
    struct tm tm = {};
    ::localtime_r(&t.tv_sec, &tm);

    std::string preformattedPrefix = formatPrefix(priority, ident, pid, t, tm);

    char prefix[256] = {};
    
    const size_t prefixSize = strftime(prefix, sizeof(prefix), preformattedPrefix.c_str(), &tm);

    const bool addNewLine = endsWithNewLine(message, size);

    std::string ret;

    ret.reserve(prefixSize + size + (addNewLine ? 1 : 0));
    
    ret += std::string_view(prefix, prefixSize);
    ret += std::string_view(message, size);

    if(addNewLine)
    {
        ret += '\n';
    }

    return ret;
}

