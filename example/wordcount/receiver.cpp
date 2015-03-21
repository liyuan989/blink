#include <example/wordcount/hash.h>

#include <blink/EventLoop.h>
#include <blink/TcpServer.h>
#include <blink/Log.h>

#include <boost/bind.hpp>

#include <fstream>
#include <stdio.h>

using namespace blink;

class WordCountReceiver : Nocopyable
{
public:
    WordCountReceiver(EventLoop* loop, const InetAddress& listen_addr)
        : loop_(loop),
          server_(loop, listen_addr, "WordCountReceiver"),
          senders_(0)
    {
        server_.setConnectionCallback(boost::bind(&WordCountReceiver::onConnection, this, _1));
        server_.setMessageCallback(boost::bind(&WordCountReceiver::onMessage, this, _1, _2, _3));
    }

    void start(int senders)
    {
        LOG_INFO << "start " << senders << " senders";
        senders_ = senders;
        word_counts_.clear();
        server_.start();
    }

private:
    void onConnection(const TcpConnectionPtr& connection)
    {
        LOG_INFO << connection->peerAddress().toIpPort() << " -> "
                 << connection->localAddress().toIpPort() << " is "
                 << (connection->connected() ? "UP" : "DOWN");
        if (!connection->connected())
        {
            if (--senders_ == 0)
            {
                output();
                loop_->quit();
            }
        }
    }

    void onMessage(const TcpConnectionPtr& connection,
                   Buffer* buf,
                   Timestamp receive_time)
    {
        const char* crlf = NULL;
        while ((crlf = buf->findCRLF()) != NULL)
        {
            const char* tab = std::find(buf->peek(), crlf, '\t');
            if (tab != crlf)
            {
                string word(buf->peek(), tab);
                int64_t count = atoll(tab);
                word_counts_[word] += count;
            }
            else
            {
                LOG_ERROR << "Wrong format , no tab found";
                connection->shutdown();
            }
            buf->resetUntil(crlf + 2);
        }
    }

    void output()
    {
        LOG_INFO << "Writing shared";
        std::ofstream out("shared");
        for (WordCountMap::iterator it = word_counts_.begin();
             it != word_counts_.end(); ++it)
        {
            out << it->first << '\t' << it->second << '\n';
        }
    }

    EventLoop*    loop_;
    TcpServer     server_;
    int           senders_;
    WordCountMap  word_counts_;
};

int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s <listen_port> <numbers_of_senders>\n", argv[0]);
        return 1;
    }
    EventLoop loop;
    uint16_t port = static_cast<uint16_t>(atoi(argv[1]));
    InetAddress listen_addr(port);
    WordCountReceiver receiver(&loop, listen_addr);
    receiver.start(atoi(argv[2]));
    loop.loop();
    return 0;
}
