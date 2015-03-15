#include <example/protobuf/rpcbench/echo.pb.h>

#include <blink/protorpc/RpcChannel.h>
#include <blink/EventLoopThreadPool.h>
#include <blink/CountDownLatch.h>
#include <blink/TcpConnection.h>
#include <blink/InetAddress.h>
#include <blink/TcpClient.h>
#include <blink/EventLoop.h>
#include <blink/Log.h>

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/bind.hpp>

#include <stdio.h>

using namespace blink;

const int kRequests = 50000;

class RpcClient : Nocopyable
{
public:
    RpcClient(EventLoop* loop,
              const InetAddress& server_addr,
              CountDownLatch* all_connected,
              CountDownLatch* all_finished)
        : client_(loop, server_addr, "RpcClient"),
          channel_(new RpcChannel),
          stub_(channel_.get()),
          all_connected_(all_connected),
          all_finished_(all_finished),
          count_(0)
    {
        client_.setConnectionCallback(boost::bind(&RpcClient::onConnection, this, _1));
        client_.setMessageCallback(boost::bind(&RpcChannel::onMessage, channel_.get(), _1, _2, _3));
    }

    void connect()
    {
        client_.connect();
    }

    void sendRequest()
    {
        echo::EchoRequest request;
        request.set_payload("001010");
        echo::EchoResponse* response = new echo::EchoResponse;
        stub_.echo(NULL, &request, response, NewCallback(this, &RpcClient::replied, response));
    }

private:
    void onConnection(const TcpConnectionPtr& connection)
    {
        if (connection->connected())
        {
            connection->setTcpNoDelay(true);
            channel_->setConnection(connection);
            all_connected_->countDown();
        }
    }

    void replied(echo::EchoResponse* response)
    {
        ++count_;
        if (count_ < kRequests)
        {
            sendRequest();
        }
        else
        {
            LOG_INFO << "RpcClient " << this << " finished";
            all_finished_->countDown();
        }
    }

    TcpClient                client_;
    RpcChannelPtr            channel_;
    echo::EchoService::Stub  stub_;
    CountDownLatch*          all_connected_;
    CountDownLatch*          all_finished_;
    int                      count_;
};


int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s <ipaddr> <port> [n_clients]\n", argv[0]);
        return 1;
    }
    int n_threads = 1;
    int n_clients = 1;
    if (argc >= 4)
    {
        n_threads = atoi(argv[3]);
    }
    if (argc == 5)
    {
        n_clients = atoi(argv[4]);
    }

    LOG_INFO << "pid = " << tid() << " threads = " << n_threads
             << " clients = " << n_clients;
    CountDownLatch all_connected(n_clients);
    CountDownLatch all_finished(n_clients);
    InetAddress server_addr(argv[1], static_cast<uint16_t>(atoi(argv[2])));
    EventLoop loop;
    EventLoopThreadPool pool(&loop, "rpcbench-client");
    pool.setThreadNumber(n_threads);
    pool.start();
    boost::ptr_vector<RpcClient> clients;

    for (int i = 0; i < n_clients; ++i)
    {
        clients.push_back(new RpcClient(pool.getNextLoop(), server_addr, &all_connected, &all_finished));
        clients.back().connect();
    }
    all_connected.wait();
    Timestamp start(Timestamp::now());
    LOG_INFO << "all connected";

    for (int i = 0; i < n_clients; ++i)
    {
        clients[i].sendRequest();
    }
    all_finished.wait();
    Timestamp end(Timestamp::now());
    LOG_INFO << "all finished";
    double seconds = timeDifference(end, start);
    printf("%f seconds\n", seconds);
    printf("%.1f calls per second\n", n_clients * kRequests / seconds);

    exit(0);
}
