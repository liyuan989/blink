#include <blink/TcpServer.h>
#include <blink/Nocopyable.h>
#include <blink/ProcessInfo.h>
#include <blink/EventLoop.h>
#include <blink/FileTool.h>
#include <blink/Atomic.h>
#include <blink/Log.h>

#include <boost/bind.hpp>

#include <unistd.h>

#include <stdio.h>

using namespace blink;

int g_numberThreads = 0;

class Echoserver : Nocopyable
{
public:
    Echoserver(EventLoop* loop, const InetAddress& server_addr)
        : server_(loop, server_addr, "Echoserver"),
          start_time_(Timestamp::now())
    {
        server_.setConnectionCallback(boost::bind(&Echoserver::onConnection, this, _1));
        server_.setMessageCallback(boost::bind(&Echoserver::onMessage, this, _1, _2, _3));
        server_.setThreadNumber(g_numberThreads);
        loop->runEvery(5.0, boost::bind(&Echoserver::printThroughput, this));
    }

    void start()
    {
        LOG_INFO << "starting " << g_numberThreads << " threads.";
        server_.start();
    }

private:
    void onConnection(const TcpConnectionPtr& connection)
    {
        LOG_INFO << "Echoserver - " << connection->peerAddress().toIpPort() << " -> "
                 << connection->localAddress().toIpPort() << " is "
                 << (connection->connected() ? "UP" : "DOWN");
        connection->setTcpNoDelay(true);
        if (connection->connected())
        {
            connections_.increment();
        }
        else
        {
            connections_.decrement();
        }
    }

    void onMessage(const TcpConnectionPtr& connection,
                   Buffer* buf,
                   Timestamp receive_time)
    {
        size_t len = buf->readableSize();
        transferred_bytes_.add(len);
        received_messages_.incrementAndGet();
        connection->send(buf);
    }

    void printThroughput()
    {
        Timestamp end_time = Timestamp::now();
        double bytes = static_cast<double>(transferred_bytes_.getAndSet(0));
        int32_t messages = received_messages_.getAndSet(0);
        double time = timeDifference(end_time, start_time_);
        printf("%.3f MiB/s %.2f Ki Msg/s %.2f bytes/msg\n",
               bytes / time / 1024 / 1024,
               messages / time / 1024,
               bytes / messages);
        printConnection();
        fflush(stdout);
        start_time_ = end_time;
    }

    void printConnection()
    {
        string procStatus = procStat();
        printf("%d connection, file %d , VmSize %ld KiB, RSS %ld KiB, \n",
               connections_.get(), openedFiles(),
               getLong(procStatus, "VmSize:"), getLong(procStatus, "VmRSS:"));
        string meminfo;
        readFile("/proc/meminfo", 65536, &meminfo);
        long total_kb = getLong(meminfo, "MemTotal:");
        long free_kb = getLong(meminfo, "MemFree:");
        long buffers_kb = getLong(meminfo, "Buffers:");
        long cached_kb = getLong(meminfo, "Cached:");
        printf("system memory used %ld KiB\n", total_kb - free_kb - buffers_kb - cached_kb);
    }

    long getLong(const string& procStatus, const char* key)
    {
        long result = 0;
        size_t pos = procStatus.find(key);
        if (pos != string::npos)
        {
            result = atoi(procStatus.c_str() + pos + strlen(key));
        }
        return result;
    }

    TcpServer    server_;
    AtomicInt32  connections_;
    AtomicInt32  received_messages_;
    AtomicInt64  transferred_bytes_;
    Timestamp    start_time_;
};

int main(int argc, char const *argv[])
{
    LOG_INFO << "pid = " << getpid() << ", tid = " << current_thread::tid()
             << ", max files = " << maxOpenFiles();
    if (argc > 1)
    {
        g_numberThreads = atoi(argv[1]);
    }
    EventLoop loop;
    Echoserver server(&loop, InetAddress(9600));
    server.start();
    loop.loop();
    return 0;
}
