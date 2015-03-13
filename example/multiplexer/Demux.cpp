#include <blink/TcpServer.h>
#include <blink/TcpClient.h>
#include <blink/Nocopyable.h>
#include <blink/InetAddress.h>
#include <blink/EventLoop.h>
#include <blink/Log.h>

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>

#include <unistd.h>

#include <queue>
#include <map>
#include <stdio.h>

using namespace blink;

typedef boost::shared_ptr<TcpClient> TcpClientPtr;

const size_t kMaxPacketLen = 255;
const size_t kHeaderLen = 3;
const uint16_t kListenPort = 9600;
const uint16_t kSocksPort = 9700;
const char* kSocksIp = "127.0.0.1";

struct Entry
{
    int               connection_id;
    TcpClientPtr      client;
    TcpConnectionPtr  connection;
    Buffer            pending;
};

class DemuxServer : Nocopyable
{
public:
    DemuxServer(EventLoop* loop,
                const InetAddress& listen_addr,
                const InetAddress& socks_addr);

    void start();

private:
    void onServerConnection(const TcpConnectionPtr& connection);
    void onServerMessage(const TcpConnectionPtr& connection,
                         Buffer* buf,
                         Timestamp receive_time);
    void doCommand(const string& command);
    void onSocksConnection(int connection_id, const TcpConnectionPtr& connection);
    void onSocksMessage(int connection_id,
                        const TcpConnectionPtr& connection,
                        Buffer* buf,
                        Timestamp receive_time);
    void sendServerPacket(int connection_id, Buffer* buf);

    EventLoop*            loop_;
    TcpServer             server_;
    TcpConnectionPtr      server_connection_;
    const InetAddress     socks_addr_;
    std::map<int, Entry>  socks_connections_;
};

DemuxServer::DemuxServer(EventLoop* loop,
                         const InetAddress& listen_addr,
                         const InetAddress& socks_addr)
    : loop_(loop),
      server_(loop, listen_addr, "DemuxServer"),
      socks_addr_(socks_addr)
{
    server_.setConnectionCallback(boost::bind(&DemuxServer::onServerConnection, this, _1));
    server_.setMessageCallback(boost::bind(&DemuxServer::onServerMessage, this, _1, _2, _3));
}

void DemuxServer::start()
{
    server_.start();
}

void DemuxServer::onServerConnection(const TcpConnectionPtr& connection)
{
    if (connection->connected())
    {
        if (server_connection_)
        {
            connection->shutdown();
        }
        else
        {
            server_connection_ = connection;
            LOG_INFO << "onServerConnection ser server_connection_";
        }
    }
    else
    {
        if (server_connection_ == connection)
        {
            server_connection_.reset();
            socks_connections_.clear();
            LOG_INFO << "onServerConnection reset server_connection_";
        }
    }
}

void DemuxServer::onServerMessage(const TcpConnectionPtr& connection,
                                  Buffer* buf,
                                  Timestamp receive_time)
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
                assert(socks_connections_.find(id) != socks_connections_.end());
                TcpConnectionPtr& socks_connection = socks_connections_[id].connection;
                if (socks_connection)
                {
                    assert(socks_connections_[id].pending.readableSize() == 0);
                    socks_connection->send(buf->peek() + kHeaderLen, len);
                }
                else
                {
                    socks_connections_[id].pending.append(buf->peek() + kHeaderLen, len);
                }
            }
            else
            {
                string command(buf->peek() + kHeaderLen, len);
                doCommand(command);
            }
            buf->reset(kHeaderLen + len);
        }
    }
}

void DemuxServer::doCommand(const string& command)
{
    static const string kConnectCommand = "CONNECT ";
    int id = atoi(&command[kConnectCommand.size()]);
    bool is_up = command.find(" IS UP") != string::npos;
    LOG_INFO << "doCommand " << id << " " << is_up;
    if (is_up)
    {
        assert(socks_connections_.find(id) == socks_connections_.end());
        char connection_name[256];
        snprintf(connection_name, sizeof(connection_name), "SocksClient %d", id);
        Entry entry;
        entry.connection_id = id;
        entry.client.reset(new TcpClient(loop_, socks_addr_, connection_name));
        entry.client->setConnectionCallback(boost::bind(&DemuxServer::onSocksConnection, this, id, _1));
        entry.client->setMessageCallback(boost::bind(&DemuxServer::onSocksMessage, this, id, _1, _2, _3));
        // FIXME: setWriteCompleteCallback
        socks_connections_[id] = entry;
        entry.client->connect();
    }
    else
    {
        assert(socks_connections_.find(id) != socks_connections_.end());
        TcpConnectionPtr& socks_connection = socks_connections_[id].connection;
        if (socks_connection)
        {
            socks_connection->shutdown();
        }
        else
        {
            socks_connections_.erase(id);  // FIXME: is redundant?
        }
    }
}

void DemuxServer::onSocksConnection(int connection_id, const TcpConnectionPtr& connection)
{
    assert(socks_connections_.find(connection_id) != socks_connections_.end());
    if (connection->connected())
    {
        socks_connections_[connection_id].connection = connection;
        Buffer& pending_data = socks_connections_[connection_id].pending;
        if (pending_data.readableSize() > 0)
        {
            connection->send(&pending_data);
        }
    }
    else
    {
        if (server_connection_)
        {
            char buf[256];
            int len = snprintf(buf, sizeof(buf), "DISCONNECT %d\r\n", connection_id);
            Buffer buffer;
            buffer.append(buf, len);
            sendServerPacket(0, &buffer);
        }
        else
        {
            socks_connections_.erase(connection_id);  // FIXME: is redundant?
        }
    }
}

void DemuxServer::onSocksMessage(int connection_id,
                                 const TcpConnectionPtr& connection,
                                 Buffer* buf,
                                 Timestamp receive_time)
{
    assert(socks_connections_.find(connection_id) != socks_connections_.end());
    while (buf->readableSize() > kMaxPacketLen)
    {
        Buffer packet;
        packet.append(buf->peek(), kMaxPacketLen);
        buf->reset(kMaxPacketLen);
        sendServerPacket(connection_id, &packet);
    }
    if (buf->readableSize() > 0)
    {
        sendServerPacket(connection_id, buf);
    }
}

void DemuxServer::sendServerPacket(int connection_id, Buffer* buf)
{
    size_t len = buf->readableSize();
    LOG_DEBUG << len;
    assert(len <= kMaxPacketLen);
    uint8_t header[kHeaderLen] =
    {
        static_cast<uint8_t>(len),
        static_cast<uint8_t>(connection_id & 0xFF),
        static_cast<uint8_t>((connection_id & 0xFF00) >> 8),
    };
    buf->prepend(header, kHeaderLen);
    if (server_connection_)
    {
        server_connection_->send(buf);
    }
}

int main(int argc, char const *argv[])
{
    LOG_INFO << "pid = " << getpid() << ", tid = " << tid();
    EventLoop loop;
    InetAddress listen_addr(kListenPort);
    if (argc > 1)
    {
        kSocksIp = argv[1];
    }
    InetAddress socks_addr(kSocksIp, kSocksPort);
    DemuxServer server(&loop, listen_addr, socks_addr);
    server.start();
    loop.loop();
    return 0;
}
