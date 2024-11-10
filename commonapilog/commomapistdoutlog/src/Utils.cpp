#include <sstream>
#include <cstdlib>
#include <climits>
#include <unistd.h>
#include <string.h>

#include "Utils.hpp"

namespace commonapistdoutlogger
{

std::sting getLogHostname()
{
    std::ostringstream os;

    if(auto hostname = ::getenv("COMMON_API_HOSTNAME"))
    {
        os << hostname;
    }else
    {
        char hostNameBuffer[HOST_NAME_MAX + 1] = {};
        ::gethostname(hostNameBuffer, HOST_NAME_MAX);
        os << hostNameBuffer;
    }

    return os.str();
}


std::string getLogFqd()
{
    std::ostringstream os;
    os << getLogHostname();

    char domainNameBuffer[HOST_NAME_MAX + 1] = {};

    ::getdomainname(domainNameBuffer, sizeof(domainNameBuffer) - 1);

    if(domainNameBuffer[0] && strcmp(domainNameBuffer, "(node)"))
    {
        os << '.' << domainNameBuffer;
    }

    return os.str();
}

}
