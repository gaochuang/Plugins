
#include <logger/Logger.hpp>
#include <logger/LoggerPlugin.hpp>

#include "FileDescriptor.hpp"
#include "RedirectOutPid.hpp"
#include "Configuration.hpp"
#include "MessageRouter.hpp"
#include "FileLogger.hpp"
#include "FifoLogger.hpp"
#include "NullLogger.hpp"
#include "MessageFormat.hpp"

#include <iostream>
#include <memory>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <cstring>
#include <sys/stat.h>

using namespace commonApi;
using namespace commonApi::logger;
using namespace commonapistdoutlogger;

namespace
{
    /* https://tools.ietf.org/html/rfc5424 */
    //<34>1 2024-11-06T14:48:27.003Z mymachine.example.com app-name 12345 ID47 [exampleSDID@32473 iut="3" eventSource="Application"] User login successful
    constexpr const char* RFC5424_PREFIX("<$r>1 %Y-%m-%dT%H:%M:%S.$6$z $H $i $p - - ");

    struct LoggerInfo
    {
        std::shared_ptr<PluginServices>& service;
        std::string ident;
        int facility;
        pid_t pid;
    };

    bool isTheSameFile(int fdA, int fdB)
    {
        struct stat statA, statB;
        if(::fstat(fdA, &statA) != 0 || ::fstat(fdB, &statB) != 0)
        {
            return false;
        }

        // 仅在设备号和 inode 号都相同时返回 true
        return (statA.st_dev == statB.st_dev ) && (statA.st_ino == statB.st_ino);
    }

    FileDescriptor getStdoutFd()
    {
        auto fd = openOtherProcessStdOut();
        if(fd >= 0)
        {
            return fd;
        }

        return {STDOUT_FILENO, false};
    }

    FileDescriptor getStdErrFd()
    {
        auto fd = openOtherProcessStdErr();
        if(fd >= 0)
        {
            return fd;
        }

        return {STDERR_FILENO, false};
    }

    const char* getMessageFormatPrefix()
    {
        const char* prefixFormat = ::getenv("COMMON_API_LOGGER_PREFIX");
        if(prefixFormat)
        {
            std::cout << "COMMON_API_STDOUT_PREFIX:  " << prefixFormat << " " << std::endl;
        }else
        {
            std::cout << "COMMON_API_STDOUT_PREFIX is not define, use RFC 5425 as default " << std::endl;
            prefixFormat = RFC5424_PREFIX;
        }

        return prefixFormat;
    }

    std::string getFdType(int fd)
    {
        struct stat sb;
        if (fstat(fd, &sb) != 0)
        {
            return strerror(errno);
        }

        switch (sb.st_mode & S_IFMT)
        {
            case S_IFBLK:
                return "block device";
            case S_IFCHR:
                return "character device";
            case S_IFDIR:
                return "directory";
            case S_IFLNK:
                return "symlink";
            case S_IFIFO:
                return "fifo";
            case S_IFSOCK:
                return "socket";
            case S_IFREG:
                return "regular file";
            default:
            {
                std::ostringstream os;
                os << "unknown type " << (sb.st_mode & S_IFMT);
                return os.str();
            }
        }
    }

    bool isFifoOrSocket(int fd)
    {
        struct  stat sb;
        return (fstat(fd, &sb) == 0 && ((S_ISREG(sb.st_mode) || (S_ISCHR(sb.st_mode)))));
    }

    bool isFileOrCharDevice(int fd)
    {
        struct stat sb;

        return (0 == ::fstat(fd, &sb)) && (S_ISREG(sb.st_mode)) || (S_ISCHR(sb.st_mode));
    }

    std::unique_ptr<MessageFormatter> getMessageFormatter()
    {
        return std::make_unique<MessageFormatter>(getMessageFormatPrefix());
    }

    std::unique_ptr<LogWriter> createLogWriter(FileDescriptor&& fd, const std::string& name)
    {
        if(isFifoOrSocket(fd))
        {
            std::cout << name << " (fd " << fd << " ) " << "is a pipe/socket, creating fifo logger" <<std::endl;
            return std::make_unique<FifoLogger>(std::move(fd));
        }

        if(isFileOrCharDevice(fd))
        {
            std::cout << name << " (fd " << fd << " ) " << " is a regular file or tty, creating fifo logger" <<std::endl;
            return std::make_unique<FileLogger>(std::move(fd));
        }

        std::cout << name << " ( fd " << fd << " ) of type "<< getFdType(fd) << "can not be written to, creating null logger " << std::endl;

        return std::make_unique<NullLogger>();
    }

    std::shared_ptr<Logger> getLoggerPlugin(const LoggerInfo& info)
    {
        FileDescriptor stdoutFd = getStdoutFd();
        FileDescriptor stderrFd = getStdErrFd();

        std::ostringstream errors;

        auto config = getConfiguration(errors);
        auto errs = errors.str();
        if(!errs.empty())
        {
            ::dprintf(stderrFd,"%s", errs.c_str());
        }

        if(isTheSameFile(stdoutFd, stderrFd))
        {
            std::cout << "STDOUT ( " << stdoutFd << ") and STDERR (" <<stderrFd << ") are the same: all will be write to STDOUT" << std::endl;

            return std::make_shared<MessageRouter>(
                getMessageFormatter(),
                info.ident,
                info.facility,
                info.pid,
                std::move(config),
                createLogWriter(std::move(stdoutFd), "stdout"));
        }

        std::cout << "STDOUT (" << stdoutFd << ") and STDERR ( " << stderrFd << " ) are not the same: messages with level <= " << config.minErrLevel
        << "will be written to STDERR " <<std::endl;

        return std::make_shared<MessageRouter>(getMessageFormatter(), 
        info.ident, info.facility, info.pid, std::move(config),
        createLogWriter(std::move(stdoutFd), "stdout"),
        createLogWriter(std::move(stderrFd), "stderr"));
    }
}

COMMONAPI_DEFINE_LOGGER_PLUGIN_CREATOR(services, params)
{
    auto val = ::getenv("COMMON_API_STDOUT_LOGGER_DISABLE_ENV_NAME");
    if(nullptr == val)
    {
        std::cout << " COMMON_API_STDOUT_LOGGER_DISABLE_ENV_NAME " << " = " << val << " not defined, PACKAGE_NAME " << " plugin is disabled " << std::endl;  
        return nullptr;
    }

    std::cout << " COMMON_API_STDOUT_LOGGER_DISABLE_ENV_NAME  defined, stdout logger enable " <<std::endl;

    return getLoggerPlugin({services, params.indent, params.facility, getpid()});
}
