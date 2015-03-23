#ifndef __EXAMPLE_CDNS_RESOLVER_H__
#define __EXAMPLE_CDNS_RESOLVER_H__

#include <blink/StringPiece.h>
#include <blink/InetAddress.h>
#include <blink/Nocopyable.h>
#include <blink/Timestamp.h>

#include <boost/ptr_container/ptr_map.hpp>
#include <boost/function.hpp>

extern "C"
{

// struct hostent
// {
//     char*  h_name;            /* official name of host */
//     char** h_aliases;         /* alias list */
//     int    h_addrtype;        /* host address type */
//     int    h_length;          /* length of address */
//     char** h_addr_list;       /* list of addresses */
// };
//
// #define h_addr h_addr_list[0]    /* for backward compatibility */

struct hostent;
struct ares_channeldata;
typedef ares_channeldata* ares_channel;

}

namespace blink
{

class Channel;
class EventLoop;

}  // namespace blink

class Resolver : blink::Nocopyable
{
public:
    typedef boost::function<void (const blink::InetAddress&)> Callback;

    enum Option
    {
        kDnsAndHostFile,
        kDnsOnly,
    };

    explicit Resolver(blink::EventLoop* loop, Option opt = kDnsAndHostFile);
    ~Resolver();

    bool resolve(blink::StringArg hostname, const Callback& cb);

private:
    void onRead(int sockfd, blink::Timestamp t);
    void onTimer();
    void onQueryResult(int status, struct hostent* result, const Callback& cb);
    void onSockCreate(int sockfd, int type);
    void onSockStateChange(int sockfd, bool read, bool write);

    static void ares_host_callback(void* data, int status, int timeout, struct hostent* hostent);
    static int ares_sock_create_callback(int sockfd, int type, void* data);
    static void ares_sock_state_callback(void* data, int sockfd, int read, int write);

    struct QueryData
    {
        Resolver*  owner;
        Callback   callback;

        QueryData(Resolver* o, const Callback& cb)
            : owner(o), callback(cb)
        {
        }
    };

    typedef boost::ptr_map<int, blink::Channel> ChannelList;

    blink::EventLoop*  loop_;
    ares_channel       ctx_;
    bool               timer_active_;
    ChannelList        channels_;
};

#endif
