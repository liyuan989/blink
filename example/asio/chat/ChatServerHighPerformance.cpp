#include <example/asio/chat/Codec.h>

#include <blink/TcpServer.h>
#include <blink/EventLoop.h>
#include <blink/MutexLock.h>
#include <blink/ThreadLocalSingleton.h>
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
        server_.setThreadInitCallback(boost::bind(&ChatServer::threadInit, this, _1));
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
        if (connection->connected())
        {
            LocalConnections::getInstance().insert(connection);
        }
        else
        {
            LocalConnections::getInstance().erase(connection);
        }
    }

    void onStringMessage(const TcpConnectionPtr& connection,
                         const string& message,
                         Timestamp receive_time)
    {
        EventLoop::Functor func = boost::bind(&ChatServer::destribueMessage, this, message);
        LOG_DEBUG;
        for (std::set<EventLoop*>::iterator it = loops_.begin(); it != loops_.end(); ++it)
        {
            (*it)->queueInLoop(func);
        }
        LOG_DEBUG;
    }

    void threadInit(EventLoop* loop)
    {
        assert(LocalConnections::pointer() == NULL);
        LocalConnections::getInstance();
        assert(LocalConnections::pointer() != NULL);
        MutexLockGuard guard(mutex_);
        loops_.insert(loop);
    }

    void destribueMessage(const string& message)
    {
        LOG_DEBUG << "begin";
        for (ConnectionList::iterator it = LocalConnections::getInstance().begin();
             it != LocalConnections::getInstance().end(); ++it)
        {
            codec_.send(boost::get_pointer(*it), message);
        }
        LOG_DEBUG << "end";
    }

    typedef std::set<TcpConnectionPtr> ConnectionList;
    typedef ThreadLocalSingleton<ConnectionList> LocalConnections;

    TcpServer             server_;
    Codec                 codec_;
    MutexLock             mutex_;
    std::set<EventLoop*>  loops_;
};

int main(int argc, char const *argv[])
{
    LOG_INFO << "pid = " << getpid() << ", tid = " << current_thread::tid();
    EventLoop loop;
    InetAddress server_addr(9600);
    ChatServer server(&loop, server_addr);
    server.setThreadnumber(5);
    server.start();
    loop.loop();
    return 0;
}
