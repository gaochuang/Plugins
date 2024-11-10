#ifndef COMMON_API_LOG_WRITER_HPP
#define COMMON_API_LOG_WRITER_HPP

#include <string>

namespace commonapistdoutlogger
{
    class LogWriter
    {
    public:
        virtual ~LogWriter() = default;
        virtual void write(const std::string& message) = 0;
        virtual void writeAsync(const std::string& message) = 0;
        virtual void waitAllWriteAsyncsCompleted() = 0;

        LogWriter(const LogWriter&) = delete;
        LogWriter(LogWriter&&) = delete;
        LogWriter& operator=(const LogWriter&) = delete;
        LogWriter& operator=(LogWriter&&) = delete ;
    protected:
       LogWriter() = default;
    };

}

#endif
