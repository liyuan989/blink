#include <blink/Acceptor.h>
#include <blink/SocketBase.h>
#include <blink/EventLoop.h>
#include <blink/InetAddress.h>
#include <blink/Log.h>

#include <boost/bind.hpp>

#include <fcntl.h>

#include <errno.h>
#include <assert.h>

namespace blink
{

Acceptor::Acceptor(EventLoop* loop, const InetAddress& listen_addr, bool reuse_port)
    : loop_(loop),
      accept_socket_(sockets::createNonblockingOrDie()),
      accept_channel_(loop_, accept_socket_.fd()),
      listenning_(false),
      idle_fd_(::open("/dev/null", O_RDONLY | O_CLOEXEC))
{
    assert(idle_fd_ >= 0);
    accept_socket_.setReuseAddr(true);
    accept_socket_.setReusePort(reuse_port);
    accept_socket_.bindAddress(listen_addr);
    accept_channel_.setReadCallback(boost::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor()
{
    accept_channel_.disableAll();
    accept_channel_.remove();
    ::close(idle_fd_);
}

void Acceptor::listen()
{
    loop_->assertInLoopThread();
    listenning_ = true;
    accept_socket_.listen();
    accept_channel_.enableReading();
}

void Acceptor::handleRead()
{
    loop_->assertInLoopThread();
    InetAddress peer_addr;
    int connectfd = accept_socket_.accept(&peer_addr);
    if (connectfd >= 0)
    {
        //string hostport = peer_addr.toIpPort();
        //LOG_TRACE << "Accepts of " << hostport;
        if (new_connection_callback_)
        {
            new_connection_callback_(connectfd, peer_addr);
        }
        else
        {
            int val = sockets::close(connectfd);
            if (val < 0)
            {
                LOG_ERROR << "Acceptor::handleRead sockets::close";
            }
        }
    }
    else
    {
        LOG_ERROR << "in Acceptor::handleRead";
        if (errno == EMFILE)
        {
            ::close(idle_fd_);
            idle_fd_ = ::accept(accept_socket_.fd(), NULL, NULL);
            ::close(idle_fd_);
            idle_fd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
        }
    }
}

}  // namespace blink
