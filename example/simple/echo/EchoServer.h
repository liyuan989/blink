#ifndef __EXAMPLE_ECHOSERVER_H__
#define __EXAMPLE_ECHOSERVER_H__

#include <blink/TcpServer.h>
#include <blink/Nocopyable.h>
#include <blink/EventLoop.h>
#include <blink/InetAddress.h>

class EchoServer : blink::Nocopyable
{
public:
    EchoServer(blink::EventLoop* loop,
               const blink::InetAddress& listen_addr,
               const blink::string& name);

    void start();

private:
    void onConnection(const blink::TcpConnectionPtr& connection);

    void onMessage(const blink::TcpConnectionPtr& connection,
                   blink::Buffer* buf,
                   blink::Timestamp time);

    blink::EventLoop*  loop_;
    blink::TcpServer   server_;
};

#endif
