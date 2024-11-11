#ifndef COMMON_API_ABORT_HPP_
#define COMMON_API_ABORT_HPP_
namespace commonapistdoutlogger
{
    [[noreturn]] void logAndAbort(const char* file, int line, const char* format, ...) noexcept
        __attribute__((format(printf, 3, 4)));
}

#define COMMON_API_STDOUT_LOGGER_ABORT(_format, ...) ::commonapistdoutlogger::logAndAbort(__FILE__, __LINE__, (_format), ##__VA_ARGS__)

#endif
