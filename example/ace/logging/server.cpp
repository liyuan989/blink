#include <example/ace/logging/logrecord.pb.h>

#include <blink/protobuf/ProtobufCodecLite.h>
#include <blink/EventLoop.h>
#include <blink/TcpServer.h>
#include <blink/FileTool.h>
#include <blink/Atomic.h>
#include <blink/Log.h>

#include <boost/bind.hpp>

#include <stdio.h>

using namespace blink;

namespace logging
{

extern const char logtag[] = "LOG0";
typedef ProtobufCodecLiteT<LogRecord, logtag> Codec;

class Session : Nocopyable
{
public:
    explicit Session(const TcpConnectionPtr& connection)
        : codec_(boost::bind(&Session::onMessage, this, _1, _2, _3)),
          file_(getFileName(connection))
    {
        connection->setMessageCallback(boost::bind(&Codec::onMessage, &codec_, _1, _2, _3));
    }

private:
    // FIXME: duplicate code LogFile
    // or use LogFile instead
    string getFileName(const TcpConnectionPtr& connection)
    {
        string filename;
        filename += connection->peerAddress().toIp();

        char timebuf[32];
        struct tm tm;
        time_t now = time(NULL);
        gmtime_r(&now, &tm);
        strftime(timebuf, sizeof(timebuf), ".%Y%m%d-%H%M%S.", &tm);
        filename += timebuf;

        char buf[32];
        snprintf(buf, sizeof(buf), "%d", global_count_.incrementAndGet());
        filename += buf;

        filename += ".log";
        LOG_INFO << "Session of " << connection->name() << " file " << filename;
        return filename;
    }

    void onMessage(const TcpConnectionPtr& connection,
                   const MessagePtr& message,
                   Timestamp receive_time)
    {
        LogRecord* log_record = blink::down_cast<LogRecord*>(message.get());
        if (log_record->has_heartbeat())
        {
            // TODO:
        }
        const char* sep = "=============\n";
        std::string str = log_record->DebugString();
        file_.appendFile(str.data(), str.size());
        file_.appendFile(sep, strlen(sep));
        LOG_DEBUG << str;
    }

    Codec               codec_;
    AppendFile          file_;
    static AtomicInt32  global_count_;
};

AtomicInt32 Session::global_count_;

typedef boost::shared_ptr<Session> SessionPtr;

class LogServer : Nocopyable
{
public:
    LogServer(EventLoop* loop, const InetAddress& listen_addr, int num_threads)
        : loop_(loop),
          server_(loop, listen_addr, "AceLoggingServer")
    {
        server_.setConnectionCallback(boost::bind(&LogServer::onConnection, this, _1));
        if (num_threads > 1)
        {
            server_.setThreadNumber(num_threads);
        }
    }

    void start()
    {
        server_.start();
    }

private:
    void onConnection(const TcpConnectionPtr& connection)
    {
        if (connection->connected())
        {
            SessionPtr session(new Session(connection));
            connection->setContext(session);
        }
        else
        {
            connection->setContext(SessionPtr());
        }
    }

    EventLoop*  loop_;
    TcpServer   server_;
};

}  // namespace logging

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <listen_port> [num_threads]\n", argv[0]);
        return 1;
    }
    LOG_INFO << "pid = " << getpid();
    uint16_t port = static_cast<uint16_t>(atoi(argv[1]));
    InetAddress listen_addr(port);
    int num_threads = argc > 2 ? atoi(argv[2]) : 1;
    EventLoop loop;
    logging::LogServer server(&loop, listen_addr, num_threads);
    server.start();
    loop.loop();
    return 0;
}
