#include <example/wordcount/hash.h>

#include <blink/EventLoopThread.h>
#include <blink/CountDownLatch.h>
#include <blink/Nocopyable.h>
#include <blink/TcpClient.h>
#include <blink/Log.h>

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/tokenizer.hpp>
#include <boost/bind.hpp>

#include <fstream>
#include <iostream>
#include <stdio.h>

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

using namespace blink;

size_t g_batchSize = 65536;
const size_t kMaxHashSize = 10 * 1000 * 1000;

class SendThrottler : Nocopyable
{
public:
    SendThrottler(EventLoop* loop, const InetAddress& addr)
        : client_(loop, addr, "Sender"),
          connect_latch_(1),
          disconnect_latch_(1),
          cond_(mutex_),
          congestion_(false)
    {
        LOG_INFO << "SendTHrottler [" << addr.toIpPort() << "]";
        client_.setConnectionCallback(boost::bind(&SendThrottler::onConnection, this, _1));
    }

    void connect()
    {
        client_.connect();
        connect_latch_.wait();
    }

    void disconnect()
    {
        if (buffer_.readableSize() > 0)
        {
            LOG_DEBUG << "send " << buffer_.readableSize() << " bytes";
            connection_->send(&buffer_);
        }
        connection_->shutdown();
        disconnect_latch_.wait();
    }

    void send(const string& word, int64_t count)
    {
        buffer_.append(word);
        // FIXME: use LogStream
        char buf[64];
        snprintf(buf, sizeof(buf), "\t%" PRId64 "\r\n", count);
        buffer_.append(buf);
        if (buffer_.readableSize() >= g_batchSize)
        {
            throttle();
            LOG_DEBUG << "send " << buffer_.readableSize();
            connection_->send(&buffer_);
        }
    }

private:
    void onConnection(const TcpConnectionPtr& connection)
    {
        if (connection->connected())
        {
            connection->setHighWaterMarkCallback(
                boost::bind(&SendThrottler::onHighWaterMark, this), 1024 * 1024);
            connection->setWriteCompleteCallback(
                boost::bind(&SendThrottler::onWriteComplete, this));
            connection_ = connection;
            connect_latch_.countDown();
        }
        else
        {
            connection_.reset();
            disconnect_latch_.countDown();
        }
    }

    void onHighWaterMark()
    {
        MutexLockGuard guard(mutex_);
        congestion_ = true;
    }

    void onWriteComplete()
    {
        MutexLockGuard guard(mutex_);
        bool old_congestion = congestion_;
        congestion_ = false;
        if (old_congestion)
        {
            cond_.wakeup();
        }
    }

    void throttle()
    {
        MutexLockGuard guard(mutex_);
        while (congestion_)
        {
            cond_.wait();
        }
    }

    TcpClient         client_;
    TcpConnectionPtr  connection_;
    CountDownLatch    connect_latch_;
    CountDownLatch    disconnect_latch_;
    Buffer            buffer_;
    MutexLock         mutex_;
    Condition         cond_;
    bool              congestion_;
};

class WordCountSender : Nocopyable
{
public:
    explicit WordCountSender(const std::string& receivers)
        : loop_(loop_thread_.startLoop())
    {
        typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
        boost::char_separator<char> sep(", ");
        tokenizer tokens(receivers, sep);
        for (tokenizer::iterator toke_iter = tokens.begin();
             toke_iter != tokens.end(); ++toke_iter)
        {
            std::string ip_port = *toke_iter;
            size_t colon = ip_port.find(':');
            if (colon != std::string::npos)
            {
                uint16_t port = static_cast<uint16_t>(atoi(ip_port.c_str() + colon + 1));
                InetAddress addr(ip_port.substr(0, colon), port);
                buckets_.push_back(new SendThrottler(loop_, addr));
            }
            else
            {
                assert(0 && "Invalid address");
            }
        }
    }

    void connectAll()
    {
        for (size_t i = 0; i < buckets_.size(); ++i)
        {
            buckets_[i].connect();
        }
        LOG_INFO << "All connected";
    }

    void disconnectAll()
    {
        for (size_t i = 0; i < buckets_.size(); ++i)
        {
            buckets_[i].disconnect();
        }
        LOG_INFO << "All disconnected";
    }

    void processFile(const char* filename)
    {
        LOG_INFO << "processFile " << filename;
        WordCountMap word_counts;
        // FIXME: use mmap to read file
        std::ifstream in(filename);
        string word;
        // FIXME: make local hash optional
        boost::hash<string> hash;
        while (in)
        {
            word_counts.clear();
            while (in >> word)
            {
                word_counts[word] += 1;
                if (word_counts.size() > kMaxHashSize)
                {
                    break;
                }
            }

            LOG_INFO << "send " << word_counts.size() << " records";
            for (WordCountMap::iterator it = word_counts.begin();
                 it != word_counts.end(); ++it)
            {
                size_t index = hash(it->first) % buckets_.size();
                buckets_[index].send(it->first, it->second);
            }
        }
    }

private:
    EventLoopThread                   loop_thread_;
    EventLoop*                        loop_;
    boost::ptr_vector<SendThrottler>  buckets_;
};

int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s <address_of_receivers> <input_file1> [input_file2] ...\n", argv[0]);
        fprintf(stderr, "Example: %s 'ip1:port1,ip2:port2,ip3:port3' input_file1 input_file2\n", argv[0]);
        return 1;
    }
    const char* batch_size = ::getenv("BATCH_SIZE");
    if (batch_size)
    {
        g_batchSize = atoi(batch_size);
    }
    WordCountSender sender(argv[1]);
    sender.connectAll();
    for (int i = 2; i < argc; ++i)
    {
        sender.processFile(argv[i]);
    }
    sender.disconnectAll();
    return 0;
}
