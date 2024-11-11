#ifndef COMMON_API_FIFO_LOGGER_HPP_
#define COMMON_API_FIFO_LOGGER_HPP_

#include "LogWriter.hpp"
#include "FileDescriptor.hpp"

namespace commonapistdoutlogger
{
    class FifoLogger : public LogWriter
    {
    public:
        FifoLogger(FileDescriptor&& fd);
        ~FifoLogger() = default;

        void write(const std::string& message) override;
        void writeAsync(const std::string& message) override;
        void waitAllWriteAsyncsCompleted() override;
    private:
        FileDescriptor fd;
    };

}

#endif
