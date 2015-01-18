#ifndef __BLINK_EVENTLOOPTHREADPOOL_H__
#define __BLINK_EVENTLOOPTHREADPOOL_H__

#include "Nocopyable.h"

#include <boost/function.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

#include <vector>

namespace blink
{

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool : Nocopyable
{
public:
    typedef boost::function<void (EventLoop*)> ThreadInitCallback;

    EventLoopThreadPool(EventLoop* base_loop);
    ~EventLoopThreadPool();

    void start(const ThreadInitCallback& cb = ThreadInitCallback());

    // valid after caling start().
    // round-robin.
    EventLoop* getNextLoop();

    // valid after caling start().
    // It will always return the same value, with the same hash_code.
    EventLoop* getLoopForHash(size_t hash_code);

    // Get all EventLoop* in loops_.
    // If loops_ is empty, return the base_loop_ anyway.
    std::vector<EventLoop*> getAllLoops();

    bool started() const
    {
        return started_;
    }

    void setThreadNumber(int number_threads)
    {
        number_threads_ = number_threads;
    }

private:
    EventLoop*                          base_loop_;
    bool                                started_;
    int                                 number_threads_;
    int                                 next_;
    boost::ptr_vector<EventLoopThread>  threads_;
    std::vector<EventLoop*>             loops_;
};

}  // namespace blink

#endif
