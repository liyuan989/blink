#include <example/asio/chat/Codec.h>

#include <blink/TcpClient.h>
#include <blink/EventLoop.h>
#include <blink/EventLoopThread.h>
#include <blink/EventLoopThreadPool.h>
#include <blink/MutexLock.h>
#include <blink/CurrentThread.h>
#include <blink/Log.h>

#include <boost/bind.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

#include <iostream>
#include <stdlib.h>
#include <stdio.h>

using namespace blink;

int                       g_connections = 0;
AtomicInt32               g_aliveConnections;
AtomicInt32               g_messagereceived;
Timestamp                 g_startTime;
std::vector<Timestamp>    g_receiveTime;
EventLoop*                g_loop;
boost::function<void ()>  g_statistic;

class ChatClient : Nocopyable
{
public:
    ChatClient(EventLoop* loop, const InetAddress& server_addr)
        : loop_(loop),
          client_(loop, server_addr, "ChatClient"),
          codec_(boost::bind(&ChatClient::onStringMessage, this, _1, _2, _3))
    {
        client_.setConnectionCallback(boost::bind(&ChatClient::onConnection, this, _1));
        client_.setMessageCallback(boost::bind(&Codec::onMessage, &codec_, _1, _2, _3));
    }

    void connect()
    {
        client_.connect();
    }

    Timestamp receiveTime() const
    {
        return receive_time_;
    }

private:
    void onConnection(const TcpConnectionPtr& connection)
    {
        LOG_INFO << connection->peerAddress().toIpPort() << " -> "
                 << connection->localAddress().toIpPort() << " is "
                 << (connection->connected() ? "UP" : "DOWN");
        if (connection->connected())
        {
            connection_ = connection;
            if (g_aliveConnections.incrementAndGet() == g_connections)
            {
                LOG_INFO << "all connected";
                loop_->runAfter(3.0, boost::bind(&ChatClient::send, this));
            }
        }
        else
        {
            connection_.reset();
        }
    }

    void onStringMessage(const TcpConnectionPtr& connection,
                         const string& message,
                         Timestamp receive_time)
    {
        receive_time_ = loop_->pollReturnTime();
        int received = g_messagereceived.incrementAndGet();
        if (received == g_connections)
        {
            Timestamp end_time = Timestamp::now();
            LOG_INFO << "all received " << g_connections << " in "
                     << timeDifference(end_time, g_startTime);
            g_loop->queueInLoop(g_statistic);
        }
        else if (received % 1000 == 0)
        {
            LOG_DEBUG << received;
        }
    }

    void send()
    {
        g_startTime = Timestamp::now();
        codec_.send(boost::get_pointer(connection_), "hey");
        LOG_DEBUG << "send";
    }

    EventLoop*        loop_;
    TcpClient         client_;
    Codec             codec_;
    TcpConnectionPtr  connection_;
    Timestamp         receive_time_;
};

void statistic(const boost::ptr_vector<ChatClient>& clients)
{
    LOG_INFO << "statistic " << clients.size();
    std::vector<double> seconds(clients.size());
    for (size_t i = 0; i < seconds.size(); ++i)
    {
        seconds[i] = timeDifference(clients[i].receiveTime(), g_startTime);
    }
    std::sort(seconds.begin(), seconds.end());
    for (size_t i = 0; i < clients.size();
         i += std::max(static_cast<size_t>(1), clients.size() / 20))
    {
        printf("%6zd%% %.6f\n", i * 100 / clients.size(), seconds[i]);
    }
    if (clients.size() >= 100)
    {
        printf("%6d%% %.6f\n", 99, seconds[clients.size() - clients.size()/100]);
    }
    printf("%6d%% %.6f\n", 100, seconds.back());
}

int main(int argc, char const *argv[])
{
    if (argc < 3)
    {
        fprintf(stderr, "usage: %s <ip> <port> connections [threads]\n", argv[0]);
        return 1;
    }
    int threads = 0;
    if (argc >= 4)
    {
        g_connections = atoi(argv[3]);
    }
    if (argc == 5)
    {
        threads = atoi(argv[4]);
    }
    LOG_INFO << "pid = " << getpid() << ", tid = " << current_thread::tid();
    InetAddress server_addr(argv[1], static_cast<uint16_t>(atoi(argv[2])));
    EventLoop loop;
    g_loop = &loop;
    EventLoopThreadPool loop_pool(&loop, "char-loadtest");
    loop_pool.setThreadNumber(threads);
    loop_pool.start();
    g_receiveTime.reserve(g_connections);
    boost::ptr_vector<ChatClient> clients(g_connections);
    g_statistic = boost::bind(statistic, boost::ref(clients));
    for (int i = 0; i < g_connections; ++i)
    {
        clients.push_back(new ChatClient(loop_pool.getNextLoop(), server_addr));
        clients[i].connect();
        current_thread::sleepMicroseconds(200 * 1000);
    }
    loop.loop();
    return 0;
}
