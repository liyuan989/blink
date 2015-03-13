#include <blink/protorpc/RpcServer.h>
#include <blink/protorpc/RpcChannel.h>
#include <blink/Log.h>

#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>

#include <boost/bind.hpp>

namespace blink
{

RpcServer::RpcServer(EventLoop* loop,
                     const InetAddress& listen_addr,
                     const string& name_arg)
    : server_(loop, listen_addr, name_arg)
{
    server_.setConnectionCallback(boost::bind(&RpcServer::onConnection, this, _1));
    //server_.setMessageCallback(boost::bind(&RpcServer::onMessage, this, _1, _2, _3));
}

void RpcServer::registerService(::google::protobuf::Service* service)
{
    const google::protobuf::ServiceDescriptor* desc = service->GetDescriptor();
    services_[desc->name()] = service;
}

void RpcServer::start()
{
    server_.start();
}

void RpcServer::onConnection(const TcpConnectionPtr& connection)
{
    LOG_INFO << "RpcServer - " << connection->peerAddress().toIpPort() << " -> "
             << connection->localAddress().toIpPort() << " is "
             << (connection->connected() ? "UP" : "DOWN");
    if (connection->connected())
    {
        RpcChannelPtr channel(new RpcChannel(connection));
        channel->setServices(&services_);
        connection->setMessageCallback(boost::bind(&RpcChannel::onMessage, channel.get(), _1, _2, _3));
        connection->setContext(channel);
    }
    else
    {
        connection->setContext(RpcChannelPtr());
        // FIXME:
    }
}

// void RpcServer::onMessage(const TcpConnectionPtr& connection,
//                           Buffer* buf,
//                           Timestamp receive_time)
// {
//     RpcChannelPtr& channel = boost::any_cast<RpcChannelPtr&>(connection->getContext());
//     channel->onMessage(connection, buf, receive_time);
// }

}  // namespace blink
