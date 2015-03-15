#include <example/protobuf/rpc/sudoku.pb.h>

#include <blink/protorpc/RpcChannel.h>
#include <blink/TcpConnection.h>
#include <blink/TcpClient.h>
#include <blink/EventLoop.h>
#include <blink/InetAddress.h>
#include <blink/Log.h>

#include <boost/bind.hpp>

#include <stdio.h>

using namespace blink;

class RpcClient : Nocopyable
{
public:
    RpcClient(EventLoop* loop, const InetAddress& server_addr)
        : loop_(loop),
          client_(loop, server_addr, "RpcClient"),
          channel_(new RpcChannel()),
          stub_(channel_.get())
    {
        client_.setConnectionCallback(boost::bind(&RpcClient::onConnection, this, _1));
        client_.setMessageCallback(boost::bind(&RpcChannel::onMessage, channel_.get(), _1, _2, _3 ));
        //client_.enableRetry();
    }

    void connect()
    {
        client_.connect();
    }

private:
    void onConnection(const TcpConnectionPtr& connection)
    {
        if (connection->connected())
        {
            channel_->setConnection(connection);
            sudoku::SudokuRequest request;
            request.set_checkerboard("001010");
            sudoku::SudokuResponse* response = new sudoku::SudokuResponse;
            stub_.solve(NULL, &request, response, NewCallback(this, &RpcClient::solved, response));
        }
        else
        {
            loop_->quit();
        }
    }

    void solved(sudoku::SudokuResponse* response)
    {
        LOG_INFO << "solved:\n" << response->DebugString().c_str();
        client_.disconnect();
    }

    EventLoop*                   loop_;
    TcpClient                    client_;
    RpcChannelPtr                channel_;
    sudoku::SudokuService::Stub  stub_;
};

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <ipaddr> <port>\n", argv[0]);
        return 1;
    }
    LOG_INFO << "pid = " << tid();
    EventLoop loop;
    InetAddress server_addr(argv[1], static_cast<uint16_t>(atoi(argv[2])));
    RpcClient client(&loop, server_addr);
    client.connect();
    loop.loop();
    google::protobuf::ShutdownProtobufLibrary();
    return 0;
}
