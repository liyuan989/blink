#include <example/memcached/server/MemcacheServer.h>

#include <blink/EventLoop.h>
#include <blink/Atomic.h>
#include <blink/Log.h>

#include <boost/bind.hpp>

using namespace blink;

AtomicInt64 g_cas;

struct MemcacheServer::Stats
{
};

const int MemcacheServer::kShareds;

MemcacheServer::Options::Options()
{
    memset(this, 0, sizeof(*this));
}

MemcacheServer::MemcacheServer(EventLoop* loop, const Options& options)
    : loop_(loop),
      options_(options),
      start_time_(::time(NULL) - 1),
      server_(loop, InetAddress(options.tcp_port), "blink-memcached"),
      stats_(new Stats)
{
    server_.setConnectionCallback(boost::bind(&MemcacheServer::onConnection, this, _1));
}


MemcacheServer::~MemcacheServer()
{
}

void MemcacheServer::start()
{
    server_.start();
}

void MemcacheServer::stop()
{
    loop_->runAfter(3.0, boost::bind(&EventLoop::quit, loop_));
}

bool MemcacheServer::storeItem(const ItemPtr& item, Item::UpdatePolicy policy, bool* exists)
{
    assert(item->neededBytes() == 0);
    MutexLock& mutex = shards_[item->hash() % kShareds].mutex;
    ItemMap& items = shards_[item->hash() % kShareds].items;
    MutexLockGuard guard(mutex);
    ItemMap::const_iterator it = items.find(item);
    *exists = it != items.end();
    if (policy == Item::kSet)
    {
        item->setCas(g_cas.incrementAndGet());
        if (*exists)
        {
            items.erase(it);
        }
        items.insert(item);
    }
    else
    {
        if (policy == Item::kAdd)
        {
            if (*exists)
            {
                return false;
            }
            else
            {
                item->setCas(g_cas.incrementAndGet());
                items.insert(item);
            }
        }
        else if (policy == Item::kReplace)
        {
            if (*exists)
            {
                item->setCas(g_cas.incrementAndGet());
                items.erase(it);
                items.insert(item);
            }
            else
            {
                return false;
            }
        }
        else if (policy == Item::kAppend || policy == Item::kPrepend)
        {
            if (*exists)
            {
                const ConstItemPtr& old_item = *it;
                int new_len = static_cast<int>(item->valueLength() + old_item->valueLength() - 2);
                ItemPtr new_item(Item::makeItem(item->key(),
                                                old_item->flags(),
                                                old_item->rel_exptime(),
                                                new_len,
                                                g_cas.incrementAndGet()));
                if (policy == Item::kAppend)
                {
                    new_item->append(old_item->value(), old_item->valueLength() - 2);
                    new_item->append(item->value(), item->valueLength());
                }
                else
                {
                    new_item->append(item->value(), item->valueLength() - 2);
                    new_item->append(old_item->value(), old_item->valueLength());
                }
                assert(new_item->neededBytes() == 0);
                assert(new_item->endWithCRLF());
                items.erase(it);
                items.insert(new_item);
            }
            else
            {
                return false;
            }
        }
        else if (policy == Item::kCas)
        {
            if (*exists && (*it)->cas() == item->cas())
            {
                item->setCas(g_cas.incrementAndGet());
                items.erase(it);
                items.insert(item);
            }
            else
            {
                return false;
            }
        }
        else
        {
            assert(false);
        }
    }
    return true;
}

ConstItemPtr MemcacheServer::getItem(const ConstItemPtr& key) const
{
    MutexLock& mutex = shards_[key->hash() % kShareds].mutex;
    const ItemMap& items = shards_[key->hash() % kShareds].items;
    MutexLockGuard guard(mutex);
    ItemMap::const_iterator it = items.find(key);
    return it != items.end() ? *it : ConstItemPtr();
}

bool MemcacheServer::deleteItem(const ConstItemPtr& key)
{
    MutexLock& mutex = shards_[key->hash() % kShareds].mutex;
    ItemMap& items = shards_[key->hash() % kShareds].items;
    MutexLockGuard guard(mutex);
    return items.erase(key) == 1;
}

void MemcacheServer::onConnection(const TcpConnectionPtr& connection)
{
    if (connection->connected())
    {
        SessionPtr session(new Session(this, connection));
        MutexLockGuard guard(mutex_);
        assert(sessions_.find(connection->name()) == sessions_.end());
        sessions_[connection->name()] = session;
    }
    else
    {
        MutexLockGuard guard(mutex_);
        assert(sessions_.find(connection->name()) != sessions_.end());
        sessions_.erase(connection->name());
    }
}
