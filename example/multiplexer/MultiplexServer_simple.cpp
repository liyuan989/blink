#include "TcpServer.h"
#include "TcpClient.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "Nocopyable.h"
#include "Log.h"

#include <boost/bind.hpp>

#include <unistd.h>

#include <queue>
#include <map>
#include <stdio.h>

using namespace blink;

const int kMaxConnections = 10;  // max id = 65535
const size_t kMaxPacketLen = 255;
const size_t kHeaderLen = 3;
const uint16_t kClientPort = 9600;
const uint16_t kBackendPort = 9700;
const char* kBackendIp = "127.0.0.1";

class MultiplexServer : Nocopyable
{
public:
    MultiplexServer(EventLoop* loop,
                    const InetAddress& listen_addr,
                    const InetAddress& backend_addr);

    void start();

private:
    void onClientConnection(const TcpConnectionPtr& connection);
    void onClientMessageConnection(const TcpConnectionPtr& connection,
                                   Buffer* buf,
                                   Timestamp receive_time);
    void sendBackendString(int id, const std::string& message);
    void sendBackendBuffer(int id, Buffer* buf);
    void sendBackendPacket(int id, Buffer* buf);
    void onBackendConnection(const TcpConnectionPtr& connection);
    void onBackendMessageConnection(const TcpConnectionPtr& connection,
                                    Buffer* buf,
                                    Timestamp receive_time);
    void sendToClient(Buffer* buf);
    void doCommand(const std::string& command);

    TcpServer                        server_;
    TcpClient                        backend_;
    TcpConnectionPtr                 backend_connection_;
    std::map<int, TcpConnectionPtr>  client_connections_;
    std::queue<int>                  available_ids_;
};

MultiplexServer::MultiplexServer(EventLoop* loop,
                                 const InetAddress& listen_addr,
                                 const InetAddress& backend_addr)
    : server_(loop, listen_addr, "MultiplexServer"),
      backend_(loop, backend_addr, "MultiplexBackend")
{
    server_.setConnectionCallback(boost::bind(&MultiplexServer::onClientConnection, this, _1));
    server_.setMessageCallback(boost::bind(&MultiplexServer::onClientMessageConnection, this, _1, _2, _3));
    backend_.setConnectionCallback(boost::bind(&MultiplexServer::onBackendConnection, this, _1));
    backend_.setMessageCallback(boost::bind(&MultiplexServer::onBackendMessageConnection, this, _1, _2, _3));
    backend_.enableRetry();
}

void MultiplexServer::start()
{
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
        if (!available_ids_.empty())
        {
            id = available_ids_.front();
            available_ids_.pop();
            client_connections_[id] = connection;
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

void MultiplexServer::onClientMessageConnection(const TcpConnectionPtr& connection,
                                                Buffer* buf,
                                                Timestamp receive_time)
{
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
    if (backend_connection_)
    {
        backend_connection_->send(buf);
    }
}

void MultiplexServer::onBackendConnection(const TcpConnectionPtr& connection)
{
    LOG_TRACE << "Backend " << connection->localAddress().toIpPort() << " -> "
              << connection->peerAddress().toIpPort() << " is "
              << (connection->connected() ? "UP" : "DOWN");
    if (connection->connected())
    {
        backend_connection_ = connection;
        assert(available_ids_.empty());
        for (int i = 1; i <= kMaxConnections; ++i)
        {
            available_ids_.push(i);
        }
    }
    else
    {
        backend_connection_.reset();
        for (std::map<int, TcpConnectionPtr>::iterator it = client_connections_.begin();
             it != client_connections_.end(); ++it)
        {
            it->second->shutdown();
        }
        client_connections_.clear();
        while (!available_ids_.empty())
        {
            available_ids_.pop();
        }
    }
}

void MultiplexServer::onBackendMessageConnection(const TcpConnectionPtr& connection,
                                                 Buffer* buf,
                                                 Timestamp receive_time)
{
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
            if (id != 0)
            {
                std::map<int, TcpConnectionPtr>::iterator it = client_connections_.find(id);
                if (it != client_connections_.end())
                {
                    it->second->send(buf->peek() + kHeaderLen, len);
                }
            }
            else
            {
                std::string command(buf->peek() + kHeaderLen, len);
                LOG_INFO << "Backend command " << command;
                doCommand(command);
            }
            buf->reset(kHeaderLen + len);
        }
    }
}

void MultiplexServer::doCommand(const std::string& command)
{
    static const std::string kDisconnectedCommand = "DISCONNECT ";
    if (command.size() > kDisconnectedCommand.size()
        && std::equal(kDisconnectedCommand.begin(), kDisconnectedCommand.end(), command.begin()))
    {
        int id = atoi(&command[kDisconnectedCommand.size()]);
        std::map<int, TcpConnectionPtr>::iterator it = client_connections_.find(id);
        if (it != client_connections_.end())
        {
            it->second->shutdown();
        }
    }
}

int main(int argc, char const *argv[])
{
    LOG_INFO << "pid = " << getpid() << ", tid = " << tid();
    if (argc > 1)
    {
        kBackendIp = argv[1];
    }
    EventLoop loop;
    InetAddress listen_addr(kClientPort);
    InetAddress backend_addr(kBackendIp, kBackendPort);
    MultiplexServer server(&loop, listen_addr, backend_addr);
    server.start();
    loop.loop();
    return 0;
}
