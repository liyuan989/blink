#ifndef __EXAMPLE_SOCK4A_TUNNEL_H__
#define __EXAMPLE_SOCK4A_TUNNEL_H__

#include <blink/InetAddress.h>
#include <blink/EventLoop.h>
#include <blink/TcpServer.h>
#include <blink/TcpClient.h>
#include <blink/Log.h>

#include <boost/enable_shared_from_this.hpp>
#include <boost/bind.hpp>

class Tunnel : blink::Nocopyable,
               public boost::enable_shared_from_this<Tunnel>
{
public:
    Tunnel(blink::EventLoop* loop,
           const blink::InetAddress& server_addr,
           const blink::TcpConnectionPtr& server_connection)
        : client_(loop, server_addr, server_connection->name()),
          server_connection_(server_connection)
    {
        LOG_INFO << "Tunnel " << server_connection->peerAddress().toIpPort()
                 << " <-> " << server_addr.toIpPort();
    }

    ~Tunnel()
    {
        LOG_INFO << "~Tunnel()";
    }

    void setup()
    {
        client_.setConnectionCallback(
            boost::bind(&Tunnel::onClientConnection, shared_from_this(), _1));
        client_.setMessageCallback(
            boost::bind(&Tunnel::onClientMessage, shared_from_this(), _1, _2, _3));
        server_connection_->setHighWaterMarkCallback(
            boost::bind(Tunnel::onHighWaterMarkWeak, boost::weak_ptr<Tunnel>(shared_from_this()), _1, _2),
            10 * 1024 * 1024);
    }

    void teardown()
    {
        client_.setConnectionCallback(blink::defaultConnectionCallback);
        client_.setMessageCallback(blink::defaultMessageCallback);
        if (server_connection_)
        {
            server_connection_->setContext(boost::any());
            server_connection_->shutdown();
        }
    }

    void connect()
    {
        client_.connect();
    }

    void disconnect()
    {
        client_.disconnect();
    }

    void onClientConnection(const blink::TcpConnectionPtr& connection)
    {
        LOG_DEBUG << (connection->connected() ? "UP" : "DOWN");
        if (connection->connected())
        {
            connection->setTcpNoDelay(true);
            connection->setHighWaterMarkCallback(
                boost::bind(Tunnel::onHighWaterMarkWeak, boost::weak_ptr<Tunnel>(shared_from_this()), _1, _2),
                10 * 1024 * 1024);
            server_connection_->setContext(connection);
            if (server_connection_->inputBuffer()->readableSize() > 0)
            {
                connection->send(server_connection_->inputBuffer());
            }
        }
        else
        {
            teardown();
        }
    }

    void onClientMessage(const blink::TcpConnectionPtr& connection,
                         blink::Buffer* buf,
                         const blink::Timestamp receive_time)
    {
        LOG_DEBUG << connection->name() << " " << buf->readableSize();
        if (server_connection_)
        {
            server_connection_->send(buf);
        }
        else
        {
            buf->resetAll();
            abort();
        }
    }

    void onHighWaterMark(const blink::TcpConnectionPtr& connection,
                         size_t bytes_to_sent)
    {
        LOG_INFO << "onHighWaterMark " << connection->name()
                 << " bytes " << bytes_to_sent;
        disconnect();
    }

    static void onHighWaterMarkWeak(const boost::weak_ptr<Tunnel>& weak_tunnel,
                                    const blink::TcpConnectionPtr& connection,
                                    size_t bytes_to_sent)
    {
        boost::shared_ptr<Tunnel> tunnel = weak_tunnel.lock();
        if (tunnel)
        {
            tunnel->onHighWaterMark(connection, bytes_to_sent);
        }
    }

private:
    blink::TcpClient         client_;
    blink::TcpConnectionPtr  server_connection_;
};

typedef boost::shared_ptr<Tunnel> TunnelPtr;

#endif
