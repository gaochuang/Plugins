#ifndef COMMON_API_REDIRECT_OUT_PID_HPP
#define COMMON_API_REDIRECT_OUT_PID_HPP

#include "FileDescriptor.hpp"

namespace commonapistdoutlogger
{
    FileDescriptor openOtherProcessStdOut();
    FileDescriptor openOtherProcessStdErr();
}

#endif
