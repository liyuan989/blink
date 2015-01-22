#include "Nocopyable.h"
#include "EventLoop.h"
#include "TcpServer.h"
#include "Log.h"

#include <boost/bind.hpp>

#include <unistd.h>

#include <list>
#include <stdio.h>

using namespace blink;

const int64_t kTimeZoneValue = static_cast<int64_t>(8) * 3600 * 1000 * 1000;

class EchoServer : Nocopyable
{
public:
    EchoServer(EventLoop* loop, const InetAddress& listen_addr, int idle_seconds);

    void start()
    {
        server_.start();
    }

private:
    void onConnection(const TcpConnectionPtr& connection);
    void onMessage(const TcpConnectionPtr& connection,
                   Buffer* buf,
                   Timestamp receive_time);
    void onTimer();
    void dumpConnectionList();

    typedef boost::weak_ptr<TcpConnection> WeakTcpConnectionPtr;
    typedef std::list<WeakTcpConnectionPtr> WeakConnectionList;

    struct Node
    {
        Timestamp                     last_receive_time;
        WeakConnectionList::iterator  position;
    };

    TcpServer           server_;
    int                 idle_seconds_;
    WeakConnectionList  connection_list_;
};

EchoServer::EchoServer(EventLoop* loop, const InetAddress& listen_addr, int idle_seconds)
    : server_(loop, listen_addr, "EchoServer"), idle_seconds_(idle_seconds)
{
    server_.setConnectionCallback(boost::bind(&EchoServer::onConnection, this, _1));
    server_.setMessageCallback(boost::bind(&EchoServer::onMessage, this, _1, _2, _3));
    loop->runEvery(1.0, boost::bind(&EchoServer::onTimer, this));
    dumpConnectionList();
}

void EchoServer::onConnection(const TcpConnectionPtr& connection)
{
    LOG_INFO << "EchoServer - " << connection->peerAddress().toIpPort() << " -> "
             << connection->localAddress().toIpPort() << " is "
             << (connection->connected() ? "UP" : "DOWN");
    if (connection->connected())
    {
        Node node;
        node.last_receive_time = Timestamp::now();
        connection_list_.push_back(connection);
        node.position = --connection_list_.end();
        connection->setContext(node);
    }
    else
    {
        assert(!connection->getContext().empty());
        const Node& node = boost::any_cast<Node>(connection->getContext());
        connection_list_.erase(node.position);
    }
    dumpConnectionList();
}

void EchoServer::onMessage(const TcpConnectionPtr& connection,
                           Buffer* buf,
                           Timestamp receive_time)
{
    std::string message = buf->resetAllToString();
    int64_t microseconds = receive_time.microSecondsSinceEpoch() + kTimeZoneValue;
    LOG_INFO << connection->name() << " echo " << message.size() << " bytes at "
             << Timestamp(microseconds).toFormattedString();
    connection->send(message);
    assert(!connection->getContext().empty());
    Node* node = boost::any_cast<Node>(connection->getMutableContext());
    node->last_receive_time = receive_time;
    connection_list_.splice(connection_list_.end(), connection_list_, node->position);
    assert(node->position == --connection_list_.end());
    dumpConnectionList();
}

void EchoServer::onTimer()
{
    dumpConnectionList();
    Timestamp now = Timestamp::now();
    for (WeakConnectionList::iterator it = connection_list_.begin();
         it != connection_list_.end();)
    {
        TcpConnectionPtr connection = it->lock();
        if (connection)
        {
            Node* node = boost::any_cast<Node>(connection->getMutableContext());
            double age = timeDifference(now, node->last_receive_time);
            if (age > idle_seconds_)
            {
                if (connection->connected())
                {
                    connection->shutdown();
                    LOG_INFO << "shutting down " << connection->name();
                    connection->forceCloseWithDelay(3.5);  // > round trip of whole internet.
                }
            }
            else if (age < 0)
            {
                LOG_WARN << "Time jump";
                node->last_receive_time = now;
            }
            else
            {
                break;
            }
            ++it;
        }
        else
        {
            LOG_WARN << "expired";
            it = connection_list_.erase(it);
        }
    }
}

void EchoServer::dumpConnectionList()
{
    LOG_INFO << "size = " << connection_list_.size();
    for (WeakConnectionList::const_iterator it = connection_list_.begin();
         it != connection_list_.end(); ++it)
    {
        TcpConnectionPtr connection = it->lock();
        if (connection)
        {
            printf("connection %p\n", boost::get_pointer(connection));
            const Node& node = boost::any_cast<Node>(connection->getContext());
            int64_t miroseconds = node.last_receive_time.microSecondsSinceEpoch() + kTimeZoneValue;
            printf("           time %s\n", Timestamp(miroseconds).toFormattedString().c_str());
        }
        else
        {
            printf("expired\n");
        }
    }
}

int main(int argc, char const *argv[])
{
    EventLoop loop;
    InetAddress listen_addr(9600);
    int idle_seconds = 10;
    if (argc > 1)
    {
        idle_seconds = atoi(argv[1]);
    }
    LOG_INFO << "pid = " << getpid() << ", tid = " << tid()
             << "idle seconds = " << idle_seconds;
    EchoServer server(&loop, listen_addr, idle_seconds);
    server.start();
    loop.loop();
    return 0;
}
