#ifndef COMMON_API_SIGNAL_SET_OPERATION_HPP_
#define COMMON_API_SIGNAL_SET_OPERATION_HPP_

#include <csignal>
#include <cstring>
#include <cerrno>

#include "Abort.hpp"

namespace commonapistdoutlogger
{
    inline void signalEmptySet(sigset_t* t)
    {
        if(nullptr == t)
        {
            COMMON_API_STDOUT_LOGGER_ABORT("signalEmptySet: pointer is nullptr");
        }
        if( -1 == ::sigemptyset(t) )
        {
            COMMON_API_STDOUT_LOGGER_ABORT("sigemptyset: %s", strerror(errno));
        }
    }

    inline void signalFillSet(sigset_t* t)
    {
        if(nullptr == t)
        {
            COMMON_API_STDOUT_LOGGER_ABORT("signalFillSet: pointer is nullptr");
        }

        if(-1 == ::sigfillset(t))
        {
            COMMON_API_STDOUT_LOGGER_ABORT("signalFillSet: %s", strerror(errno));
        }
    }

    inline void signalAddSet(sigset_t* t, int signo)
    {
        if(nullptr == t)
        {
            COMMON_API_STDOUT_LOGGER_ABORT("signalAddSet: pointer is nullptr");
        }

        if(-1 == ::sigaddset(t, signo))
        {
            COMMON_API_STDOUT_LOGGER_ABORT("sigaddset: %s", strerror(errno));
        }
    }

    inline void signalDelSet(sigset_t* t, int signo)
    {
        if(nullptr == t)
        {
            COMMON_API_STDOUT_LOGGER_ABORT("signalDelSet: pointer is nullptr");
        }
        if(-1 == ::sigdelset(t, signo))
        {
            COMMON_API_STDOUT_LOGGER_ABORT("sigdelset: %s", strerror(errno));
        }
    }

    [[nodiscard]] inline bool signalIsMember(sigset_t* t, int no)
    {
        if(nullptr == t)
        {
            COMMON_API_STDOUT_LOGGER_ABORT("signalIsMember: pointer is nullptr");
        }

        return 1 == ::sigismember(t, no);

    }

    inline void signalPending(sigset_t* t)
    {
        if(nullptr == t)
        {
            COMMON_API_STDOUT_LOGGER_ABORT("signalIsMember: pointer is nullptr");
        }

        if(-1 == ::sigpending(t))
        {
            COMMON_API_STDOUT_LOGGER_ABORT("sigpending: %s", strerror(errno));
        }
    }
}

#endif
