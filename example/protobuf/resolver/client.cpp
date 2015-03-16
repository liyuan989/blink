#include <example/protobuf/resolver/resolver.pb.h>

#include <blink/protorpc/RpcChannel.h>
#include <blink/TcpConnection.h>
#include <blink/TcpClient.h>
#include <blink/EventLoop.h>
#include <blink/InetAddress.h>
#include <blink/SocketBase.h>
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
          channel_(new RpcChannel),
          got_(0),
          total_(0),
          stub_(channel_.get())
    {
        client_.setConnectionCallback(boost::bind(&RpcClient::onConnection, this, _1));
        client_.setMessageCallback(boost::bind(&RpcChannel::onMessage, channel_.get(), _1, _2, _3));
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
            total_ = 4;
            resolve("github.com");
            resolve("google.com");
            resolve("liyuanlife.com");
            resolve("stackoverflow.com");
        }
        else
        {
            loop_->quit();
        }
    }

    void resolve(const std::string& host)
    {
        resolver::ResolveRequest request;
        request.set_adress(host);
        resolver::ResolveResponse* response = new resolver::ResolveResponse;
        stub_.solve(NULL, &request, response, NewCallback(this, &RpcClient::resolved, response, host));
    }

    void resolved(resolver::ResolveResponse* respone, std::string host)
    {
        if (respone->resolved())
        {
            char buf[32];
            uint32_t ip = respone->ip(0);
            struct sockaddr_in addr;
            addr.sin_addr.s_addr = ip;
            blink::sockets::toIp(buf, sizeof(buf), addr);
            LOG_INFO << "resolved " << host << ": " << buf << "\n"
                     << respone->DebugString();
        }
        else
        {
            LOG_INFO << "resolved " << host << " failed";
        }
        if (++got_ >= total_)
        {
            client_.disconnect();
        }
    }

    EventLoop*                       loop_;
    TcpClient                        client_;
    RpcChannelPtr                    channel_;
    int                              got_;
    int                              total_;
    resolver::ResolverService::Stub  stub_;
};

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <ipaddr> <port>\n", argv[0]);
        return 1;
    }
    LOG_INFO << "pid = " << getpid();
    EventLoop loop;
    InetAddress server_addr(argv[1], static_cast<uint16_t>(atoi(argv[2])));
    RpcClient client(&loop, server_addr);
    client.connect();
    loop.loop();
    return 0;
}
