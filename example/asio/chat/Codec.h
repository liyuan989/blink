#ifndef __EXAMPLE_CODEC_H__
#define __EXAMPLE_CODEC_H__

#include "Nocopyable.h"
#include "TcpConnection.h"
#include "SocketBase.h"
#include "Buffer.h"
#include "Log.h"

#include <boost/function.hpp>

#include <string>

class Codec : blink::Nocopyable
{
public:
    typedef boost::function<void (const blink::TcpConnectionPtr&,
                                  const std::string&,
                                  blink::Timestamp)> StringMessageCallback;

    explicit Codec(const StringMessageCallback& cb)
        : message_callback_(cb)
    {
    }

    void onMessage(const blink::TcpConnectionPtr& connection,
                   blink::Buffer* buf,
                   blink::Timestamp receive_time)
    {
        while (buf->readableSize() >= kHeaderlen)
        {
            const int32_t len = blink::sockets::ntoh32(buf->peekInt32());
            if (len > 65536 || len < 0)
            {
                LOG_ERROR << "Invalid length " << len;
                connection->shutdown();
                break;
            }
            else if (buf->readableSize() >= len + kHeaderlen)
            {
                buf->reset(kHeaderlen);
                std::string message(buf->peek(), len);
                message_callback_(connection, message, receive_time);
                buf->reset(len);
            }
            else
            {
                break;
            }
        }
    }

    void send(blink::TcpConnection* connection, std::string message)
    {
        blink::Buffer buf;
        buf.append(&*message.begin(), message.size());
        buf.prependInt32(message.size());
        connection->send(&buf);
    }

private:
    StringMessageCallback  message_callback_;
    static const size_t    kHeaderlen = sizeof(int32_t);
};

const size_t Codec::kHeaderlen;

#endif
