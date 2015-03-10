#include "CountDownLatch.h"
#include "EventLoop.h"
#include "EventLoopThreadPool.h"
#include "Log.h"
#include "TcpClient.h"

#include <boost/bind.hpp>
#include <boost/program_options.hpp>

#include <iostream>
#include <stdio.h>

namespace po = boost::program_options;
using namespace blink;

class Client
{
public:
    enum Operation
    {
        kGet,
        kSet,
    };

    Client(const string& name,
           EventLoop*loop,
           const InetAddress& server_addr,
           Operation operation,
           int requests,
           int keys,
           int value_len,
           CountDownLatch* connected,
           CountDownLatch* finished)
        : name_(name),
          client_(loop, server_addr, name),
          operation_(operation),
          sent_(0),
          acked_(0),
          requests_(requests),
          keys_(keys),
          value_len_(value_len),
          value_(value_len, 'a'),
          connected_(connected),
          finished_(finished)
    {
        value_ += "\r\n";
        client_.setConnectionCallback(boost::bind(&Client::onConnection, this, _1));
        client_.setMessageCallback(boost::bind(&Client::onMessage, this, _1, _2, _3));
        client_.connect();
    }

    void send()
    {
        Buffer buf;
        fill(&buf);
        connection_->send(&buf);
    }

private:
    void onConnection(const TcpConnectionPtr& connection)
    {
        if (connection->connected())
        {
            connection_ = connection;
            connected_->countDown();
        }
        else
        {
            connection_.reset();
            client_.getLoop()->queueInLoop(boost::bind(&CountDownLatch::countDown, finished_));
        }
    }

    void onMessage(const TcpConnectionPtr& connection,
                   Buffer* buf,
                   Timestamp receive_time)
    {
        if (operation_ == kSet)
        {
            while (buf->readableSize() > 0)
            {
                const char* crlf = buf->findCRLF();
                if (crlf)
                {
                    buf->resetUntil(crlf + 2);
                    ++acked_;
                    if (sent_ < requests_)
                    {
                        send();
                    }
                }
                else
                {
                    break;
                }
            }
        }
        else
        {
            while (buf->readableSize() > 0)
            {
                const char* end = static_cast<const char*>(
                    memmem(buf->peek(), buf->readableSize(), "END\r\n", 5));
                if (end)
                {
                    buf->resetUntil(end + 5);
                    ++acked_;
                    if (sent_ < requests_)
                    {
                        send();
                    }
                }
                else
                {
                    break;
                }
            }
        }
        if (acked_ == requests_)
        {
            connection_->shutdown();
        }
    }

    void fill(Buffer* buf)
    {
        char request[256];
        if (operation_ == kSet)
        {
            snprintf(request, sizeof(request), "set %s%d 42 0 %d\r\n",
                     name_.c_str(), sent_ % keys_, value_len_);
            ++sent_;
            buf->append(request);
            buf->append(value_);
        }
        else
        {
            snprintf(request, sizeof(request), "get %s%d\r\n", name_.c_str(), sent_ % keys_);
            ++sent_;
            buf->append(request);
        }
    }

    string                 name_;
    TcpClient              client_;
    TcpConnectionPtr       connection_;
    const Operation        operation_;
    int                    sent_;
    int                    acked_;
    const int              requests_;
    const int              keys_;
    const int              value_len_;
    string                 value_;
    CountDownLatch* const  connected_;
    CountDownLatch* const  finished_;
};

int main(int argc, char* argv[])
{
    Log::setLogLevel(Log::WARN);
    uint16_t tcpport = 11211;
    string host_ip = "127.0.0.1";
    int threads = 4;
    int clients = 100;
    int requests = 100000;
    int keys = 10000;
    bool set = false;

    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "Help")
        ("port,p", po::value<uint16_t>(&tcpport), "TCP port")
        ("ip,i", po::value<string>(&host_ip), "Host IP")
        ("threads,t", po::value<int>(&threads), "Number of worker threads")
        ("client,c", po::value<int>(&clients), "Number of concurent clients")
        ("requests,r", po::value<int>(&requests), "Number of requests per client")
        ("keys,k", po::value<int>(&keys), "Number of keys per client")
        ("set,s", "Get or Set")
        ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help"))
    {
        std::cout << desc << "\n";
        return 0;
    }
    set = vm.count("set");

    InetAddress server_addr(host_ip, tcpport);
    LOG_WARN << "Connecting " << server_addr.toIpPort();

    EventLoop loop;
    EventLoopThreadPool pool(&loop);

    int value_len = 100;
    Client::Operation operation = set ? Client::kGet : Client::kGet;

    double memoryMiB = 1.0 * clients * keys * (32 + 80 + value_len + 8) / 1024 / 1024;
    LOG_WARN << "estimated memcached_server memory usage "
             << static_cast<int>(memoryMiB) << " MiB";

    pool.setThreadNumber(threads);
    pool.start();

    char buf[32];
    CountDownLatch connected(clients);
    CountDownLatch finished(clients);
    boost::ptr_vector<Client> holder;
    for (int i = 0; i < clients; ++i)
    {
        snprintf(buf, sizeof(buf), "%-d", i + 1);
        holder.push_back(new Client(buf, pool.getNextLoop(), server_addr, operation,
                         requests, keys, value_len, &connected, &finished));
    }
    connected.wait();
    LOG_WARN << clients << " clients all connected";
    Timestamp start = Timestamp::now();
    for (int i = 0; i < clients; ++i)
    {
        holder[i].send();
    }
    finished.wait();
    Timestamp end = Timestamp::now();
    LOG_WARN << "all finished";
    double seconds = timeDifference(end, start);
    LOG_WARN << seconds << " seconds";
    LOG_WARN << 1.0 * clients * requests / seconds << " requests per seconds";
    return 0;
}
