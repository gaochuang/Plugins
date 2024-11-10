#include "FileLogger.hpp"
#include "SignalPipeBlock.hpp"

#include <unistd.h>

using namespace commonapistdoutlogger;

namespace
{
    bool isFatalError(ssize_t ret)
    {
        return ((ret == -1) && (errno != EAGAIN) && (errno != ENOSPC) &&
                (errno != EIO) && (errno != EMSGSIZE) &&
                (errno != ENOMEM) && (errno != ENOBUFS));
    }
}

FileLogger::FileLogger(FileDescriptor&& fd):fd(std::move(fd))
{

}


void FileLogger::write(const std::string& message)
{
    if(fd < 0)
    {
        return;
    }

    const SignalPipeBlocker sigpipeBlocker;

    if(isFatalError(TEMP_FAILURE_RETRY(::write(fd, message.c_str(), message.size()))))
    {
        fd.close();
    }
}

void FileLogger::writeAsync(const std::string& message)
{
    if(fd < 0)
    {
        return;
    }

    const SignalPipeBlocker sigpipeBlocker;

    if(isFatalError(TEMP_FAILURE_RETRY(::write(fd, message.c_str(), message.size()))))
    {
        fd.close();
    }
}

void FileLogger::waitAllWriteAsyncsCompleted()
{
    if(fd >=0 )
    {
        ::fsync(fd);
    }
}
