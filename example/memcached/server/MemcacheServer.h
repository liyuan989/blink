#ifndef __EXAMPLE_MEMCACHED_MEMCACHESERVER_H__
#define __EXAMPLE_MEMCACHED_MEMCACHESERVER_H__

#include "Item.h"
#include "Session.h"

#include "Nocopyable.h"
#include "TcpServer.h"
#include "MutexLock.h"

#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#include <boost/array.hpp>

namespace boost
{

std::size_t hash_value(const blink::string& str)
{
    return hash_range(str.begin(), str.end());
}

}  // namespace boost

class MemcacheServer : blink::Nocopyable
{
public:
    struct Options
    {
        uint16_t  tcp_port;
        uint16_t  udp_port;
        uint16_t  gperf_port;
        int       threads;
    };

    MemcacheServer(blink::EventLoop* loop, const Options& options);
    ~MemcacheServer();

    void start();
    void stop();
    bool storeItem(const ItemPtr& item, Item::UpdatePolicy policy, bool* exists);
    ConstItemPtr getItem(const ConstItemPtr& key) const;
    bool deleteItem(const ConstItemPtr& key);

    void setThreadNumber(int threads)
    {
        server_.setThreadNumer(threads);
    }

    time_t startTime() const
    {
        return start_time_;
    }

private:
    void onConnection(const blink::TcpConnectionPtr& connection);

    // a complicated solution to save memory
    struct Hash
    {
        size_t operator()(const ConstItemPtr& x) const
        {
            return x->hash();
        }
    };

    struct Equal
    {
        bool operator()(const ConstItemPtr& x, const ConstItemPtr& y) const
        {
            return x->key() == y->key();
        }
    };

    typedef boost::unordered_set<ConstItemPtr, Hash, Equal> ItemMap;

    struct MapWithLock
    {
        ItemMap                   items;
        mutable blink::MutexLock  mutex;
    };

    struct States;

    static const int kShareds = 4096;

    blink::EventLoop*                                loop_;
    Options                                          options_;
    const time_t                                     start_time_;
    mutable blink::MutexLock                         mutex_;
    boost::unordered_map<blink::string, SessionPtr>  sessions_;
    boost::array<MapWithLock, kShareds>              shards_;
    blink::TcpServer                                 server_;
    boost::scoped_ptr<States>                        stats_;

    // server_ is not guarded by mutex_, but here because it has to destruct before sessions_
};

#endif
