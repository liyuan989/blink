#ifndef __BLINK_TCPCONNECTION_H__
#define __BLINK_TCPCONNECTION_H__

#include "Nocopyable.h"
#include "Buffer.h"
#include "InetAddress.h"
#include "Callbacks.h"

#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/any.hpp>

#include <string>

struct tcp_info;  // defined in <netinet/tcp.h>

namespace blink
{

class EventLoop;
class Channel;
class Socket;
class Timestamp;

class TcpConnection : Nocopyable,
                      public boost::enable_shared_from_this<TcpConnection>
{
public:
    // Constructs a TcpConnection with a connected sockfd.
    TcpConnection(EventLoop* loop,
                  const std::string& connection_name,
                  int sockfd,
                  const InetAddress& local_addr,
                  const InetAddress& peer_addr);
    ~TcpConnection();

    // return true if success.
    bool getTcpInfo(struct tcp_info* tcpi) const;
    std::string getTcpInfoString() const;

    void send(const void* data, size_t len);
    void send(const std::string& data);

    // This one will swap data.
    void send(Buffer* buf);

    // Not thread safe, no simultaneous calling.
    void shutdown();

    // Not thread safe, no simultaneous calling.
    //void shutdownAndForceCloseAfter(double seconds);
    void forceClose();
    void forceCloseWithDelay(double seconds);
    void setTcpNoDelay(bool on);

    // Called when TcpServer accepts a new connection.
    // Should be called only once.
    void connectEstablished();

    // Called when TcpServer has removed me from its map.
    // Should be called only once.
    void connectDestroyed();

    EventLoop* getLoop() const
    {
        return loop_;
    }

    const std::string& name() const
    {
        return name_;
    }

    const InetAddress& localAddress() const
    {
        return local_addr_;
    }

    const InetAddress& peerAddress() const
    {
        return peer_addr_;
    }

    bool connected() const
    {
        return state_ == kConnected;
    }

    void setContext(const boost::any& context)
    {
        context_ = context;
    }

    const boost::any& getContext() const
    {
        return context_;
    }

    boost::any* getMutableContext()
    {
        return &context_;
    }

    void setConnectionCallback(const ConnectionCallback& cb)
    {
        connection_callback_ = cb;
    }

    void setMessageCallback(const MessageCallback& cb)
    {
        message_callback_ = cb;
    }

    void setWriteCompleteCallback(const WriteCompleteCallback& cb)
    {
        write_complete_callback_ = cb;
    }

    void setHighWaterMarkCallback(const HighWaterMarkCallback& cb)
    {
        high_water_mark_callback_ = cb;
    }

    void setCloseCallback(const CloseCallback& cb)
    {
        close_callback_ = cb;
    }

    Buffer* inputBuffer()
    {
        return &input_buffer_;
    }

    Buffer* outputBuffer()
    {
        return &output_buffer_;
    }

private:
    enum StateE
    {
        kDisconnected,
        kConnecting,
        kConnected,
        kDisconnecting,
    };

    void handleRead(Timestamp receive_time);
    void handleWrite();
    void handleClose();
    void handleError();
    void sendInLoop(const std::string& data);
    void sendInLoop(const void* data, size_t len);
    void shutdownInLoop();
    //void shutdownAndForceCloseInLoop(double seconds);
    void forceCloseInLoop();

    void setState(StateE state)
    {
        state_ = state;
    }

    EventLoop*                  loop_;
    const std::string           name_;
    StateE                      state_;
    boost::scoped_ptr<Socket>   socket_;
    boost::scoped_ptr<Channel>  channel_;
    const InetAddress           local_addr_;
    const InetAddress           peer_addr_;
    ConnectionCallback          connection_callback_;
    MessageCallback             message_callback_;
    WriteCompleteCallback       write_complete_callback_;
    HighWaterMarkCallback       high_water_mark_callback_;
    CloseCallback               close_callback_;
    size_t                      high_water_mark_;
    Buffer                      input_buffer_;
    Buffer                      output_buffer_;
    boost::any                  context_;
};

typedef boost::shared_ptr<TcpConnection> TcpConnectionPtr;

}  // namespace blink

#endif
