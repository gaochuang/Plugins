#ifndef SOCKET_ADDRESS_HPP
#define SOCKET_ADDRESS_HPP

#include <sys/socket.h>
#include <netinet/in.h>
#include <string>

namespace commapihttpheartbeat
{

class SocketAddress
{
public:
    SocketAddress() noexcept;
    SocketAddress(const struct sockaddr* s, socklen_t size) noexcept;

    static SocketAddress createIPv4(const std::string& address, const std::string& service);
    static SocketAddress createIPv6(const std::string& address, const std::string& service);
    static SocketAddress create(const std::string& str);

    struct sockaddr* getAddress() noexcept {return &sa.s;}
    const struct sockaddr* getAddress() const noexcept {return &sa.s;}

    socklen_t getSize() const noexcept {return size;}

private:
    union SA
    {
        struct sockaddr s;
        struct sockaddr_in sin;
        struct sockaddr_in6 sin6;
    };

    SA sa;
    socklen_t size;
};

}

#endif
