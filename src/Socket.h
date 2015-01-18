#ifndef __BLINK_SOCKET_H__
#define __BLINK_SOCKET_H__

#include "Nocopyable.h"

#include <stddef.h>

struct tcp_info;
struct sockaddr_in;

namespace blink
{

class InetAddress;

class Socket : Nocopyable
{
public:
    explicit Socket(int sockfd)
        : sockfd_(sockfd)
    {
    }

    ~Socket();

    bool getTcpInfo(struct tcp_info* tcp) const;
    bool getTcpInfoString(char* buf, size_t len) const;
    void bindAddress(const InetAddress& local_addr);
    void listen();

    // set to non-blocking and close-on-exec
    int accept(InetAddress* addr);

    void shutdownWrite();

    // enable/disable TCP_NODELAY(Nagle's algorithm)
    void setTcpNoDelay(bool on);

    // enable/disable SO_REUSEADDR
    void setReuseAddr(bool on);

    // enable/disable SO_REUSEPORT
    void setReusePort(bool on);

    // enable/disable SO_KEEPALIVE
    void setKeepAlive(bool on);

    int fd() const
    {
        return sockfd_;
    }

private:
    const int sockfd_;
};

}  // namespace blink

#endif
