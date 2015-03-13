#ifndef __BLINK_PROTORPC_RPCSERVER_H__
#define __BLINK_PROTORPC_RPCSERVER_H__

#include <blink/Nocopyable.h>
#include <blink/TcpServer.h>

namespace google
{

namespace protobuf
{

class Service;

}  // namespace protobuf

}  // namespace google

namespace blink
{

class RpcServer : Nocopyable
{
public:
    RpcServer(EventLoop* loop,
              const InetAddress& listen_addr,
              const string& name_arg);

    void setThreadNumer(int n)
    {
        server_.setThreadNumber(n);
    }

    const string& name() const
    {
        return server_.name();
    }

    void registerService(::google::protobuf::Service* service);
    void start();

private:
    void onConnection(const TcpConnectionPtr& connection);

    // void onMessage(const TcpConnectionPtr& connection,
    //                Buffer* buf,
    //                Timestamp receive_time);

    TcpServer                                            server_;
    std::map<std::string, ::google::protobuf::Service*>  services_;
};

}  // namespace blink

#endif
