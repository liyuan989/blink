#ifndef __BLINK_THREAD_H__
#define __BLINK_THREAD_H__

#include "Nocopyable.h"
#include "Atomic.h"

#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>

#include <pthread.h>

#include <string>

namespace blink
{

class Thread : Nocopyable
{
public:
    typedef boost::function<void ()> ThreadFunc;

    explicit Thread(const ThreadFunc& func, const std::string& thread_name = std::string());
    ~Thread();

    void start();
    int join();

    bool started() const
    {
        return started_;
    }

    pid_t tid() const
    {
        return *tid_;
    }

    const std::string name() const
    {
        return name_;
    }

    static int numCreated()
    {
        return num_created_.get();
    }

private:
    void setDefaultName();

    bool                      started_;
    bool                      joined_;
    pthread_t                 pthread_id_;
    boost::shared_ptr<pid_t>  tid_;
    ThreadFunc                func_;
    std::string               name_;

    static AtomicInt32 num_created_;
};

}  // namespace blink

#endif
