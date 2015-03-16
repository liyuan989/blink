#include <example/protobuf/resolver/resolver.pb.h>

#include <blink/protorpc/RpcServer.h>
#include <blink/SocketBase.h>
#include <blink/EventLoop.h>
#include <blink/Log.h>

#include <boost/bind.hpp>

using namespace blink;

namespace resolver
{

class ResolverServiceImpl : public resolver::ResolverService
{
public:
    void solve(::google::protobuf::RpcController* controller,
               const resolver::ResolveRequest* request,
               resolver::ResolveResponse* response,
               ::google::protobuf::Closure* done)
    {
        response->set_resolved(true);
        struct sockaddr_in addr;
        sockets::fromIpPort("10.33.1.48", 9600, &addr);
        response->add_ip(addr.sin_addr.s_addr);
        response->add_port(9600);
        done->Run();
    }
};

}  // namespace resolver

int main(int argc, char* argv[])
{
    LOG_INFO << "pid = " << getpid();
    EventLoop loop;
    resolver::ResolverServiceImpl impl;
    RpcServer server(&loop, InetAddress(9600), "RpcServer");
    server.registerService(&impl);
    server.start();
    loop.loop();
    return 0;
}
