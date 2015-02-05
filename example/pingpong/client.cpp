#include "TcpClient.h"
#include "Nocopyable.h"
#include "EventLoop.h"
#include "EventLoopThreadPool.h"
#include "Thread.h"
#include "Log.h"

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/bind.hpp>

#include <unistd.h>

#include <stdio.h>

using namespace blink;

class Client;

class Session : Nocopyable
{
public:
    Session(EventLoop* loop,
            const InetAddress& server_addr,
            const string& name,
            Client* owner)
        : client_(loop, server_addr, name),
          owner_(owner),
          bytes_read_(0),
          bytes_written_(0),
          messages_read_(0)
    {
        client_.setConnectionCallback(boost::bind(&Session::onConnection, this, _1));
        client_.setMessageCallback(boost::bind(&Session::onMessage, this, _1, _2, _3));
    }

    void start()
    {
        client_.connect();
    }

    void stop()
    {
        client_.disconnect();
    }

    int64_t bytesRead() const
    {
        return bytes_read_;
    }

    int64_t bytesWritten() const
    {
        return bytes_written_;
    }

    int64_t messagesRead() const
    {
        return messages_read_;
    }

private:
    void onConnection(const TcpConnectionPtr& connection);

    void onMessage(const TcpConnectionPtr& connection,
                   Buffer* buf,
                   Timestamp receive_time)
    {
        ++messages_read_;
        bytes_read_ += buf->readableSize();
        bytes_written_ += buf->readableSize();
        connection->send(buf);
    }

    TcpClient  client_;
    Client*    owner_;
    int64_t    bytes_read_;
    int64_t    bytes_written_;
    int64_t    messages_read_;
};

class Client : Nocopyable
{
public:
    Client(EventLoop* loop,
           const InetAddress& server_addr,
           int block_size,
           int session_count,
           int timeout,
           int thread_count)
        : loop_(loop),
          thread_pool_(loop),
          session_count_(session_count),
          timeout_(timeout)
    {
        loop->runAfter(timeout, boost::bind(&Client::handleTimeout, this));
        if (thread_count > 1)
        {
            thread_pool_.setThreadNumber(thread_count);
        }
        thread_pool_.start();
        for (int i = 0; i < block_size; ++i)
        {
            message_.push_back(static_cast<char>(i % 128));
        }
        for (int i = 0; i < session_count; ++i)
        {
            char buf[32];
            snprintf(buf, sizeof(buf), "C%05d", i);
            Session* session = new Session(thread_pool_.getNextLoop(), server_addr, buf, this);
            session->start();
            sessions_.push_back(session);
        }
    }

    const string& message() const
    {
        return message_;
    }

    void onConnect()
    {
        if (num_connected_.incrementAndGet() == session_count_)
        {
            LOG_WARN << "all connected";
        }
    }

    void onDisconnect(const TcpConnectionPtr& connection)
    {
        if (num_connected_.decrementAndGet() == 0)
        {
            LOG_WARN << "all disconnected";
            int64_t total_bytes_read = 0;
            int64_t total_messages_read = 0;
            for (boost::ptr_vector<Session>::iterator it = sessions_.begin();
                 it != sessions_.end(); ++it)
            {
                total_bytes_read += it->bytesRead();
                total_messages_read += it->messagesRead();
            }
            LOG_WARN << total_bytes_read << " total bytes read";
            LOG_WARN << total_messages_read << " total message read";
            LOG_WARN << static_cast<double>(total_bytes_read) / static_cast<double>(total_messages_read)
                     << " average message size";
            LOG_WARN << static_cast<double>(total_bytes_read) / (timeout_ * 1024 * 1024)
                     << " MiB/s throughout";
            connection->getLoop()->queueInLoop(boost::bind(&Client::quit, this));
        }
    }

private:
    void quit()
    {
        loop_->queueInLoop(boost::bind(&EventLoop::quit, loop_));
    }

    void handleTimeout()
    {
        LOG_WARN << "stop";
        std::for_each(sessions_.begin(), sessions_.end(), boost::mem_fn(&Session::stop));
    }

    EventLoop*                  loop_;
    EventLoopThreadPool         thread_pool_;
    int                         session_count_;
    int                         timeout_;
    boost::ptr_vector<Session>  sessions_;
    string                      message_;
    AtomicInt32                 num_connected_;
};

void Session::onConnection(const TcpConnectionPtr& connection)
{
    if (connection->connected())
    {
        connection->setTcpNoDelay(true);
        connection->send(owner_->message());
        owner_->onConnect();
    }
    else
    {
        owner_->onDisconnect(connection);
    }
}

int main(int argc, char const *argv[])
{
    if (argc != 7)
    {
        fprintf(stderr, "Usage: %s <ipaddr> <port> <threads> <blocksize> "
                "<sessions> <time>\n", argv[0]);
        return 1;
    }
    LOG_INFO << "pid = " << getpid() << ", tid = " << tid();
    Log::setLogLevel(Log::WARN);
    const char* ip = argv[1];
    uint16_t port = static_cast<uint16_t>(atoi(argv[2]));
    int thread_count = atoi(argv[3]);
    int block_size = atoi(argv[4]);
    int session_count = atoi(argv[5]);
    int timeout = atoi(argv[6]);
    EventLoop loop;
    Client client(&loop, InetAddress(ip, port), block_size, session_count, timeout, thread_count);
    loop.loop();
    return 0;
}
