#include "FifoLogger.hpp"
#include "SignalPipeBlock.hpp"

using namespace commonapistdoutlogger;

namespace
{
    bool isFatalError(ssize_t ret)
    {
        return ((ret == -1) && (errno != EAGAIN) && (errno != EMSGSIZE) && (errno != ENOBUFS) && (errno != ENOMEM));
    }
}

FifoLogger::FifoLogger(FileDescriptor&& fd):fd(std::move(fd))
{

}

void FifoLogger::write(const std::string& message)
{
    if (fd < 0)
    {
        return;
    }

    const SignalPipeBlocker sigpipeBlocker;

    if(isFatalError(::write(fd, message.c_str(), message.size())))
    {
        fd.close();
    }
}

void FifoLogger::writeAsync(const std::string& message)
{
    if(fd < 0)
    {
        return;
    }

   const SignalPipeBlocker sigpipeBlocker;

    if(isFatalError(::write(fd, message.c_str(), message.size())))
    {
        fd.close();
    }
}

void FifoLogger::waitAllWriteAsyncsCompleted()
{

}