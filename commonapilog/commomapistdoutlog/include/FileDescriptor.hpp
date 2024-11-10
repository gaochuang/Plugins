#ifndef COMMON_API_FILE_DESCRIPTOR_HPP_
#define COMMON_API_FILE_DESCRIPTOR_HPP_

namespace commonapistdoutlogger
{

class FileDescriptor
{
public:
    FileDescriptor(int fd, bool ownership) noexcept;

    FileDescriptor(FileDescriptor&& fd) noexcept;

    FileDescriptor& operator=(FileDescriptor&& fd) noexcept;

    ~FileDescriptor();

    operator int() const noexcept {return fd;}

    void close() noexcept;

    FileDescriptor(const FileDescriptor&) = delete;
    FileDescriptor& operator=(const FileDescriptor&) = delete;

private:
    int fd;
    bool ownership;
};

}

#endif
