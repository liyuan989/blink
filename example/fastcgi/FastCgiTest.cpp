#include <example/fastcgi/FastCgi.h>

#include <blink/TcpServer.h>
#include <blink/EventLoop.h>
#include <blink/Log.h>

#include <boost/bind.hpp>

using namespace blink;

void onRequest(const TcpConnectionPtr& connection,
               FastCgiCodec::ParamMap& params,
               Buffer* in)
{
    LOG_INFO << connection->name() << ": " << params["REQUEST_URI"];
    for (FastCgiCodec::ParamMap::iterator it = params.begin();
         it != params.end(); ++it)
    {
        LOG_DEBUG << it->first << " = " << it->second;
    }
    Buffer response;
    response.append("Content-Type: text/plain\r\n\r\n");
    response.append("Hello FastCgi");
    FastCgiCodec::respond(&response);
    connection->send(&response);
}

void onConnection(const TcpConnectionPtr& connection)
{
    if (connection->connected())
    {
        boost::shared_ptr<FastCgiCodec> codec(new FastCgiCodec(onRequest));
        connection->setContext(codec);
        connection->setMessageCallback(boost::bind(&FastCgiCodec::onMessage, codec, _1, _2, _3));
    }
};

int main(int argc, char const *argv[])
{
    EventLoop loop;
    TcpServer server(&loop, InetAddress(9600), "FastCgi");
    server.setConnectionCallback(onConnection);
    server.start();
    loop.loop();
    return 0;
}
