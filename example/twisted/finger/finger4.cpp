#include <blink/EventLoop.h>
#include <blink/TcpServer.h>

using namespace blink;

void onMessage(const TcpConnectionPtr& connection,
               Buffer* buf,
               Timestamp receive_time)
{
    if (buf->findCRLF())
    {
        connection->shutdown();
    }
}

int main(int argc, char const *argv[])
{
    EventLoop loop;
    TcpServer server(&loop, InetAddress(9600), "FingerServer");
    server.setMessageCallback(onMessage);
    server.start();
    loop.loop();
    return 0;
}
