#include <unistd.h>

#include "FileDescriptor.hpp"

using namespace commonapistdoutlogger;

FileDescriptor::FileDescriptor(int fd, bool ownership) noexcept:
                fd(fd),
                ownership(ownership)
{
}

FileDescriptor::FileDescriptor(FileDescriptor&& fd) noexcept :
                    fd(fd.fd),
                    ownership(fd.ownership)
{
    fd.fd = -1;
    fd.ownership = false;
}

FileDescriptor::~FileDescriptor()
{
    close();
}

FileDescriptor&  FileDescriptor::operator=(FileDescriptor&& fd) noexcept
{
    close();
    this->fd = fd.fd;
    fd.fd = -1;
    fd.ownership = false;
    return *this;
}


void FileDescriptor::close() noexcept
{
    if(fd >= 0 && ownership)
    {
        ::close(fd);
    }
}
