#ifndef COMMON_API_NULL_LOGGER_HPP_
#define COMMON_API_NULL_LOGGER_HPP_

#include "LogWriter.hpp"

namespace commonapistdoutlogger
{
    class NullLogger : public LogWriter
    {
    public:
        NullLogger() {}
        ~NullLogger(){}
        
        void write(const std::string& ) override {}
        void writeAsync(const std::string& ) override {}
        void waitAllWriteAsyncsCompleted() override {}
    };
}

#endif
