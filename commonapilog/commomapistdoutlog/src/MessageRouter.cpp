#include <algorithm>
#include <sstream>

#include "NullLogger.hpp"
#include "MessageRouter.hpp"
#include "MessageFormat.hpp"

using namespace commonapistdoutlogger;

namespace
{
    constexpr bool ONLY_STDOUT(true);

    bool isBadFacility(int priority) noexcept
    {
        return (LOG_FAC(priority) >= LOG_NFACILITIES);
    }

    bool isIncludeLevel(const Configuration& configuration, int level) noexcept
    {
        return (std::find(configuration.includeLevels.cbegin(), configuration.includeLevels.cend(), level) != configuration.includeLevels.end());
    }

    bool isIncludedFacility(const Configuration& configuration, int facility) noexcept
    {

        return ((facility == LOG_KERN) || (std::find(configuration.includeFacilities.cbegin(),
                configuration.includeFacilities.cend(),
                facility) != configuration.includeFacilities.cend()));
    }

    MessageRouter::MessageTargets createMessageTargetArray(const Configuration& configuration, bool onlySTDOUT = false) noexcept
    {
        MessageRouter::MessageTargets targets{};
        for(size_t i = 0; i < targets.size(); i++)
        {
            const int level = LOG_PRI(i);
            const int facility = (i & LOG_FACMASK);

            if(isIncludedFacility(configuration, facility) && isIncludeLevel(configuration, level))
            {
                if(!(onlySTDOUT) && (level <= configuration.minErrLevel))
                {
                    targets[i] = MessageRouter::MessageTarget::STDERR;
                }else
                {
                    targets[i] = MessageRouter::MessageTarget::STDOUT;
                }
            }
        }

        return targets;
    }

    bool checkFacility(int defaultFacility)
    {
        if(defaultFacility < 0 || (defaultFacility >= (LOG_NFACILITIES << 3)))
        {
            std::ostringstream os;
            os << "bad default facility " << defaultFacility;
            throw std::runtime_error(os.str());
        }

        if(!defaultFacility)
        {
            defaultFacility = LOG_USER;
        }

        return defaultFacility;
    }
}

MessageRouter::MessageRouter(std::unique_ptr<MessageFormatter> messageFormatter,
                    const std::string& ident,
                    int facility,
                    pid_t pid,
                    Configuration&& configuration,
                    std::unique_ptr<LogWriter> logger):
                    messageTargets(createMessageTargetArray(configuration, ONLY_STDOUT)),
                    messageFormatter(std::move(messageFormatter)),
                    ident(ident),
                    defaultFacility(checkFacility(facility)),
                    pid(pid),
                    configuration(std::move(configuration)),
                    stdoutLogger(std::move(logger)),
                    stderrLogger(std::make_unique<NullLogger>())
{
}              

MessageRouter::MessageRouter(std::unique_ptr<MessageFormatter> messageFormatter,
                   const std::string& ident,
                   int facility,
                   pid_t pid,
                   Configuration&& configuration,
                   std::unique_ptr<LogWriter> stdoutLogger,
                   std::unique_ptr<LogWriter> stderrLogger):
                   messageTargets(createMessageTargetArray(configuration, ONLY_STDOUT)),
                   messageFormatter(std::move(messageFormatter)),
                   ident(ident),
                   defaultFacility(checkFacility(facility)),
                   pid(pid),
                   configuration(std::move(configuration)),
                   stdoutLogger(std::move(stdoutLogger)),
                   stderrLogger(std::move(stderrLogger))

{

}

MessageRouter::MessageTarget MessageRouter::getMessageTarget(int priority) const noexcept
{
    if(isBadFacility(priority) || priority < 0 || static_cast<size_t>(priority) >= messageTargets.size())
    {
        return MessageTarget::DROPPED;
    }

    if(!(priority & LOG_FACMASK))
    {
        priority |= defaultFacility;
    }

    return messageTargets[priority];
}

bool MessageRouter::isStderrMessage(int messagePriority) const noexcept
{
    return ((messagePriority & LOG_PRIMASK) <= configuration.minErrLevel);
}

std::string MessageRouter::createMessage(int priority, const char* message, size_t size)
{
    return messageFormatter->createMessage(ident, pid, defaultFacility, priority, message, size);
}

void MessageRouter::write(int priority, const char* message, size_t size)
{
    switch (getMessageTarget(priority))
    {
    case MessageTarget::DROPPED:
        break;
    case MessageTarget::STDERR:
        stderrLogger->write(createMessage(priority, message, size));
        break;
    case MessageTarget::STDOUT:
        stdoutLogger->write(createMessage(priority, message, size));
        break;
    }
}

void MessageRouter::writeAsync(int priority, const char* message, size_t size)
{
    switch (getMessageTarget(priority))
    {
    case MessageTarget::DROPPED:
        break;
    case MessageTarget::STDERR:
        stderrLogger->writeAsync(createMessage(priority, message, size));
        break;
    case MessageTarget::STDOUT:
       stdoutLogger->writeAsync(createMessage(priority, message, size));
       break;
    }
}

void MessageRouter::waitAllWriteAndCompleted()
{
    stdoutLogger->waitAllWriteAsyncsCompleted();
    stderrLogger->waitAllWriteAsyncsCompleted();
}
