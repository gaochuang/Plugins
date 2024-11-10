#ifndef COMMON_API_FILE_LOGGER_HPP_
#define COMMON_API_FILE_LOGGER_HPP_

#include "FileDescriptor.hpp"
#include "LogWriter.hpp"

namespace commonapistdoutlogger
{
    class FileLogger : public LogWriter
    {
    public:
        FileLogger(FileDescriptor&& fd);
        ~FileLogger() = default;
    
        void write(const std::string& message) override;
        void writeAsync(const std::string& message) override;
        void waitAllWriteAsyncsCompleted() override;
    private:
       FileDescriptor fd;
    };

};

#endif
