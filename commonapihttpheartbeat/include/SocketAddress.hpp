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

    [[nodiscard]] struct sockaddr* getAddress() noexcept {return &sa.s;}
    [[nodiscard]] const struct sockaddr* getAddress() const noexcept {return &sa.s;}

    [[nodiscard]] socklen_t getSize() const noexcept {return size;}

    [[nodiscard]] socklen_t* getSizePointer() noexcept;

    bool operator==(const SocketAddress& other) const noexcept;
    bool operator!=(const SocketAddress& other) const noexcept;

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
