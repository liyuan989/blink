#ifndef __BLINK_CONNECTOR_H__
#define __BLINK_CONNECTOR_H__

#include "Nocopyable.h"
#include "InetAddress.h"

#include <boost/enable_shared_from_this.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/function.hpp>

namespace blink
{

class EventLoop;
class Channel;

class Connector : Nocopyable,
                  public boost::enable_shared_from_this<Connector>
{
public:
    typedef boost::function<void (int sockfd)> NewConnectionCallback;

    Connector(EventLoop* loop, const InetAddress& server_addr);
    ~Connector();

    // Can be called in any thread.
    void start();

    // Must be called in loop thread.
    void restart();

    // Can be called in any thread.
    void stop();

    void setNewConnectionCallback(const NewConnectionCallback& cb)
    {
        new_connection_callback_ = cb;
    }

    const InetAddress& serverAddress() const
    {
        return server_addr_;
    }

private:
    enum State
    {
        kDisconnected,
        kConnecting,
        kConnected,
    };

    void startInLoop();
    void stopInLoop();
    void connect();
    void connecting(int sockfd);
    void handleWrite();
    void handleError();
    void retry(int sockfd);
    int removeAndResetChannel();
    void resetChannel();

    void setState(State state)
    {
        state_ = state;
    }

    EventLoop*                  loop_;
    InetAddress                 server_addr_;
    bool                        connect_;                   // atomic
    State                       state_;                     // FIXME: use atomic variable
    boost::scoped_ptr<Channel>  channel_;
    NewConnectionCallback       new_connection_callback_;
    int                         retry_delay_ms_;

    static const int            kMaxRetryDelayMs = 30 * 1000;
    static const int            kInitRetryDelayMs = 500;
};

}  // namespace blink

#endif
