#include <blink/protorpc/rpc.pb.h>
#include <blink/protorpc/RpcCodec.h>
#include <blink/EventLoopThreadPool.h>
#include <blink/ThreadLocal.h>
#include <blink/TcpServer.h>
#include <blink/TcpClient.h>
#include <blink/EventLoop.h>
#include <blink/Log.h>

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/bind.hpp>

#include <stdio.h>

using namespace blink;

class BackendSession : Nocopyable
{
public:
    BackendSession(EventLoop* loop,
                   const InetAddress& backend_addr,
                   const string& name)
        : loop_(loop),
          client_(loop, backend_addr, name),
          codec_(boost::bind(&BackendSession::onRpcMessage, this, _1, _2, _3)),
          next_id_(0)
    {
        client_.setConnectionCallback(boost::bind(&BackendSession::onConnection, this, _1));
        client_.setMessageCallback(boost::bind(&RpcCodec::onMessage, &codec_, _1, _2, _3));
        client_.enableRetry();
    }

    void connect()
    {
        client_.connect();
    }

    bool send(RpcMessage& message, const TcpConnectionPtr& client_connection)
    {
        loop_->assertInLoopThread();
        if (connection_)
        {
            uint64_t id = ++next_id_;
            Request request = {message.id(), client_connection};
            assert(outstandings_.find(id) == outstandings_.end());
            outstandings_[id] = request;
            message.set_id(id);
            codec_.send(connection_, message);
            LOG_DEBUG << "forward " << request.origin_id << " from " << client_connection->name()
                      << " as " << id << " to " << connection_->name();
            return true;
        }
        else
        {
            return true;
        }
    }

private:
    void onConnection(const TcpConnectionPtr& connection)
    {
        loop_->assertInLoopThread();
        LOG_INFO << "Backend " << connection->localAddress().toIpPort() << " -> "
                 << connection->peerAddress().toPort() << " is "
                 << (connection->connected() ? "UP" : "DOWN");
        if (connection->connected())
        {
            connection_ = connection;
        }
        else
        {
            connection_.reset();
        }
    }

    void onRpcMessage(const TcpConnectionPtr& connection,
                      const RpcMessagePtr& message,
                      Timestamp receive_time)
    {
        loop_->assertInLoopThread();
        std::map<uint64_t, Request>::iterator it = outstandings_.find(message->id());
        if (it != outstandings_.end())
        {
            uint64_t origin_id = it->second.origin_id;
            TcpConnectionPtr client_connection = it->second.client_connection.lock();
            outstandings_.erase(it);

            if (client_connection)
            {
                LOG_DEBUG << "send back " << origin_id << " of " << client_connection->name()
                          << " using " << message->id() << " from " << connection_->name();
                message->set_id(origin_id);
                codec_.send(client_connection, *message);
            }
        }
        else
        {
            // FIXME: LOG_ERROR
        }
    }

    struct Request
    {
        uint64_t                        origin_id;
        boost::weak_ptr<TcpConnection>  client_connection;
    };

    EventLoop*                   loop_;
    TcpClient                    client_;
    RpcCodec                     codec_;
    TcpConnectionPtr             connection_;
    uint64_t                     next_id_;
    std::map<uint64_t, Request>  outstandings_;
};


class Balancer : Nocopyable
{
public:
    Balancer(EventLoop* loop,
             const InetAddress& listen_addr,
             const string& name,
             const std::vector<InetAddress>& backends)
        : loop_(loop),
          server_(loop, listen_addr, name),
          codec_(boost::bind(&Balancer::onRpcMessage, this, _1, _2, _3)),
          backends_(backends)
    {
        server_.setThreadInitCallback(boost::bind(&Balancer::initPerThread, this, _1));
        server_.setConnectionCallback(boost::bind(&Balancer::onConnection, this, _1));
        server_.setMessageCallback(boost::bind(&RpcCodec::onMessage, &codec_, _1, _2, _3));
    }

    void setThreadNumber(int n)
    {
        server_.setThreadNumber(n);
    }

    void start()
    {
        server_.start();
    }

private:
    struct PerThread
    {
        size_t                             current;
        boost::ptr_vector<BackendSession>  backends;

        PerThread()
            : current(0)
        {
        }
    };

    void initPerThread(EventLoop* ioloop)
    {
        int count = thread_count_.getAndAdd(1);
        LOG_INFO << "IO thread " << count;
        PerThread& t = t_backends_.value();
        t.current = count % backends_.size();

        for (size_t i = 0; i < backends_.size(); ++i)
        {
            char buf[32];
            snprintf(buf, sizeof(buf), "%s#%d", backends_[i].toIpPort().c_str(), count);
            t.backends.push_back(new BackendSession(ioloop, backends_[i], buf));
            t.backends.back().connect();
        }
    }

    void onConnection(const TcpConnectionPtr& connection)
    {
        LOG_INFO << "Client " << connection->peerAddress().toIpPort() << " -> "
                 << connection->localAddress().toPort() << " is "
                 << (connection->connected() ? "UP" : "DOWN");
        if (!connection->connected())
        {
            // FIXME: cancel outstanding calls, otherwise, memory leak
        }
    }

    void onRpcMessage(const TcpConnectionPtr& connection,
                      const RpcMessagePtr& message,
                      Timestamp receive_time)
    {
        PerThread& t = t_backends_.value();
        bool succeed = false;
        for (size_t i = 0; i < t.backends.size() && !succeed; ++i)
        {
            succeed = t.backends[t.current].send(*message, connection);
            t.current = (t.current + 1) % t.backends.size();
        }
        if (!succeed)
        {
            // FIXME: no backend available
        }
    }

    EventLoop*                loop_;
    TcpServer                 server_;
    RpcCodec                  codec_;
    std::vector<InetAddress>  backends_;
    AtomicInt32               thread_count_;
    ThreadLocal<PerThread>    t_backends_;
};

int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s <listen port> <backend ip:port> [backend ip:port]\n", argv[0]);
        return 1;
    }
    LOG_INFO << "pid = " << getpid();
    std::vector<InetAddress> backends;
    for (int i = 2; i < argc; ++i)
    {
        string hostport = argv[i];
        size_t colon = hostport.find(':');
        if (colon != string::npos)
        {
            string ip = hostport.substr(0, colon);
            uint16_t port = static_cast<uint16_t>(atoi(hostport.c_str() + colon + 1));
            backends.push_back(InetAddress(ip, port));
        }
        else
        {
            fprintf(stderr, "Invalid backend address %s\n", argv[i]);
            return 1;
        }
    }

    uint16_t port = static_cast<uint16_t>(atoi(argv[1]));
    InetAddress listen_addr(port);
    EventLoop loop;
    Balancer balancer(&loop, listen_addr, "RpcBalancer", backends);
    balancer.setThreadNumber(4);
    balancer.start();
    loop.loop();
    google::protobuf::ShutdownProtobufLibrary();
    return 0;
}
