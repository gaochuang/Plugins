#include "SocketAddress.hpp"

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <netdb.h>
#include <arpa/inet.h>
#include <ostream>
#include <sstream>

using namespace commapihttpheartbeat;

namespace 
{

size_t addrMemCopy(const void* src, size_t srcSize, void* dst,  size_t dstSize) noexcept
{
    size_t min = std::min(srcSize, dstSize);
    memcpy(dst, src, min);
    if(dstSize > srcSize)
    {
        memset(static_cast<char*>(dst)+ srcSize, 0, dstSize - srcSize);
    }
    return min;
}

socklen_t getAddressInfo(int family, const std::string& node, const std::string& service, struct sockaddr* sa)
{
    const struct addrinfo info = {0, family};

    struct addrinfo* result = nullptr;

    int ret = getaddrinfo(node.c_str(), service.c_str(), &info, &result);
    if(ret)
    {
        std::ostringstream oss;
        oss << "getaddrinfo failed,  node: "<< node.c_str() << "service: " << service.c_str() << "error reason: " << gai_strerror(ret) << ": " << std::endl;
        throw oss.str();
    }

    socklen_t size = result->ai_addrlen;
    memcpy(sa, result->ai_addr, size);
    freeaddrinfo(result);
    return size;
}

}

SocketAddress::SocketAddress() noexcept : size(0)
{
    memset(&sa, 0 ,sizeof(sa));
}

SocketAddress::SocketAddress(const struct sockaddr* s, socklen_t size) noexcept : size(static_cast<socklen_t>(addrMemCopy(s, size, &sa, sizeof(sa))))
{

}

SocketAddress SocketAddress::createIPv4(const std::string& address, const std::string& service)
{
    SocketAddress sa;
    sa.size = getAddressInfo(AF_INET, address, service, &sa.sa.s);
    return sa;
}

SocketAddress SocketAddress::createIPv6(const std::string& address, const std::string& service)
{
    SocketAddress sa;

    sa.size = getAddressInfo(AF_INET6, address, service, &sa.sa.s);
    return sa;
}

//ipv4:192.168.1.1:8080
SocketAddress SocketAddress::create(const std::string& str)
{
    const size_t first = str.find(':');
    
    std::ostringstream oss;
    if(first == std::string::npos || 1 == first)
    {
        oss << "invalid address: " << str << std::endl; 
        throw oss.str();
    }

    const size_t last = str.rfind(':');
    if((first == last) || (last == first + 1) || (last == str.size() - 1))
    {
        oss << "invalid address: " << str << std::endl; 
        throw oss.str();
    }
    
    //ipv4:192.168.1.1:8080
    const std::string prefix = str.substr(0, first);

    const std::string addr = str.substr(first + 1, last - first - 1);

    const std::string service = str.substr(last + 1);

    if("ipv4" == prefix)
    {
        return createIPv4(addr, service);
    }

    if("ipv6" == prefix)
    {
        return createIPv6(addr, service);
    }

    //可以自定义地址
    if("ip" == prefix)
    {
        SocketAddress sa;
        sa.size = getAddressInfo(AF_UNSPEC, addr, service, &sa.sa.s);
        return sa;
    }

    oss << "invalid address: " << str << std::endl; 
    throw oss.str();
}

socklen_t* SocketAddress::getSizePointer() noexcept
{
    size = sizeof(sa);
    return &size;
}

bool SocketAddress::operator==(const SocketAddress& other) const noexcept
{
    return ((size == other.size) && (memcmp(&sa, &other.sa, size) == 0));
}

bool SocketAddress::operator!=(const SocketAddress& other) const noexcept
{
    return ((size != other.size) && (memcmp(&sa, &other.sa, size) != 0));
}
