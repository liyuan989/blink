#ifndef __BLINK_ACCEPTOR_H__
#define __BLINK_ACCEPTOR_H__

#include "Nocopyable.h"
#include "Channel.h"
#include "Socket.h"

#include <boost/function.hpp>

namespace blink
{

class EventLoop;
class InetAddress;

class Acceptor : Nocopyable
{
public:
    typedef boost::function<void (int connectfd, const InetAddress&)> NewConnectionCallback;

    Acceptor(EventLoop* loop, const InetAddress& listen_addr, bool reuse_port);
    ~Acceptor();

    void listen();

    void setNewConnectionCallback(const NewConnectionCallback& cb)
    {
        new_connection_callback_ = cb;
    }

    bool listenning() const
    {
        return listenning_;
    }

private:
    void handleRead();

    EventLoop*             loop_;
    Socket                 accept_socket_;
    Channel                accept_channel_;
    NewConnectionCallback  new_connection_callback_;
    bool                   listenning_;
    int                    idle_fd_;
};

}  // namespace blink

#endif
