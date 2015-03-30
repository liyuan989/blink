#include <blink/EventLoop.h>
#include <blink/TcpServer.h>
#include <blink/Log.h>

#include <boost/shared_ptr.hpp>

#include <unistd.h>

#include <stdio.h>

using namespace blink;

typedef boost::shared_ptr<FILE> FilePtr;

const int kBuffSize = 64 * 1024;
const char* g_filename = NULL;

void onHighWaterMark(const TcpConnectionPtr& connection, size_t len)
{
    LOG_INFO << "HighWaterMark " << len;
}

void onConnection(const TcpConnectionPtr& connection)
{
    LOG_INFO << "FileServer - " << connection->peerAddress().toIpPort() << " -> "
             << connection->localAddress().toIpPort() << " is "
             << (connection->connected() ? "UP" : "DOWN");
    if (connection->connected())
    {
        LOG_INFO << "FileServer - Sending file " << g_filename
                 << " to " << connection->peerAddress().toIpPort();
        connection->setHighWaterMarkCallback(onHighWaterMark, kBuffSize + 1);
        FILE* fp = fopen(g_filename, "rb");
        if (fp)
        {
            FilePtr fptr(fp, fclose);
            connection->setContext(fptr);
            char buf[kBuffSize];
            size_t nread = fread(buf, 1, sizeof(buf), fp);
            connection->send(buf, static_cast<int>(nread));
        }
        else
        {
            connection->shutdown();
            LOG_INFO << "FileServer - no such file";
        }
    }
}

void onWriteComplete(const TcpConnectionPtr& connection)
{
    const FilePtr& fptr = boost::any_cast<FilePtr>(connection->getContext());
    char buf[kBuffSize];
    size_t nread = fread(buf, 1, sizeof(buf), boost::get_pointer(fptr));
    if (nread > 0)
    {
        connection->send(buf, static_cast<int>(nread));
    }
    else
    {
        connection->shutdown();
        LOG_INFO << "FileServer - done";
    }
}

int main(int argc, char const *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return 1;
    }
    LOG_INFO << "pid = " << getpid() << ", tid = " << tid();
    g_filename = argv[1];
    EventLoop loop;
    TcpServer server(&loop, InetAddress(9600), "FileServer");
    server.setConnectionCallback(onConnection);
    server.setWriteCompleteCallback(onWriteComplete);
    server.start();
    loop.loop();
    return 0;
}
