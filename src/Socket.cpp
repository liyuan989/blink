#include "Socket.h"
#include "SocketBase.h"
#include "InetAddress.h"
#include "Log.h"

#include <netinet/tcp.h>

#include <string.h>
#include <assert.h>
#include <errno.h>
#include <stdio.h>

namespace blink
{

Socket::~Socket()
{
    sockets::close(sockfd_);
}

bool Socket::getTcpInfo(struct tcp_info* tcpi) const
{
#ifdef TCP_INFO
    assert(tcpi != NULL);
    socklen_t len = sizeof(*tcpi);
    memset(&len, 0, sizeof(tcpi));
    int val = ::getsockopt(sockfd_, SOL_TCP, TCP_INFO, tcpi, &len);
    if (val < 0)
    {
        LOG_ERROR << "TCP_INFO failed";
        return false;
    }
    else
    {
        return true;
    }
#else
    LOG_ERROR << "TCP_INFO is not supported";
    return false;
#endif
}

//  struct tcp_info
//  {
//      __u8    tcpi_state;
//      __u8    tcpi_ca_state;
//      __u8    tcpi_retransmits;
//      __u8    tcpi_probes;
//      __u8    tcpi_backoff;
//      __u8    tcpi_options;
//      __u8    tcpi_snd_wscale : 4, tcpi_rcv_wscale : 4;
//
//      __u32   tcpi_rto;
//      __u32   tcpi_ato;
//      __u32   tcpi_snd_mss;
//      __u32   tcpi_rcv_mss;
//
//      __u32   tcpi_unacked;
//      __u32   tcpi_sacked;
//      __u32   tcpi_lost;
//      __u32   tcpi_retrans;
//      __u32   tcpi_fackets;
//
//      /* Times. */
//      __u32   tcpi_last_data_sent;
//      __u32   tcpi_last_ack_sent;
//      __u32   tcpi_last_data_recv;
//      __u32   tcpi_last_ack_recv;
//
//      /* Metrics. */
//      __u32   tcpi_pmtu;
//      __u32   tcpi_rcv_ssthresh;
//      __u32   tcpi_rtt;
//      __u32   tcpi_rttvar;
//      __u32   tcpi_snd_ssthresh;
//      __u32   tcpi_snd_cwnd;
//      __u32   tcpi_advmss;
//      __u32   tcpi_reordering;
//  };

bool Socket::getTcpInfoString(char* buf, size_t len) const
{
    struct tcp_info tcpi;
    bool ret = getTcpInfo(&tcpi);
    if (ret)
    {
        snprintf(buf, len, "unrecovered=%u "
                 "rto=%u ato=%u snd_mss=%u rcv_mss=%u "
                 "lost=%u retrans=%u rtt=%u rttvar=%u "
                 "ssthresh=%u cwnd=%u total_retrans=%u",
                 tcpi.tcpi_retransmits,
                 tcpi.tcpi_rto,
                 tcpi.tcpi_ato,
                 tcpi.tcpi_snd_mss,
                 tcpi.tcpi_rcv_mss,
                 tcpi.tcpi_lost,
                 tcpi.tcpi_retrans,
                 tcpi.tcpi_rtt,
                 tcpi.tcpi_rttvar,
                 tcpi.tcpi_snd_ssthresh,
                 tcpi.tcpi_snd_cwnd,
                 tcpi.tcpi_total_retrans);
    }
    return ret;
}

void Socket::bindAddress(const InetAddress& local_addr)
{
    struct sockaddr_in addr = local_addr.getSockAddrInet();
    int val = sockets::bind(sockfd_, sockets::sockaddr_cast(&addr), sizeof(addr));
    if (val < 0)
    {
        LOG_FATAL << "Socket::bindAddress";
    }
}

void Socket::listen()
{
    int val = sockets::listen(sockfd_, SOMAXCONN);
    if (val < 0)
    {
        LOG_FATAL << "Socket::listen";
    }
}

int Socket::accept(InetAddress* addr)
{
    struct sockaddr_in addr_in;
    memset(&addr_in, 0, sizeof(addr_in));
    int len = sizeof(addr_in);
#if VALGRIND || defined (NO_ACCEPT4)
    int connectfd = sockets::accept(sockfd_, sockets::sockaddr_cast(&addr_in), &len);
    bool ret = sockets::setNonBlockAndCloseOnExec(sockfd_);
    if (!ret)
    {
        LOG_FATAL << "Socket::accept, setNonBlockAndCloseOnExec";
    }
#else
    int connectfd = sockets::accept4(sockfd_, sockets::sockaddr_cast(&addr_in),
                                     &len, SOCK_NONBLOCK | SOCK_CLOEXEC);
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
    else
    {
        addr->setSockAddrInet(addr_in);
    }
    return connectfd;
}

void Socket::shutdownWrite()
{
    if (sockets::shutdown(sockfd_, SHUT_WR) < 0)
    {
        LOG_ERROR << "Socket::shutdownWrite";
    }
}

void Socket::setTcpNoDelay(bool on)
{
#ifdef TCP_NODELAY
    int optval = on ? 1 : 0;
    int val = ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval,
                           static_cast<socklen_t>(sizeof(optval)));
    if (val < 0)
    {
        LOG_ERROR << "TCP_NODELAY failed";
    }
#else
    if (on)
    {
        LOG_ERROR << "TCP_NODELAY is not supported";
    }
#endif
}

void Socket::setReuseAddr(bool on)
{
#ifdef SO_REUSEADDR
    int optval = on ? 1 : 0;
    int val = ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval,
                           static_cast<socklen_t>(sizeof(optval)));
    if (val < 0)
    {
        LOG_ERROR << "SO_REUSEADDR failed";
    }
#else
    if (on)
    {
        LOG_ERROR << "SO_REUSEADDR is not supported";
    }
#endif
}

void Socket::setReusePort(bool on)
{
#ifdef SO_REUSEPORT
    int optval = on ? 1 : 0;
    int val = ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval,
                           static_cast<socklen_t>(sizeof(optval)));
    if (val < 0)
    {
        LOG_ERROR << "SO_REUSEPORT failed";
    }
#else
    if (on)
    {
        LOG_ERROR << "SO_REUSEPORT is not supported";
    }
#endif
}

void Socket::setKeepAlive(bool on)
{
#ifdef SO_KEEPALIVE
    int optval = on ? 1 : 0;
    int val = ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval,
                           static_cast<socklen_t>(sizeof(optval)));
    if (val < 0)
    {
        LOG_ERROR << "SO_KEEPALIVE failed";
    }
#else
    if (on)
    {
        LOG_ERROR << "SO_KEEPALIVE is not supported";
    }
#endif
}

}  // namespace blink
