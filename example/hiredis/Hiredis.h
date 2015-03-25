#ifndef __EXAMPLE_HIREDIS_HIREDIS_H__
#define __EXAMPLE_HIREDIS_HIREDIS_H__

#include <blink/StringPiece.h>
#include <blink/InetAddress.h>
#include <blink/Nocopyable.h>
#include <blink/Callbacks.h>
#include <blink/Types.h>

#include <hiredis/hiredis.h>

#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>

class redisAsyncContext;

namespace blink
{

class Channel;
class EventLoop;

}  // namespace blink

class Hiredis : blink::Nocopyable,
                public boost::enable_shared_from_this<Hiredis>
{
public:
    typedef boost::function<void (Hiredis*, int)> ConnectCallback;
    typedef boost::function<void (Hiredis*, int)> DisconnectCallback;
    typedef boost::function<void (Hiredis*, redisReply*)> CommandCallback;

    Hiredis(blink::EventLoop* loop, const blink::InetAddress& server_addr);
    ~Hiredis();

    const blink::InetAddress& serverAddress() const
    {
        return server_addr_;
    }

    redisAsyncContext* context()
    {
        return context_;
    }

    void setConnectCallback(const ConnectCallback& cb)
    {
        connect_callback_ = cb;
    }

    void setDisconnectCallback(const DisconnectCallback& cb)
    {
        disconnect_callback_ = cb;
    }

    bool connected() const;
    const char* errstr() const;
    void connect();
    void disconnect();  // FIXME: implement this with redisAsyncDisconnect
    int command(const CommandCallback& cb, blink::StringArg cmd, ...);
    int ping();

private:
    void handleRead(blink::Timestamp receive_time);
    void handleWrite();
    int fd() const;
    void logConnection(bool up) const;
    void setChannel();
    void removeChannel();
    void connectCallback(int status);
    void disconnectCallback(int status);
    void commandCallback(redisReply* reply, CommandCallback* privdata);
    void pingCallback(Hiredis* me, redisReply* reply);

    static Hiredis* getHiredis(const redisAsyncContext* ac);
    static void connectCallback(const redisAsyncContext* ac, int status);
    static void disconnectCallback(const redisAsyncContext* ac, int status);
    static void commandCallback(redisAsyncContext* ac, void* r, void* privdata);
    static void addRead(void* privdata);
    static void delRead(void* privdata);
    static void addWrite(void* privdata);
    static void delWrite(void* privdata);
    static void cleanup(void* privdata);

    blink::EventLoop*                  loop_;
    const blink::InetAddress           server_addr_;
    redisAsyncContext*                 context_;
    boost::shared_ptr<blink::Channel>  channel_;
    ConnectCallback                    connect_callback_;
    DisconnectCallback                 disconnect_callback_;
};

#endif
