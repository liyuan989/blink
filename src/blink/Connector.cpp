#include <blink/Connector.h>
#include <blink/SocketBase.h>
#include <blink/EventLoop.h>
#include <blink/Channel.h>
#include <blink/InetAddress.h>
#include <blink/Acceptor.h>
#include <blink/Log.h>

#include <boost/bind.hpp>

#include <errno.h>
#include <assert.h>

namespace blink
{

const int Connector::kMaxRetryDelayMs;
const int Connector::kInitRetryDelayMs;

Connector::Connector(EventLoop* loop, const InetAddress& server_addr)
    : loop_(loop),
      server_addr_(server_addr),
      connect_(false),
      state_(kDisconnected),
      retry_delay_ms_(kInitRetryDelayMs)
{
    LOG_DEBUG << "constructor[" << this << "]";
}

Connector::~Connector()
{
    LOG_DEBUG << "destructor[" << this << "]";
    assert(!channel_);
}

void Connector::start()
{
    connect_ = true;
    loop_->runInLoop(boost::bind(&Connector::startInLoop, this));  // unsafe
}

void Connector::startInLoop()
{
    loop_->assertInLoopThread();
    assert(state_ == kDisconnected);
    if (connect_)
    {
        connect();
    }
    else
    {
        LOG_DEBUG << "do not connect";
    }
}

void Connector::connect()
{
    int sockfd = sockets::createNonblocking();
    struct sockaddr_in server_addr = server_addr_.getSockAddrInet();
    int val = sockets::connect(sockfd,
                               sockets::sockaddr_cast(&server_addr),
                               sizeof(server_addr));
    int saved_errno = (val == 0) ? 0 : errno;
    switch (saved_errno)
    {
        case 0:
        case EINPROGRESS:          // Operation now in progress
        case EINTR:                // Interrupted system call
        case EISCONN:              // Transport endpoint is already connected
            connecting(sockfd);
            break;
        case EAGAIN:               // Try again
        case EADDRINUSE:           // Address already in use
        case EADDRNOTAVAIL:        // Cannot assign requested address
        case ECONNREFUSED:         // Connection refused
        case ENETUNREACH:          // Network is unreachable
            retry(sockfd);
            break;
        case EACCES:               // Permission denied
        case EPERM:                // Operation not permitted
        case EAFNOSUPPORT:         // Address family not supported by protocol
        case EALREADY:             // Operation already in progress
        case EBADF:                // Bad file number
        case EFAULT:               // Bad address
        case ENOTSOCK:             // Socket operation on non-socket
            LOG_ERROR << "connect error in Connector::startInLoop " << saved_errno;
            sockets::close(sockfd);
            break;
        default:
            LOG_ERROR << "unexpected error in Connector::startInLoop " << saved_errno;
            sockets::close(sockfd);
            break;
    }
}

void Connector::connecting(int sockfd)
{
    setState(kConnecting);
    assert(!channel_);
    channel_.reset(new Channel(loop_, sockfd));
    channel_->setWriteCallback(boost::bind(&Connector::handleWrite, this));  // unsafe
    channel_->setErrorCallback(boost::bind(&Connector::handleError, this));  // unsafe
    channel_->enableWriting();  // chanel_->tie(shared_from_tihs()) is not working
}                              // as channel_ is not managed by shared_ptr.

void Connector::restart()
{
    loop_->assertInLoopThread();
    setState(kDisconnected);
    retry_delay_ms_ = kInitRetryDelayMs;
    connect_ = true;
    startInLoop();
}

void Connector::stop()
{
    connect_ = false;
    loop_->queueInLoop(boost::bind(&Connector::stopInLoop, this));  // unsafe
}

void Connector::stopInLoop()
{
    loop_->assertInLoopThread();
    if (state_ == kConnecting)
    {
        setState(kDisconnected);
        int sockfd = removeAndResetChannel();
        retry(sockfd);
    }
}

int Connector::removeAndResetChannel()
{
    channel_->disableAll();
    channel_->remove();
    int sockfd = channel_->fd();
    // Can't reset channel_ here, because we are inside Channel::handleEvent
    loop_->queueInLoop(boost::bind(&Connector::resetChannel, this));  // unsafe
    return sockfd;
}

void Connector::resetChannel()
{
    channel_.reset();
}

void Connector::retry(int sockfd)
{
    sockets::close(sockfd);
    setState(kDisconnected);
    if (connect_)
    {
        LOG_INFO << "Connector::retry - Retry connecting to " << server_addr_.toIpPort()
                 << " in " << retry_delay_ms_ << " milliseconds. ";
        loop_->runAfter(retry_delay_ms_ / 1000.0,
                        boost::bind(&Connector::startInLoop, shared_from_this()));
        retry_delay_ms_ = std::min(retry_delay_ms_ * 2, kMaxRetryDelayMs);
    }
    else
    {
        LOG_DEBUG << "do not connect";
    }
}

void Connector::handleWrite()
{
    LOG_TRACE << "Connector::handleWrite " << state_;
    if (state_ == kConnecting)
    {
        int sockfd = removeAndResetChannel();
        int err = sockets::getSocketError(sockfd);
        if (err)
        {
            LOG_WARN << "Connector::handleWrite - SO_ERROR = " << err << strerror_rb(err);
            retry(sockfd);
        }
        else if (sockets::isSelfConnect(sockfd))
        {
            LOG_WARN << "Connector::handleWrite - Self connect";
            retry(sockfd);
        }
        else
        {
            setState(kConnected);
            if (connect_)
            {
                new_connection_callback_(sockfd);
            }
            else
            {
                sockets::close(sockfd);
            }
        }
    }
    else
    {
        assert(state_ == kDisconnected);  // what happened?
    }
}

void Connector::handleError()
{
    LOG_ERROR << "Connector::handleError state = " << state_;
    if (state_ == kConnecting)
    {
        int sockfd = removeAndResetChannel();
        int err = sockets::getSocketError(sockfd);
        LOG_TRACE << "SO_ERROR = " << err << strerror_rb(err);
        retry(sockfd);
    }
}

}  // namespace blink
