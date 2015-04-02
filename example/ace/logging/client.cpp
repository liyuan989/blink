#include <example/ace/logging/logrecord.pb.h>

#include <blink/protobuf/ProtobufCodecLite.h>
#include <blink/EventLoopThread.h>
#include <blink/EventLoop.h>
#include <blink/ProcessInfo.h>
#include <blink/TcpClient.h>
#include <blink/MutexLock.h>
#include <blink/Log.h>

#include <boost/bind.hpp>

#include <iostream>
#include <stdio.h>

using namespace blink;

// just to verify the protocol, not for practical usage.
namespace logging
{

extern const char logtag[] = "LOG0";
typedef ProtobufCodecLiteT<LogRecord, logtag> Codec;

// same as asio/chat/client.cpp
class LogClient : Nocopyable
{
public:
    LogClient(EventLoop* loop, InetAddress server_addr)
        : client_(loop, server_addr, "LogClient"),
          codec_(boost::bind(&LogClient::onMessage, this, _1, _2, _3))
    {
        client_.setConnectionCallback(boost::bind(&LogClient::onConnection, this, _1));
        client_.setMessageCallback(boost::bind(&Codec::onMessage, &codec_, _1, _2, _3));
        client_.enableRetry();
    }

    void connect()
    {
        client_.connect();
    }

    void disconnect()
    {
        client_.disconnect();
    }

    void write(const StringPiece& message)
    {
        MutexLockGuard guard(mutex_);
        updateLogRecord(message);
        if (connection_)
        {
            codec_.send(connection_, log_record_);
        }
        else
        {
            LOG_WARN << "NOT CONNECTED";
        }
    }

private:
    void onConnection(const TcpConnectionPtr& connection)
    {
        LOG_INFO << connection->localAddress().toIpPort() << " -> "
                 << connection->peerAddress().toIpPort() << " is "
                 << (connection->connected() ? "UP" : "DOWN");
        MutexLockGuard gurad(mutex_);
        if (connection->connected())
        {
            connection_ = connection;
            LogRecord_HeartBeat* hb = log_record_.mutable_heartbeat();
            hb->set_hostname(hostName().c_str());
            hb->set_process_name(procName().c_str());
            hb->set_process_id(pid());
            hb->set_process_start_time(startTime().microSecondsSinceEpoch());
            hb->set_username(username().c_str());
            updateLogRecord("Heartbeat");
            codec_.send(connection_, log_record_);
            log_record_.clear_heartbeat();
            LOG_INFO << "Type message below:";
        }
        else
        {
            connection_.reset();
        }
    }

    void onMessage(const TcpConnectionPtr& connection,
                   const MessagePtr& message,
                   Timestamp receive_time)
    {
        // SHOULD NOT HAPPEN
        LogRecord* log_record = blink::down_cast<LogRecord*>(message.get());
        LOG_WARN << log_record->DebugString();
    }

    void updateLogRecord(const StringPiece& message)
    {
        mutex_.assertLockted();
        log_record_.set_level(1);
        log_record_.set_thread_id(current_thread::tid());
        log_record_.set_timestamp(Timestamp::now().microSecondsSinceEpoch());
        log_record_.set_message(message.data(), message.size());
    }

    TcpClient         client_;
    Codec             codec_;
    MutexLock         mutex_;
    LogRecord         log_record_;
    TcpConnectionPtr  connection_;
};

}  // namespace logging

int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s <server_ip> <port>\n", argv[0]);
        return 1;
    }
    LOG_INFO << "pid = " << getpid();
    EventLoopThread loop_thread;
    uint16_t port = static_cast<uint16_t>(atoi(argv[2]));
    InetAddress server_addr(argv[1], port);
    logging::LogClient client(loop_thread.startLoop(), server_addr);
    client.connect();
    std::string line;
    while (std::getline(std::cin, line))
    {
        client.write(line);
    }
    client.disconnect();

    // wait for disconnect, then safe to destruct LogClient (esp. TcpClient).
    // otherwise, mutex_ is used after destructor.
    current_thread::sleepMicroseconds(1000 * 1000);
    google::protobuf::ShutdownProtobufLibrary();
    return 0;
}
