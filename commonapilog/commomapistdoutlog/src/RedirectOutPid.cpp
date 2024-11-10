#include "RedirectOutPid.hpp"

#include <iostream>
#include <string>
#include <cerrno>
#include <cstring>
#include <sstream>
#include <unistd.h>
#include <fcntl.h>

using namespace commonapistdoutlogger;

namespace 
{

std::string buildFdPath(const char* pid, int targetFd)
{
    std::ostringstream os;

    os << "/proc/" << pid << "/fd/" << targetFd;
    return os.str();
}

FileDescriptor openOtherProcess(int targetFd)
{
    const auto env = ::getenv("COMMON_API_STDOUT_LOGGER_REDIRECT_PID_ENV_VAR_NAME");
    if(nullptr == env)
    {
        std::cout << "env: COMMON_API_STDOUT_LOGGER_REDIRECT_PID_ENV_VAR_NAME don't define " << std::endl;
        return {-1, false};
    }

    auto path = buildFdPath(env, targetFd);
    int fd = ::open(path.c_str(), O_WRONLY | O_CLOEXEC);
    if(fd < 0)
    {
        std::cerr << path << ": " << strerror(errno) << std::endl;
        return {-1, false};
    }

    std::cout << "Use " << path << " as " << targetFd << std::endl;
    return {fd, true};
}

}

FileDescriptor commonapistdoutlogger::openOtherProcessStdOut()
{
    return openOtherProcess(STDOUT_FILENO);
}

FileDescriptor commonapistdoutlogger::openOtherProcessStdErr()
{
    return openOtherProcess(STDERR_FILENO);
}


