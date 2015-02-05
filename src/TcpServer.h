#ifndef __BLINK_TCPSERVER_H__
#define __BLINK_TCPSERVER_H__

#include "Nocopyable.h"
#include "TcpConnection.h"
#include "Atomic.h"

#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/function.hpp>

#include <map>

namespace blink
{

class EventLoop;
class EventLoopThreadPool;
class Acceptor;

// TCP server, supports single-threaded and thread-pool models.
class TcpServer : Nocopyable
{
public:
    typedef boost::function<void (EventLoop*)> ThreadInitCallback;

    enum Option
    {
        kNoReusePort,
        kReusePort,
    };

    TcpServer(EventLoop* loop,
              const InetAddress& listen_addr,
              const string& server_name,
              Option option = kNoReusePort);

    // force out-line destructor, for scoped_ptr members.
    ~TcpServer();

    // Starts the server if it's not listening.
    // It's harmless to call it multiple times.
    // Thread safe.
    void start();

    // Set the number of threads for handling input.
    //
    // Always accepts new connection in loop's thread.
    // Must be called before start.
    //
    // @param number_threads:
    // - 0 means all I/O in loop's thread , no thread will created.
    //   this is the default value.
    // - 1 means all I/O in another thread.
    // - N means a thread pool with N threads, new connection are
    //   assigned on a round-robin basis.
    void setThreadNumber(int number_threads);

    const string& hostport() const
    {
        return hostport_;
    }

    const string& name() const
    {
        return name_;
    }

    EventLoop* getLoop() const
    {
        return loop_;
    }

    // valid after calling start().
    boost::shared_ptr<EventLoopThreadPool> threadPool()
    {
        return thread_pool_;
    }

    void setThreadInitCallback(const ThreadInitCallback& cb)
    {
        thread_init_callback_ = cb;
    }

    // Set connection callback, not thread safe.
    void setConnectionCallback(const ConnectionCallback& cb)
    {
        connection_callback_ = cb;
    }

    // Set message callback , not thread safe.
    void setMessageCallback(const MessageCallback& cb)
    {
        message_callback_ = cb;
    }

    // Set writhe complete callback, not thread safe.
    void setWriteCompleteCallback(const WriteCompleteCallback& cb)
    {
        write_complete_callback_ = cb;
    }

private:
    // Not thread safe, but in loop.
    void newConnection(int sockfd, const InetAddress& peer_addr);

    // Thread safe.
    void removeConnection(const TcpConnectionPtr& connection);

    // Not thread safe, but in loop.
    void removeConnectionInLoop(const TcpConnectionPtr& connection);

    typedef std::map<string, TcpConnectionPtr> ConnectionMap;

    EventLoop*                              loop_;                     // the acceptor loop
    const string                            hostport_;
    const string                            name_;
    boost::scoped_ptr<Acceptor>             acceptor_;                 // avoid revealing Acceptor
    boost::shared_ptr<EventLoopThreadPool>  thread_pool_;
    ConnectionCallback                      connection_callback_;
    MessageCallback                         message_callback_;
    WriteCompleteCallback                   write_complete_callback_;
    ThreadInitCallback                      thread_init_callback_;
    AtomicInt32                             started_;
    int                                     next_connection_id_;
    ConnectionMap                           connections_;
};

}  // namespace blink

#endif
