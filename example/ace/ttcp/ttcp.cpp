#include <example/ace/ttcp/common.h>

#include <blink/TcpServer.h>
#include <blink/TcpClient.h>
#include <blink/EventLoop.h>
#include <blink/Log.h>

#include <boost/bind.hpp>

#include <stdio.h>

using namespace blink;

EventLoop* g_loop = NULL;

struct Context
{
    int             count;
    int64_t         bytes;
    SessionMessage  session;
    Buffer          output;

    Context()
        : count(0), bytes(0)
    {
        session.number = 0;
        session.length = 0;
    }
};

/*****************************************************************
transmit
*****************************************************************/

namespace trans
{

void onConnection(const Options& opt, const TcpConnectionPtr& connection)
{
    if (connection->connected())
    {
        printf("connected\n");
        Context context;
        context.count = 1;
        context.bytes = opt.length;
        context.session.number = opt.number;
        context.session.length = opt.length;
        context.output.appendInt32(opt.length);
        context.output.ensureWriteableSize(opt.length);
        for (int i = 0; i < opt.length; ++i)
        {
            context.output.beginWrite()[i] = "0123456789ABCDEF"[i % 16];
        }
        context.output.haveWritten(opt.length);
        connection->setContext(context);

        SessionMessage session_message = {0, 0};
        session_message.number = htonl(opt.number);
        session_message.length = htonl(opt.length);

        connection->send(&session_message, sizeof(session_message));
        connection->send(context.output.toStringPiece());
    }
    else
    {
        const Context& context = boost::any_cast<Context>(connection->getContext());
        LOG_INFO << "payload bytes " << context.bytes;
        connection->getLoop()->quit();
    }
}

void onMessage(const TcpConnectionPtr& connection, Buffer* buf, Timestamp receive_time)
{
    Context* context = boost::any_cast<Context>(connection->getMutableContext());
    while (buf->readableSize() >= sizeof(int32_t))
    {
        int32_t length = buf->readInt32();
        if (length == context->session.length)
        {
            if (context->count < context->session.number)
            {
                connection->send(context->output.toStringPiece());
                ++context->count;
                context->bytes += length;
            }
            else
            {
                connection->shutdown();
                break;
            }
        }
        else
        {
            connection->shutdown();
            break;
        }
    }
}

}  // namespace trans

void transmit(const Options& opt)
{
    InetAddress addr(opt.port);
    if (!InetAddress::resolve(opt.host, &addr))
    {
        LOG_FATAL << "Unable to resolve " << opt.host;
    }
    Timestamp start = Timestamp::now();
    EventLoop loop;
    g_loop = &loop;
    TcpClient client(&loop, addr, "TcpClient");
    client.setConnectionCallback(boost::bind(trans::onConnection, opt, _1));
    client.setMessageCallback(boost::bind(trans::onMessage, _1, _2, _3));
    client.connect();
    loop.loop();
    double elapsed = timeDifference(Timestamp::now(), start);
    double total_mb = 1.0 * opt.length * opt.number / 1024 / 1024;
    printf("%.3f MiB transferred\n%.3f MiB/s\n", total_mb, total_mb / elapsed);
}

/*****************************************************************
receive
*****************************************************************/

namespace receiving
{

void onConnection(const TcpConnectionPtr& connection)
{
    if (connection->connected())
    {
        Context context;
        connection->setContext(context);
    }
    else
    {
        const Context& context = boost::any_cast<Context>(connection->getContext());
        LOG_INFO << "payload bytes " << context.bytes;
        connection->getLoop()->quit();
    }
}

void onMessage(const TcpConnectionPtr& connection, Buffer* buf, Timestamp receive_time)
{
    while (buf->readableSize() >= sizeof(int32_t))
    {
        Context* context = boost::any_cast<Context>(connection->getMutableContext());
        SessionMessage& session_message = context->session;
        if (session_message.number == 0 && session_message.length == 0)
        {
            if (buf->readableSize() >= sizeof(session_message))
            {
                session_message.number = buf->readInt32();
                session_message.length = buf->readInt32();
                context->output.appendInt32(session_message.length);
                printf("receive number = %d\nreceive length = %d\n",
                       session_message.number, session_message.length);
            }
            else
            {
                break;
            }
        }
        else
        {
            const unsigned total_len = session_message.length + static_cast<int>(sizeof(int32_t));
            const int32_t length = buf->peekInt32();
            if (length == session_message.length)
            {
                if (buf->readableSize() >= total_len)
                {
                    buf->reset(total_len);
                    connection->send(context->output.toStringPiece());
                    ++context->count;
                    context->bytes += length;
                    if (context->count == session_message.number)
                    {
                        connection->shutdown();
                        break;
                    }
                }
                else
                {
                    break;
                }
            }
            else
            {
                printf("wrong length: %d\n", length);
                connection->shutdown();
                break;
            }
        }
    }
}

}  // namespace receiving

void receive(const Options& opt)
{
    EventLoop loop;
    g_loop = &loop;
    InetAddress listen_addr(opt.port);
    TcpServer server(&loop, listen_addr, "TtcpReceive");
    server.setConnectionCallback(boost::bind(receiving::onConnection, _1));
    server.setMessageCallback(boost::bind(receiving::onMessage, _1, _2, _3));
    server.start();
    loop.loop();
}
