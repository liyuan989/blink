#include <blink/TcpConnection.h>
#include <blink/SocketBase.h>
#include <blink/EventLoop.h>
#include <blink/Channel.h>
#include <blink/Buffer.h>
#include <blink/Socket.h>
#include <blink/Timestamp.h>
#include <blink/WeakCallback.h>
#include <blink/Log.h>

#include <boost/bind.hpp>

#include <errno.h>
#include <assert.h>

namespace blink
{

void defaultConnectionCallback(const TcpConnectionPtr& connection)
{
    LOG_TRACE << connection->localAddress().toIpPort() << " -> "
              << connection->peerAddress().toIpPort() << " is "
              << (connection->connected() ? "UP" : "DOWN");
}

void defaultMessageCallback(const TcpConnectionPtr& connection, Buffer* buffer, Timestamp receive_time)
{
    buffer->resetAll();
}

TcpConnection::TcpConnection(EventLoop* loop,
                             const string& connection_name,
                             int sockfd,
                             const InetAddress& local_addr,
                             const InetAddress& peer_addr)
    : loop_(CHECK_NOTNULL(loop)),
      name_(connection_name),
      state_(kConnecting),
      socket_(new Socket(sockfd)),
      channel_(new Channel(loop, sockfd)),
      local_addr_(local_addr),
      peer_addr_(peer_addr),
      high_water_mark_(64 * 1024 * 1024)
{
    channel_->setReadCallback(boost::bind(&TcpConnection::handleRead, this, _1));
    channel_->setWriteCallback(boost::bind(&TcpConnection::handleWrite, this));
    channel_->setCloseCallback(boost::bind(&TcpConnection::handleClose, this));
    channel_->setErrorCallback(boost::bind(&TcpConnection::handleError, this));
    LOG_DEBUG << "TcpConnection::contructor[" << name_ << "] at " << this << " fd = " << sockfd;
    socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection()
{
    LOG_DEBUG << "TcpConnection::destructor[" << name_ << "] at " << this
              << " fd = " << channel_->fd() << " state = " << stateToString();
    assert(state_ == kDisconnected);
}

const char* TcpConnection::stateToString() const
{
    switch (state_)
    {
        case kConnected:
            return "kConnected";
        case kConnecting:
            return "kConnecting";
        case kDisconnected:
            return "kDisconnected";
        case kDisconnecting:
            return "kDisconnecting";
        default:
            return "unkown state";
    }
}

bool TcpConnection::getTcpInfo(struct tcp_info* tcpi) const
{
    return socket_->getTcpInfo(tcpi);
}

string TcpConnection::getTcpInfoString() const
{
    char buf[1024];
    buf[0] = '\0';
    socket_->getTcpInfoString(buf, sizeof(buf));
    return buf;
}

void TcpConnection::send(const void* data, int len)
{
    send(StringPiece(static_cast<const char*>(data), len));
}

void TcpConnection::send(const StringPiece& message)
{
    if (state_ == kConnected)
    {
        if (loop_->isInLoopThread())
        {
            sendInLoop(message);
        }
        else
        {
            loop_->runInLoop(boost::bind(&TcpConnection::sendInLoop, this, message.asString()));
            // FIXME: use std::forward<string>(message)
        }
    }
}

void TcpConnection::send(Buffer* buf)
{
    if (state_ == kConnected)
    {
        if (loop_->isInLoopThread())
        {
            sendInLoop(buf->peek(), buf->readableSize());
            buf->resetAll();
        }
        else
        {
            loop_->runInLoop(boost::bind(&TcpConnection::sendInLoop, this, buf->resetAllToString()));
        }
    }
}

void TcpConnection::sendInLoop(const StringPiece& message)
{
    sendInLoop(message.data(), message.size());
}

void TcpConnection::sendInLoop(const void* data, size_t len)
{
    loop_->assertInLoopThread();
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool fault_error = false;
    if (state_ == kDisconnected)
    {
        LOG_ERROR << "disconnected, give up writing";
        return;
    }
    if (!channel_->isWriting() && (output_buffer_.readableSize() == 0))
    {
        nwrote = sockets::write(channel_->fd(), data, len);
        if (nwrote >= 0)
        {
            remaining = len - nwrote;
            if (remaining == 0 && write_complete_callback_)
            {
                loop_->queueInLoop(boost::bind(write_complete_callback_, shared_from_this()));
            }
        }
        else // nwrote < 0
        {
            nwrote = 0;
            LOG_ERROR << "TcpConnection::sendInLoop";
            if (errno == EPIPE || errno == ECONNRESET)  // Broken pipe or Connection reset by peer
            {
                fault_error = true;
            }
        }
    }
    assert(remaining <= len);
    if (!fault_error && (remaining > 0))
    {
        size_t old_len = output_buffer_.readableSize();
        if (old_len + remaining >= high_water_mark_
            && old_len < high_water_mark_
            && high_water_mark_callback_)
        {
            loop_->queueInLoop(boost::bind(high_water_mark_callback_,
                                           shared_from_this(),
                                           old_len + remaining));
        }
        output_buffer_.append(static_cast<const char*>(data) + nwrote, remaining);
        if (!channel_->isWriting())
        {
            channel_->enableWriting();
        }
    }
}

void TcpConnection::shutdown()
{
    if (state_ == kConnected)
    {
        setState(kDisconnecting);
        loop_->runInLoop(boost::bind(&TcpConnection::shutdownInLoop, this));
    }
}

void TcpConnection::shutdownInLoop()
{
    loop_->assertInLoopThread();
    if (!channel_->isWriting())
    {
        socket_->shutdownWrite();
    }
}

//void TcpConnection::shutdownAndForceCloseAfter(double seconds)
//{
//    if (state_ == kConnected)
//    {
//        setState(kDisconnecting);
//        loop_->runInLoop(boost::bind(&TcpConnection::shutdownAndForceCloseInLoop, this, seconds));
//    }
//}
//
//void TcpConnection::shutdownAndForceCloseInLoop(double seconds)
//{
//    loop_->assertInLoopThread();
//    if (!channel_->isWriting())
//    {
//        socket_->shutdownWrite();
//    }
//    loop_->runAfter(seconds, makeWeakCallback(shared_from_this(), &TcpConnection::forceCloseInLoop));
//}

void TcpConnection::forceClose()
{
    if (state_ == kConnected || state_ == kDisconnecting)
    {
        setState(kDisconnecting);
        loop_->queueInLoop(boost::bind(&TcpConnection::forceCloseInLoop, shared_from_this()));
    }
}

void TcpConnection::forceCloseWithDelay(double seconds)
{
    if (state_ == kConnected || state_ == kDisconnecting)
    {
        setState(kDisconnecting);
        loop_->runAfter(seconds, makeWeakCallback(shared_from_this(), &TcpConnection::forceClose));
    } // not forceCloseInLoop to avoid race condition
}

void TcpConnection::forceCloseInLoop()
{
    loop_->assertInLoopThread();
    if (state_ == kConnected || state_ == kDisconnecting)
    {
        handleClose();   // as if we received 0 byte in handleRead()
    }
}

void TcpConnection::setTcpNoDelay(bool on)
{
    socket_->setTcpNoDelay(on);
}

void TcpConnection::connectEstablished()
{
    loop_->assertInLoopThread();
    assert(state_ == kConnecting);
    setState(kConnected);
    channel_->tie(shared_from_this());
    channel_->enableReading();
    connection_callback_(shared_from_this());
}

void TcpConnection::connectDestroyed()
{
    loop_->assertInLoopThread();
    if (state_ == kConnected)
    {
        setState(kDisconnected);
        channel_->disableAll();
        connection_callback_(shared_from_this());
    }
    channel_->remove();
}

void TcpConnection::handleRead(Timestamp receive_time)
{
    loop_->assertInLoopThread();
    int saved_error = 0;
    ssize_t n = input_buffer_.readData(channel_->fd(), &saved_error);
    if (n > 0)
    {
        message_callback_(shared_from_this(), &input_buffer_, receive_time);
    }
    else if (n == 0)
    {
        handleClose();
    }
    else
    {
        errno = saved_error;
        LOG_ERROR << "TcpConnection::handleRead";
        handleError();
    }
}

void TcpConnection::handleWrite()
{
    loop_->assertInLoopThread();
    if (channel_->isWriting())
    {
        ssize_t n = sockets::write(channel_->fd(), output_buffer_.peek(), output_buffer_.readableSize());
        if (n > 0)
        {
            output_buffer_.reset(n);
            if (output_buffer_.readableSize() == 0)
            {
                channel_->disableWriting();
                if (write_complete_callback_)
                {
                    loop_->queueInLoop(boost::bind(write_complete_callback_, shared_from_this()));
                }
                if (state_ == kDisconnecting)
                {
                    shutdownInLoop();
                }
            }
        }
        else  // n <= 0
        {
            LOG_ERROR << "TcpConnection::handleWrite";
            //if (state_ == kDisconnecting)
            //{
            //    shutdownInLoop();
            //}
        }
    }
    else
    {
        LOG_TRACE << "Connection fd = " << channel_->fd() << " is down, no more writing";
    }
}

void TcpConnection::handleClose()
{
    loop_->assertInLoopThread();
    LOG_TRACE << "fd = " << channel_->fd() << " state = " << state_;
    assert(state_ == kConnected || state_ == kDisconnecting);
    setState(kDisconnected);  // Don't close fd, leave it to dustructor, so we can find leak easily.
    channel_->disableAll();
    TcpConnectionPtr guard_this(shared_from_this());
    connection_callback_(guard_this);
    close_callback_(guard_this);  // must be the last line.
}

void TcpConnection::handleError()
{
    int err = sockets::getSocketError(channel_->fd());
    LOG_ERROR << "TcpConnection::handleError [" << name_ << "] - SO_ERROR = "
              << err << " " << blink::strerror_tl(err);
}

}  // namespace blink
