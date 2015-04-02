#include <blink/TcpServer.h>
#include <blink/EventLoop.h>
#include <blink/Log.h>

#include <boost/bind.hpp>

#include <unistd.h>

#include <string>
#include <stdlib.h>
#include <stdio.h>

using namespace blink;

int thread_number = 0;

class EchoServer
{
public:
    EchoServer(EventLoop* loop, const InetAddress& listen_addr, const string& name)
        : loop_(loop), server_(loop, listen_addr, name)
    {
        server_.setConnectionCallback(boost::bind(&EchoServer::onConnection, this, _1));
        server_.setMessageCallback(boost::bind(&EchoServer::onMessage, this, _1, _2, _3));
        server_.setThreadNumber(thread_number);
    }

    void start()
    {
        server_.start();
    }

private:
    void onConnection(const TcpConnectionPtr& connection)
    {
        LOG_TRACE << connection->peerAddress().toIpPort() << " -> "
                  << connection->peerAddress().toIpPort() << " is "
                  << (connection->connected() ? "UP" : "DOWN");
        LOG_INFO << connection->getTcpInfoString();
        connection->send("hello\n");
    }

    void onMessage(const TcpConnectionPtr& connection, Buffer* buf, Timestamp time)
    {
        string message = buf->resetAllToString();
        LOG_TRACE << connection->name() << " receive " << message.size()
                  << " bytes at " << time.toFormattedString();
        if (message == "exit\n")
        {
            connection->send("bye\n");
            connection->shutdown();
        }
        if (message == "q\n" || message == "quit\n")
        {
            loop_->quit();
        }
        else
        {
            printf("%s\n", message.c_str());
            connection->send(message);
        }
    }

    EventLoop*  loop_;
    TcpServer   server_;
};

int main(int argc, char const *argv[])
{
    LOG_INFO << "pid = " << getpid() << ", tid = " << current_thread::tid();
    LOG_INFO << "sizeof TcpConnection = " << sizeof(TcpConnection);
    if (argc > 1)
    {
        thread_number = atoi(argv[1]);
    }
    EventLoop loop;
    InetAddress listen_addr(9600);
    EchoServer server(&loop, listen_addr, "EchoServer");
    server.start();
    loop.loop();
    return 0;
}
