#include "TcpServer.h"
#include "TcpClient.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "MutexLock.h"
#include "Nocopyable.h"
#include "Atomic.h"
#include "Thread.h"
#include "Log.h"

#include <boost/bind.hpp>

#include <unistd.h>

#include <queue>
#include <map>
#include <vector>
#include <stdio.h>

using namespace blink;

const int kMaxConnections = 10;  // max id = 65535
const size_t kMaxPacketLen = 255;
const size_t kHeaderLen = 3;
const uint16_t kClientPort = 9600;
const uint16_t kBackendPort = 9700;
const char* kBackendIp = "127.0.0.1";

class MultiplexServer :Nocopyable
{
public:
    MultiplexServer(EventLoop* loop,
                    const InetAddress& listen_addr,
                    const InetAddress& backend_addr,
                    int number_threads);

    void start();

private:
    void onClientConnection(const TcpConnectionPtr& connection);
    void onClientMessage(const TcpConnectionPtr& connection,
                         Buffer* buf,
                         Timestamp receive_time);
    void sendBackendString(int id, const std::string& message);
    void sendBackendBuffer(int id, Buffer* buf);
    void sendBackendPacket(int id, Buffer* buf);
    void onBackendConnection(const TcpConnectionPtr& connection);
    void onBackendMessage(const TcpConnectionPtr& connection,
                          Buffer* buf,
                          Timestamp receive_time);
    void sendToClient(Buffer* buf);
    void printStatistics();

    TcpServer                        server_;
    TcpClient                        backend_;
    int                              number_threads_;
    AtomicInt64                      transferred_;
    AtomicInt64                      received_messages_;
    int64_t                          old_counter_;
    Timestamp                        start_time_;
    MutexLock                        mutex_;
    TcpConnectionPtr                 backend_connection_;
    std::map<int, TcpConnectionPtr>  client_connections_;
    std::queue<int>                  available_ids_;
};

MultiplexServer::MultiplexServer(EventLoop* loop,
                                 const InetAddress& listen_addr,
                                 const InetAddress& backend_addr,
                                 int number_threads)
    : server_(loop, listen_addr, "MultiplexServer"),
      backend_(loop, backend_addr, "MultiplexBackend"),
      number_threads_(number_threads),
      old_counter_(0),
      start_time_(Timestamp::now())
{
    server_.setConnectionCallback(boost::bind(&MultiplexServer::onClientConnection, this, _1));
    server_.setMessageCallback(boost::bind(&MultiplexServer::onClientMessage, this, _1, _2, _3));
    server_.setThreadNumber(number_threads);
    backend_.setConnectionCallback(boost::bind(&MultiplexServer::onBackendConnection, this, _1));
    backend_.setMessageCallback(boost::bind(&MultiplexServer::onBackendMessage, this, _1, _2, _3));
    backend_.enableRetry();
    //loop->runEvery(10.0, boost::bind(&MultiplexServer::printStatistics, this));
}

void MultiplexServer::start()
{
    LOG_INFO << "starting " << number_threads_ << " threads.";
    backend_.connect();
    server_.start();
}

void MultiplexServer::onClientConnection(const TcpConnectionPtr& connection)
{
    LOG_TRACE << "Client " << connection->peerAddress().toIpPort() << " -> "
              << connection->localAddress().toIpPort() << " is "
              << (connection->connected() ? "UP" : "DOWN");
    if (connection->connected())
    {
        int id = -1;
        {
            MutexLockGuard guard(mutex_);
            if (!available_ids_.empty())
            {
                id = available_ids_.front();
                available_ids_.pop();
                client_connections_[id] = connection;
            }
        }
        if (id <= 0)
        {
            connection->shutdown();  // no client id available.
        }
        else
        {
            connection->setContext(id);
            char buf[256];
            snprintf(buf, sizeof(buf), "CONNECTION %d FROM %s IS UP\r\n",
                     id, connection->peerAddress().toIpPort().c_str());
            sendBackendString(0, buf);
        }
    }
    else
    {
        if (!connection->getContext().empty())
        {
            int id = boost::any_cast<int>(connection->getContext());
            assert(id > 0 && id <= kMaxConnections);
            char buf[256];
            snprintf(buf, sizeof(buf), "CONNECTION %d FROM %s IS DOWN\r\n",
                     id, connection->peerAddress().toIpPort().c_str());
            sendBackendString(0, buf);

            MutexLockGuard guard(mutex_);
            if (backend_connection_)
            {
                available_ids_.push(id);
                client_connections_.erase(id);
            }
            else
            {
                assert(available_ids_.empty());
                assert(client_connections_.empty());
            }
        }
    }
}

void MultiplexServer::onClientMessage(const TcpConnectionPtr& connection,
                                                Buffer* buf,
                                                Timestamp receive_time)
{
    size_t len = buf->readableSize();
    transferred_.getAndSet(len);
    received_messages_.incrementAndGet();
    if (!connection->getContext().empty())
    {
        int id = boost::any_cast<int>(connection->getContext());
        sendBackendBuffer(id, buf);
    }
    else
    {
        buf->resetAll();  // FIXME: error handing.
    }
}

