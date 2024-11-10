#ifndef COMMON_API_SIGNAL_PIPE_BLOCK_HPP
#define COMMON_API_SIGNAL_PIPE_BLOCK_HPP

#include <csignal>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

#include "Abort.hpp"
#include "SignalSetOperation.hpp"

namespace commonapistdoutlogger
{
    class SignalPipeBlocker
    {
    public:
        SignalPipeBlocker() noexcept : wasAlreadyPendingAndBlocked(false), wasAlreadyBlocked(false)
        {
            signalEmptySet(&sigpipeSet);
            signalAddSet(&sigpipeSet, SIGPIPE);
            if(!(wasAlreadyPendingAndBlocked = isSignalPending()) && !(wasAlreadyBlocked = isSignalPipeBlocked()))
            {
                blockSigpipe();
            }
        }

        ~SignalPipeBlocker()
        {
            if(wasAlreadyPendingAndBlocked)
            {
                return;
            }

            if(isSignalPending())
            {
                handlePendingSigpipe();
            }

            if(!wasAlreadyBlocked)
            {
                restoreOriginalSigmask();
            }
        }

        SignalPipeBlocker(const SignalPipeBlocker&) = delete;
        SignalPipeBlocker(SignalPipeBlocker&&) = delete;
        SignalPipeBlocker& operator=(const SignalPipeBlocker&) = delete;
        SignalPipeBlocker& operator=(SignalPipeBlocker&&) = delete;
    private:

        bool isSignalPending()
        {
            sigset_t pendingSignal;
            signalEmptySet(&pendingSignal);
            signalPending(&pendingSignal);

            return (signalIsMember(&pendingSignal, SIGPIPE));
        }

        bool isSignalPipeBlocked()
        {
            sigset_t blockedSignal;

            signalEmptySet(&blockedSignal);

            if (auto ret = pthread_sigmask(SIG_BLOCK, nullptr, &blockedSignal) != 0)
            {
                COMMON_API_STDOUT_LOGGER_ABORT("pthread_sigmask: %s", strerror(ret));
            }

            return (signalIsMember(&blockedSignal, SIGPIPE));
        }

        void saveCurentSigmask()
        {
            signalEmptySet(&oldBlockedSignal);
            if(auto ret = pthread_sigmask(SIG_BLOCK, nullptr, &oldBlockedSignal) != 0)
            {
                COMMON_API_STDOUT_LOGGER_ABORT("pthread_sigmask: %s", strerror(ret));
            }
        }

        void restoreOriginalSigmask()
        {
            if (auto ret = pthread_sigmask(SIG_UNBLOCK, &sigpipeSet, nullptr) != 0)
            {
                COMMON_API_STDOUT_LOGGER_ABORT("pthread_sigmask: %s", strerror(ret));
            }

            if (auto ret = pthread_sigmask(SIG_BLOCK, &oldBlockedSignal, nullptr) != 0)
            {
                COMMON_API_STDOUT_LOGGER_ABORT("pthread_sigmask: %s", strerror(ret));
            }
        }

        void blockSigpipe()
        {
            saveCurentSigmask();
            sigset_t newBlockedSignals;
            signalEmptySet(&newBlockedSignals);
            signalAddSet(&newBlockedSignals, SIGPIPE);
            if (auto ret = pthread_sigmask(SIG_BLOCK, &newBlockedSignals, nullptr) != 0)
            {
                COMMON_API_STDOUT_LOGGER_ABORT("pthread_sigmask: %s", strerror(ret));
            }
        }

        void handlePendingSigpipe()
        {
            const struct timespec zeroTimeout = {0, 0};
            TEMP_FAILURE_RETRY(sigtimedwait(&sigpipeSet, nullptr, &zeroTimeout));
        }

        bool wasAlreadyPendingAndBlocked;
        bool wasAlreadyBlocked;
        sigset_t oldBlockedSignal;
        sigset_t sigpipeSet;
    };
}

#endif
