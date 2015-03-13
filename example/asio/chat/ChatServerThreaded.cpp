#include <example/asio/chat/Codec.h>

#include <blink/TcpServer.h>
#include <blink/EventLoop.h>
#include <blink/MutexLock.h>
#include <blink/Log.h>

#include <boost/bind.hpp>

#include <set>
#include <stdio.h>

using namespace blink;

class ChatServer : Nocopyable
{
public:
    ChatServer(EventLoop* loop, const InetAddress& listen_addr)
        : server_(loop, listen_addr, "ChatServer"),
          codec_(boost::bind(&ChatServer::onStringMessage, this, _1, _2, _3))
    {
        server_.setConnectionCallback(boost::bind(&ChatServer::onConnection, this, _1));
        server_.setMessageCallback(boost::bind(&Codec::onMessage, &codec_, _1, _2, _3));
    }

    void start()
    {
        server_.start();
    }

    void setThreadnumber(int num)
    {
        server_.setThreadNumber(num);
    }

private:
    void onConnection(const TcpConnectionPtr& connection)
    {
        LOG_INFO << connection->peerAddress().toIpPort() << " -> "
                 << connection->localAddress().toIpPort() << " is "
                 << (connection->connected() ? "UP" : "DOWN");
        MutexLockGuard guard(mutex_);
        if (connection->connected())
        {
            connections_.insert(connection);
        }
        else
        {
            connections_.erase(connection);
        }
    }

    void onStringMessage(const TcpConnectionPtr& connection,
                         const string& message,
                         Timestamp receive_time)
    {
        MutexLockGuard guard(mutex_);
        for (ConnectionList::iterator it = connections_.begin();
             it != connections_.end(); ++it)
        {
            codec_.send(boost::get_pointer(*it), message);
        }
    }

    typedef std::set<TcpConnectionPtr> ConnectionList;

    TcpServer       server_;
    Codec           codec_;
    MutexLock       mutex_;
    ConnectionList  connections_;
};

int main(int argc, char const *argv[])
{
    LOG_INFO << "pid = " << getpid() << ", tid = " << tid();
    EventLoop loop;
    InetAddress server_addr(9600);
    ChatServer server(&loop, server_addr);
    server.setThreadnumber(5);
    server.start();
    loop.loop();
    return 0;
}
