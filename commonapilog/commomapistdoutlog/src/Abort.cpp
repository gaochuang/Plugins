#include "Abort.hpp"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>

using namespace commonapistdoutlogger;

void commonapistdoutlogger::logAndAbort(const char* file, int line, const char* format, ...) noexcept
{
    char* newFormat(nullptr);
    const int ret = asprintf(&newFormat,  "COMMON_API_LOGGER_ABORT, file %s, line %d: %s\n", file, line, format);
    if (ret == -1)
    {
        dprintf(STDERR_FILENO, "COMMON_API_LOGGER_ABORT, file %s, line %d:\n", file, line);
        va_list ap;
        va_start(ap, format);
        vdprintf(STDERR_FILENO, format, ap);
        va_end(ap);
        const auto ssize(write(STDERR_FILENO, "\n", 1));
        static_cast<void>(ssize);
    }else
    {
        va_list ap;
        va_start(ap, format);
        vdprintf(STDERR_FILENO, newFormat, ap);
        va_end(ap);
        free(newFormat);     
    }

    abort();
}