void MultiplexServer::sendBackendString(int id, const std::string& message)
{
    assert(message.size() <= kMaxPacketLen);
    Buffer buf;
    buf.append(message);
    sendBackendPacket(id, &buf);
}

void MultiplexServer::sendBackendBuffer(int id, Buffer* buf)
{
    while (buf->readableSize() > kMaxPacketLen)
    {
        Buffer packet;
        packet.append(buf->peek(), kMaxPacketLen);
        buf->reset(kMaxPacketLen);
        sendBackendPacket(id, &packet);
    }
    if (buf->readableSize() > 0)
    {
        sendBackendPacket(id, buf);
    }
}

void MultiplexServer::sendBackendPacket(int id, Buffer* buf)
{
    size_t len = buf->readableSize();
    LOG_DEBUG << "sendBackendPacket " << len;
    assert(len <= kMaxPacketLen);
    uint8_t header[kHeaderLen] = {static_cast<uint8_t>(len),
                                  static_cast<uint8_t>(id & 0xFF),
                                  static_cast<uint8_t>((id & 0xFF00) >> 8)};
    buf->prepend(header, kHeaderLen);
    TcpConnectionPtr backend_connection;
    {
        MutexLockGuard guard(mutex_);
        backend_connection = backend_connection_;
    }
    if (backend_connection)
    {
        backend_connection->send(buf);
    }
}

void MultiplexServer::onBackendConnection(const TcpConnectionPtr& connection)
{
    LOG_TRACE << "Backend " << connection->localAddress().toIpPort() << " -> "
              << connection->peerAddress().toIpPort() << " is "
              << (connection->connected() ? "UP" : "DOWN");
    std::vector<TcpConnectionPtr> connections_to_detroy;
    if (connection->connected())
    {
        MutexLockGuard guard(mutex_);
        backend_connection_ = connection;
        assert(available_ids_.empty());
        for (int i = 1; i <= kMaxConnections; ++i)
        {
            available_ids_.push(i);
        }
    }
    else
    {
        MutexLockGuard guard(mutex_);
        backend_connection_.reset();
        connections_to_detroy.reserve(client_connections_.size());
        for (std::map<int, TcpConnectionPtr>::iterator it = client_connections_.begin();
             it != client_connections_.end(); ++it)
        {
            connections_to_detroy.push_back(it->second);
        }
        client_connections_.clear();
        while (!available_ids_.empty())
        {
            available_ids_.pop();
        }
    }
    for (std::vector<TcpConnectionPtr>::iterator it = connections_to_detroy.begin();
         it != connections_to_detroy.end(); ++it)
    {
        (*it)->shutdown();
    }
}

void MultiplexServer::onBackendMessage(const TcpConnectionPtr& connection,
                                                 Buffer* buf,
                                                 Timestamp receive_time)
{
    size_t len = buf->readableSize();
    transferred_.addAndGet(len);
    received_messages_.incrementAndGet();
    sendToClient(buf);
}

void MultiplexServer::sendToClient(Buffer* buf)
{
    while (buf->readableSize() > kHeaderLen)
    {
        int len = static_cast<uint8_t>(*buf->peek());
        if (buf->readableSize() < len + kHeaderLen)
        {
            break;
        }
        else
        {
            int id = static_cast<uint8_t>(buf->peek()[1]);
            id |= (static_cast<uint8_t>(buf->peek()[2]) << 8);
            TcpConnectionPtr client_connection;
            {
                MutexLockGuard guard(mutex_);
                std::map<int, TcpConnectionPtr>::iterator it = client_connections_.find(id);
                if (it != client_connections_.end())
                {
                    client_connection = it->second;
                }
            }
            if (client_connection)
            {
                client_connection->send(buf->peek() + kHeaderLen, len);
            }
            buf->reset(kHeaderLen + len);
        }
    }
}

void MultiplexServer::printStatistics()
{
    Timestamp end_time = Timestamp::now();
    int64_t new_counter = transferred_.get();
    int64_t bytes = new_counter - old_counter_;
    int64_t messages = received_messages_.getAndSet(0);
    double time = timeDifference(end_time, start_time_);
    printf("%4.3f MiB/s %4.3f Ki Msg/s %6.2f bytes/msg\n",
           static_cast<double>(bytes) / time / 1024 / 1024,
           static_cast<double>(messages) / time / 1024,
           static_cast<double>(bytes) / static_cast<double>(messages));
    old_counter_ = new_counter;
    start_time_ = end_time;
}

int main(int argc, char const *argv[])
{
    LOG_INFO << "pid = " << getpid() << ", tid = " << tid();
    if (argc > 1)
    {
        kBackendIp = argv[1];
    }
    int number_threads = 4;
    if (argc > 2)
    {
        number_threads = atoi(argv[2]);
    }
    EventLoop loop;
    InetAddress listen_addr(kClientPort);
    InetAddress backend_addr(kBackendIp, kBackendPort);
    MultiplexServer server(&loop, listen_addr, backend_addr, number_threads);
    server.start();
    loop.loop();
    return 0;
}
