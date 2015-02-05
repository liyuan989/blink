#ifndef __BLINK_TCPCLIENT_H__
#define __BLINK_TCPCLIENT_H__

#include "Nocopyable.h"
#include "TcpConnection.h"
#include "MutexLock.h"

namespace blink
{

class Connector;

typedef boost::shared_ptr<Connector> ConnectorPtr;

class TcpClient : Nocopyable
{
public:
    TcpClient(EventLoop* loop, const InetAddress& server_addr, const string& name);

    // Force out-line destructor, for scoped_ptr member.
    ~TcpClient();

    void connect();
    void disconnect();
    void stop();

    EventLoop* getLoop() const
    {
        return loop_;
    }

    bool retry() const
    {
        return retry_;
    }

    void enableRetry()
    {
        retry_ = true;
    }

    // Set connection callback.
    // Not thread safe.
    void setConnectionCallback(const ConnectionCallback& cb)
    {
        connection_callback_ = cb;
    }

    // Set message callback.
    // Not thread safe.
    void setMessageCallback(const MessageCallback& cb)
    {
        message_callback_ = cb;
    }

    // Set Write complete callback.
    // Not thread safe.
    void setWriteCompleteCallback(const WriteCompleteCallback& cb)
    {
        write_complete_callback_ = cb;
    }

private:
    // Not thread safe, but in loop.
    void newConnection(int sockfd);

    // Not thread safe, but in loop.
    void removeConnection(const TcpConnectionPtr& connection);

    EventLoop*             loop_;
    ConnectorPtr           connector_;
    const string           name_;
    ConnectionCallback     connection_callback_;
    MessageCallback        message_callback_;
    WriteCompleteCallback  write_complete_callback_;
    bool                   retry_;                    // atomic
    bool                   connect_;                  // atomic
    int                    next_connection_id_;       // always in loop thread
    mutable                MutexLock mutex_;
    TcpConnectionPtr       connection_;               // guarded by mutex_
};

}  // namespace blink

#endif
