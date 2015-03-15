#include <example/protobuf/rpc/sudoku.pb.h>

#include <blink/protorpc/RpcServer.h>
#include <blink/EventLoop.h>
#include <blink/Log.h>

#include <boost/bind.hpp>

using namespace blink;

namespace sudoku
{

class SudokuServiceImpl : public SudokuService
{
public:
    virtual void solve(::google::protobuf::RpcController* controller,
                       const ::sudoku::SudokuRequest* request,
                       ::sudoku::SudokuResponse* response,
                       ::google::protobuf::Closure* done)
    {
        LOG_INFO << "SudokuServiceImpl::solve";
        response->set_solved(true);
        response->set_checkerboard("1234567");
        done->Run();
    }
};

}  // namespace sudoku

int main(int argc, char* argv[])
{
    LOG_INFO << "pid = " << tid();
    EventLoop loop;
    InetAddress listen_addr(9600);
    sudoku::SudokuServiceImpl impl;
    RpcServer server(&loop, listen_addr, "RpcServer");
    server.registerService(&impl);
    server.start();
    loop.loop();
    google::protobuf::ShutdownProtobufLibrary();
    return 0;
}
