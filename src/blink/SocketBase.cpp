#include <blink/SocketBase.h>
#include <blink/Endian.h>
#include <blink/Types.h>
#include <blink/Log.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <endian.h>
#include <unistd.h>
#include <fcntl.h>

#include <errno.h>
#include <string.h>
#include <stdlib.h>

namespace blink
{

namespace sockets
{

struct sockaddr* sockaddr_cast(struct sockaddr_in* addr)
{
    return static_cast<struct sockaddr*>(implicit_cast<void*>(addr));
}

const struct sockaddr* sockaddr_cast(const struct sockaddr_in* addr)
{
    return static_cast<const struct sockaddr*>(implicit_cast<const void*>(addr));
}

struct sockaddr_in* sockaddr_in_cast(struct sockaddr* addr)
{
    return static_cast<struct sockaddr_in*>(implicit_cast<void*>(addr));
}

const struct sockaddr_in* sockaddr_in_cast(const struct sockaddr* addr)
{
    return static_cast<const struct sockaddr_in*>(implicit_cast<const void*>(addr));
}

#pragma GCC diagnostic ignored "-Wold-style-cast"

uint32_t hton_long(uint32_t hostlong)
{
    return htonl(hostlong);
}

uint16_t hton_short(uint16_t hostshort)
{
    return htons(hostshort);
}

uint32_t ntoh_long(uint32_t netlong)
{
    return ntohl(netlong);
}

uint16_t ntoh_short(uint16_t netshort)
{
    return ntohs(netshort);
}

#pragma GCC diagnostic error "-Wold-style-cast"

void toIpPort(char* buf, size_t size, const struct sockaddr_in& addr)
{
    assert(size >= INET_ADDRSTRLEN);
    ::inet_ntop(AF_INET, &addr.sin_addr, buf, static_cast<socklen_t>(size));
    size_t end = strlen(buf);
    uint16_t port = networkToHost16(addr.sin_port);
    assert(size > end);
    snprintf(buf + end, size - end, ":%u", port);
}

void toIp(char* buf, size_t size, const struct sockaddr_in& addr)
{
    assert(size >= INET_ADDRSTRLEN);
    ::inet_ntop(AF_INET, &addr.sin_addr, buf, static_cast<socklen_t>(size));
}

void fromIpPort(const char* ip, uint16_t port, struct sockaddr_in* addr)
{
    addr->sin_family = AF_INET;
    addr->sin_port = hostToNetwork16(port);
    if (::inet_pton(AF_INET, ip, &addr->sin_addr) <= 0)
    {
        LOG_ERROR << "sockets::fromIpPort";
    }
}

struct hostent* gethostbyname(const char* name)
{
    struct hostent* val = ::gethostbyname(name);
    if (val == NULL)
    {
        LOG_ERROR << "gethostbyname error: " << ::hstrerror(h_errno);
    }
    return val;
}

struct hostent* gethostbyaddr(const char* addr, int len, int type)
{
    struct hostent* val = ::gethostbyaddr(addr, len, type);
    if (val == NULL)
    {
        LOG_ERROR << "gethostbyaddr error: " << ::hstrerror(h_errno);
    }
    return val;
}

int gethostbyname_r(const char* name, struct hostent* ret, char* buf,
                    size_t buflen, struct hostent** result, int* h_errnop)
{
    return ::gethostbyname_r(name, ret, buf, buflen, result, h_errnop);
}

int gethostbyaddr_r(const void* addr, socklen_t len, int type, struct hostent* ret,
                    char* buf, size_t buflen, struct hostent** result, int* h_errnop)
{
    return ::gethostbyaddr_r(addr, len, type, ret, buf, buflen, result, h_errnop);
}

int getaddrinfo(const char* hostname, const char* service,
                const struct addrinfo* hints, struct addrinfo** result)
{
    int val = ::getaddrinfo(hostname, service, hints, result);
    if (val != 0)
    {
        LOG_ERROR << "getaddrinfo error: " << ::gai_strerror(val);
    }
    return val;
}

int getnameinfo(const struct sockaddr* addr, socklen_t addrlen, char* host,
                socklen_t hostlen, char* serv, socklen_t servlen, int flags)
{
    int val = ::getnameinfo(addr, addrlen, host, hostlen, serv, servlen, flags);
    if (val != 0)
    {
        LOG_ERROR << "getnameinfo error: " << ::gai_strerror(val);
    }
    return val;
}

int socket(int domain, int type, int protocol)
{
    return ::socket(domain, type, protocol);
}

int connect(int sockfd, const struct sockaddr_in& server_addr)
{
    // connect will return -1 in non-blocking model,
    // the checking errno work leave for user.
    return ::connect(sockfd, sockets::sockaddr_cast(&server_addr), static_cast<socklen_t>(sizeof(server_addr)));
}

void bindOrDie(int sockfd, const struct sockaddr_in& addr)
{
    int val = ::bind(sockfd, sockets::sockaddr_cast(&addr), static_cast<socklen_t>(sizeof(addr)));
    if (val < 0)
    {
        LOG_FATAL << "sockets::bindOrDie";
    }
}

void listenOrDie(int sockfd)
{
    int val = ::listen(sockfd, SOMAXCONN);
    if (val < 0)
    {
        LOG_FATAL << "sockets::listenOrDie";
    }
}

int accept(int sockfd, struct sockaddr_in* addr)
{
    socklen_t len = static_cast<socklen_t>(sizeof(*addr));
#if VALGRIND || defined (NO_ACCEPT4)
    int connectfd = ::accept(sockfd, sockets::sockaddr_cast(addr), &len);
    bool ret = sockets::setNonBlockAndCloseOnExec(connectfd);
    if (!ret)
    {
        LOG_FATAL << "Socket::accept, setNonBlockAndCloseOnExec";
    }
#else
    int connectfd = ::accept4(sockfd, sockets::sockaddr_cast(addr), &len, SOCK_NONBLOCK | SOCK_CLOEXEC);
#endif
    if (connectfd < 0)
    {
        int saved_errno = errno;
        LOG_ERROR << "Socket::accept, connectfd";
        switch(saved_errno)
        {
            case EAGAIN:               // Try again
            case ECONNABORTED:         // Software caused connection abort
            case EINTR:                // Interrupted system call
            case EPROTO:               // Protocol error
            case EPERM:                // Operation not permitted
            case EMFILE:               // Too many open files
                errno = saved_errno;
                break;
            case EBADF:                // File descriptor in bad state
            case EFAULT:               // Bad address
            case EINVAL:               // Invalid argument
            case ENFILE:               // File table overflow
            case ENOBUFS:              // No buffer space available
            case ENOMEM:               // Out of memory
            case ENOTSOCK:             // Socket operation on non-socket
            case EOPNOTSUPP:           // Operation not supported on transport endpoint
                LOG_FATAL << "unexpected error of accept/accept4";
                break;
            default:
                LOG_FATAL << "unknown error of accept/accept4";
                break;
        }
    }
    return connectfd;
}

ssize_t read(int fd, void* buf, size_t n)
{
    return ::read(fd, buf, n);
}

ssize_t write(int fd, const void* buf, size_t n)
{
    return ::write(fd, buf, n);
}

int close(int fd)
{
    int val = ::close(fd);
    if (val < 0)
    {
        LOG_ERROR << "sockets::close";
    }
    return val;
}

int shutdown(int sockfd, int howto)
{
    return ::shutdown(sockfd, howto);
}

ssize_t readv(int fd, const struct iovec* iov, int iovcnt)
{
    return ::readv(fd, iov, iovcnt);
}

ssize_t writev(int fd, const struct iovec* iov, int iovcnt)
{
    return ::writev(fd, iov, iovcnt);
}

int createNonblockingOrDie()
{
#if VALGRIND
    int sockfd = sockets::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd < 0)
    {
        LOG_FATAL << "createNonblocking()";
    }
    sockets::setNonBlockAndCloseOnExec(sockfd);
#else
    int sockfd = sockets::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if (sockfd < 0)
    {
        LOG_FATAL << "createNonblocking()";
    }
#endif
    return sockfd;
}

#if VALGRIND || defined (NO_ACCEPT4)

bool setNonBlockAndCloseOnExec(int sockfd)
{
    int flags = ::fcntl(sockfd, F_GETFL, 0);
    if (flags < 0)
    {
        LOG_ERROR << "setNonBlockAndCloseOnExec()";
        return false;
    }
    flags |= O_NONBLOCK;
    if (::fcntl(sockfd, F_SETFL, flags) < 0)
    {
        LOG_ERROR << "setNonBlockAndCloseOnExec()";
        return false;
    }

    flags = ::fcntl(sockfd, F_GETFL, 0);
    if (flags < 0)
    {
        LOG_ERROR << "setNonBlockAndCloseOnExec()";
        return false;
    }
    flags |= FD_CLOEXEC;
    if (::fcntl(sockfd, F_SETFL, flags) < 0)
    {
        LOG_ERROR << "setNonBlockAndCloseOnExec()";
        return false;
    }
    return true;
}

#endif

int getSocketError(int sockfd)
{
    int optval;
    socklen_t optlen = static_cast<socklen_t>(sizeof(optval));
    if (::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
    {
        return errno;
    }
    else
    {
        return optval;
    }
}

bool isSelfConnect(int sockfd)
{
    struct sockaddr_in local_addr = getLocalAddr(sockfd);
    struct sockaddr_in peer_addr = getPeerAddr(sockfd);
    return local_addr.sin_port == peer_addr.sin_port
           && local_addr.sin_addr.s_addr == peer_addr.sin_addr.s_addr;
}

struct sockaddr_in getLocalAddr(int sockfd)
{
    struct sockaddr_in local_addr;
    memset(&local_addr, 0, sizeof(local_addr));
    socklen_t addrlen = static_cast<socklen_t>(sizeof(local_addr));
    if (::getsockname(sockfd, sockets::sockaddr_cast(&local_addr), &addrlen) < 0)
    {
        LOG_ERROR << "blink::getLocalAddr";
    }
    return local_addr;
}

struct sockaddr_in getPeerAddr(int sockfd)
{
    struct sockaddr_in peer_addr;
    memset(&peer_addr, 0, sizeof(peer_addr));
    socklen_t addrlen = static_cast<socklen_t>(sizeof(peer_addr));
    if (::getpeername(sockfd, sockets::sockaddr_cast(&peer_addr), &addrlen) < 0)
    {
        LOG_ERROR << "blink::getPeerAddr";
    }
    return peer_addr;
}

int openClientFd(char* hostname, int port)
{
    int clientfd = sockets::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientfd < 0)
    {
        return -1;
    }
    struct hostent* hostp;
    struct in_addr ipaddr;
    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = sockets::hton_short(static_cast<unsigned short>(port));
    if (::inet_pton(AF_INET, hostname, &ipaddr) != 1)
    {
        hostp = sockets::gethostbyname(hostname);
        if (hostp == NULL)
        {
            return -2;
        }
        memcpy(&serveraddr.sin_addr.s_addr, &hostp->h_addr_list[0], hostp->h_length);
    }
    else
    {
        memcpy(&serveraddr.sin_addr.s_addr, &ipaddr.s_addr, sizeof(ipaddr.s_addr));
    }
    if (sockets::connect(clientfd, serveraddr) < 0)
    {
        return -1;
    }
    return clientfd;
}

int openListenFd(int port)
{
    int listenfd = sockets::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenfd < 0)
    {
        return -1;
    }
    const int optval = 1;
    if (::setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, implicit_cast<const void*>(&optval), sizeof(int)) < 0)
    {
        LOG_ERROR << "openListenFd - setsockopt error";
        return -1;
    }
    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = sockets::hton_short(static_cast<unsigned short>(port));
#pragma GCC diagnostic ignored "-Wold-style-cast"
    serveraddr.sin_addr.s_addr = sockets::hton_long(INADDR_ANY);
#pragma GCC diagnostic error "-Wold-style-cast"
    sockets::bindOrDie(listenfd, serveraddr);
    sockets::listenOrDie(listenfd);
    return listenfd;
}

}  // namespace sockets

}  // namespace blink
