#include "TcpServer.h"
#include "SocketBase.h"
#include "EventLoop.h"
#include "EventLoopThreadPool.h"
#include "Acceptor.h"
#include "Socket.h"
#include "Log.h"

#include <boost/bind.hpp>

#include <sys/socket.h>

#include <assert.h>
#include <stdio.h>

namespace blink
{

TcpServer::TcpServer(EventLoop* loop,
                     const InetAddress& listen_addr,
                     const string& server_name,
                     Option option)
    : loop_(CHECK_NOTNULL(loop)),
      hostport_(listen_addr.toIpPort()),
      name_(server_name),
      acceptor_(new Acceptor(loop, listen_addr, option == kReusePort)),
      thread_pool_(new EventLoopThreadPool(loop, name_)),
      connection_callback_(defaultConnectionCallback),
      message_callback_(defaultMessageCallback),
      next_connection_id_(1)
{
    acceptor_->setNewConnectionCallback(boost::bind(&TcpServer::newConnection, this, _1, _2));
}

TcpServer::~TcpServer()
{
    loop_->assertInLoopThread();
    LOG_TRACE << "TcpServer::~TcpServer [" << name_ << "] destructing";
    for (ConnectionMap::iterator it = connections_.begin(); it != connections_.end(); ++it)
    {
        TcpConnectionPtr connection = it->second;
        it->second.reset();
        connection->getLoop()->runInLoop(boost::bind(&TcpConnection::connectDestroyed, connection));
        connection.reset();
    }
}

void TcpServer::start()
{
    if (started_.getAndSet(1) == 0)
    {
        thread_pool_->start(thread_init_callback_);
        assert(!acceptor_->listenning());
        loop_->runInLoop(boost::bind(&Acceptor::listen, boost::get_pointer(acceptor_)));
    }
}

void TcpServer::setThreadNumber(int number_threads)
{
    assert(number_threads >= 0);
    thread_pool_->setThreadNumber(number_threads);
}

void TcpServer::newConnection(int sockfd, const InetAddress& peer_addr)
{
    loop_->assertInLoopThread();
    EventLoop* ioloop = thread_pool_->getNextLoop();
    char buf[32];
    snprintf(buf, sizeof(buf), "%s#%d", hostport_.c_str(), next_connection_id_);
    ++next_connection_id_;
    string connection_name = name_ + buf;
    LOG_INFO << "TcpServer::newConnection [" << name_ << "] - new connection ["
             << connection_name << "] from " << peer_addr.toIpPort();
    InetAddress local_addr(sockets::getLocalAddr(sockfd));
    TcpConnectionPtr connection(new TcpConnection(ioloop, connection_name, sockfd, local_addr, peer_addr));
    connections_[connection_name] = connection;
    connection->setConnectionCallback(connection_callback_);
    connection->setMessageCallback(message_callback_);
    connection->setWriteCompleteCallback(write_complete_callback_);
    connection->setCloseCallback(boost::bind(&TcpServer::removeConnection, this, _1));
    ioloop->runInLoop(boost::bind(&TcpConnection::connectEstablished, connection));
}

void TcpServer::removeConnection(const TcpConnectionPtr& connection)
{
    loop_->runInLoop(boost::bind(&TcpServer::removeConnectionInLoop, this, connection));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& connection)
{
    loop_->assertInLoopThread();
    LOG_INFO << "TcpServer::removeConnectionInLoop [" << name_ << "] - connection "
             << connection->name();
    size_t n = connections_.erase(connection->name());
    (void)n;
    assert(n == 1);
    EventLoop* ioloop = connection->getLoop();
    ioloop->queueInLoop(boost::bind(&TcpConnection::connectDestroyed, connection));
}

}  // namespace blink
