#include <example/protobuf/rpcbench/echo.pb.h>

#include <blink/protorpc/RpcServer.h>
#include <blink/EventLoop.h>
#include <blink/Log.h>

using namespace blink;

namespace echo
{

class EchoServiceImpl : public echo::EchoService
{
public:
    void echo(::google::protobuf::RpcController* controller,
              const ::echo::EchoRequest* request,
              ::echo::EchoResponse* response,
              ::google::protobuf::Closure* done)
    {
        response->set_payload(request->payload());
        done->Run();
    }
};

}  // namespace echo

int main(int argc, char* argv[])
{
    int n_threads = 1;
    if (argc == 2)
    {
        n_threads = atoi(argv[1]);
    }
    LOG_INFO << "pid = " << getpid() << " threads = " << n_threads;
    EventLoop loop;
    echo::EchoServiceImpl impl;
    RpcServer server(&loop, InetAddress(9600), "RpcServer");
    server.setThreadNumer(n_threads);
    server.registerService(&impl);
    server.start();
    loop.loop();
    exit(0);
}
