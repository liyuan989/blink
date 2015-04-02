#include <blink/TcpClient.h>
#include <blink/SocketBase.h>
#include <blink/EventLoop.h>
#include <blink/Connector.h>
#include <blink/Log.h>

#include <boost/bind.hpp>

#include <assert.h>
#include <stdio.h>

namespace blink
{

namespace detail
{

void removeConnection(EventLoop* loop, const TcpConnectionPtr& connection)
{
    loop->queueInLoop(boost::bind(&TcpConnection::connectDestroyed, connection));
}

void removeConnector(const ConnectorPtr& connector)
{
    // TODO
}

}  // namespace detail

TcpClient::TcpClient(EventLoop* loop, const InetAddress& server_addr, const string& name_arg)
    : loop_(CHECK_NOTNULL(loop)),
      connector_(new Connector(loop, server_addr)),
      name_(name_arg),
      connection_callback_(defaultConnectionCallback),
      message_callback_(defaultMessageCallback),
      retry_(false),
      connect_(false),
      next_connection_id_(1)
{
    connector_->setNewConnectionCallback(boost::bind(&TcpClient::newConnection, this, _1));
    LOG_INFO << "TcpClient::TcpClient[" << name_              // FIXME: setConnectFailCallback
             << "] - connector " << boost::get_pointer(connector_);
}

TcpClient::~TcpClient()
{
    LOG_INFO << "TcpClient::~TcpClient[" << name_
             << "] - connector " << boost::get_pointer(connector_);
    TcpConnectionPtr connection;
    bool unique = false;
    {
        MutexLockGuard guard(mutex_);
        unique = connection_.unique();
        connection = connection_;
    }
    if (connection)
    {
        assert(loop_ = connection->getLoop());
        // not 100% safe, if we are in different thread.
        CloseCallback cb = boost::bind(detail::removeConnection, loop_, _1);
        loop_->runInLoop(boost::bind(&TcpConnection::setCloseCallback, connection, cb));
        if (unique)
        {
            connection->forceClose();
        }
    }
    else
    {
        connector_->stop();
        loop_->runAfter(1, boost::bind(detail::removeConnector, connector_));
    }
}

void TcpClient::connect()
{   // FIXME: check state
    LOG_INFO << "TcpClient::connect[" << name_ << "] - connecting to "
             << connector_->serverAddress().toIpPort();
    connect_ = true;
    connector_->start();
}

void TcpClient::disconnect()
{
    connect_ = false;
    {
        MutexLockGuard guard(mutex_);
        if (connection_)
        {
            connection_->shutdown();
        }
    }
}

void TcpClient::stop()
{
    connect_ = false;
    connector_->stop();
}

void TcpClient::newConnection(int sockfd)
{
    loop_->assertInLoopThread();
    InetAddress peer_addr(sockets::getPeerAddr(sockfd));
    char buf[32];
    snprintf(buf, sizeof(buf), ":%s#%d", peer_addr.toIpPort().c_str(), next_connection_id_);
    ++next_connection_id_;
    string connection_name = name_ + buf;
    InetAddress locak_addr(sockets::getLocalAddr(sockfd));
    TcpConnectionPtr connection(new TcpConnection(loop_, connection_name, sockfd, locak_addr, peer_addr));
    connection->setConnectionCallback(connection_callback_);
    connection->setMessageCallback(message_callback_);
    connection->setWriteCompleteCallback(write_complete_callback_);
    connection->setCloseCallback(boost::bind(&TcpClient::removeConnection, this, _1));   // unsafe
    {
        MutexLockGuard guard(mutex_);
        connection_ = connection;
    }
    connection->connectEstablished();
}

void TcpClient::removeConnection(const TcpConnectionPtr& connection)
{
    loop_->assertInLoopThread();
    assert(loop_ == connection->getLoop());
    {
        MutexLockGuard guard(mutex_);
        assert(connection_ == connection);
        connection_.reset();
    }
    loop_->queueInLoop(boost::bind(&TcpConnection::connectDestroyed, connection));
    if (retry_ && connect_)
    {
        LOG_INFO << "TcpClient::connect[" << name_ << "] - Reconnecting to "
                 << connector_->serverAddress().toIpPort();
        connector_->restart();
    }
}

}  // namespace blink
