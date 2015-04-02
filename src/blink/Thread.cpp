#include <blink/Thread.h>
#include <blink/ThreadBase.h>
#include <blink/CurrentThread.h>
#include <blink/Exception.h>
#include <blink/Log.h>

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/function.hpp>

#include <sys/prctl.h>

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

namespace blink
{

void resetAfterFork()
{
    current_thread::t_cache_tid = 0;
    current_thread::t_thread_name = "main";
    current_thread::tid();
}

class ThreadNameInitializer
{
public:
    ThreadNameInitializer()
    {
        current_thread::t_thread_name = "main";
        current_thread::tid();
        threads::pthread_atfork(NULL, NULL, resetAfterFork);
    }
};

ThreadNameInitializer g_nameInitializer;

class ThreadData
{
public:
    typedef Thread::ThreadFunc ThreadFunc;

    ThreadData(const ThreadFunc& func,
               const string& name,
               const boost::weak_ptr<pid_t>& weak_tid)
        : func_(func), name_(name), weak_tid_(weak_tid)
    {
    }

    void runInThread()
    {
        pid_t tid = current_thread::tid();
        boost::shared_ptr<pid_t> ptid = weak_tid_.lock();
        if (ptid)
        {
            *ptid = tid;
            ptid.reset();
        }
        current_thread::t_thread_name = name_.empty() ? "Thread" : name_.c_str();
        ::prctl(PR_SET_NAME, current_thread::t_thread_name);

        try
        {
            func_();
            current_thread::t_thread_name = "finished";
        }
        catch (const Exception& e)
        {
            current_thread::t_thread_name = "crashed";
            fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
            fprintf(stderr, "reason: %s\n", e.what());
            fprintf(stderr, "stack trace: %s\n", e.stackTrace());
            abort();
        }
        catch (const std::exception& e)
        {
            current_thread::t_thread_name = "crashed";
            fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
            fprintf(stderr, "reason: %s\n", e.what());
            abort();
        }
        catch (...)
        {
            current_thread::t_thread_name = "crashed";
            fprintf(stderr, "unknown exception caught in Thread %s\n", name_.c_str());
            throw;
        }
    }

private:
    ThreadFunc  func_;
    string name_;
    boost::weak_ptr<pid_t> weak_tid_;
};

void* startThread(void* arg)
{
    ThreadData* data = static_cast<ThreadData*>(arg);
    data->runInThread();
    delete data;
    return NULL;
}

AtomicInt32 Thread::num_created_;

Thread::Thread(const ThreadFunc& func, const string& thread_name)
    : started_(false),
      joined_(false),
      pthread_id_(0),
      tid_(new pid_t(0)),
      func_(func),
      name_(thread_name)
{
    setDefaultName();
}

Thread::~Thread()
{
    if (started_ && !joined_)
    {
        if (threads::pthread_detach(pthread_id_) != 0)
        {
            char buf[128];
            snprintf(buf, sizeof(buf), "Failed in pthread_detach: %s", name_.c_str());
            LOG_FATAL << buf;
        }
    }
}

void Thread::start()
{
    assert(!started_);
    started_ = true;
    ThreadData* data = new ThreadData(func_, name_, tid_);
    if (threads::pthread_create(&pthread_id_, NULL, startThread, data) != 0)
    {
        started_ = false;
        delete data;
        char buf[128];
        snprintf(buf, sizeof(buf), "Failed in pthread_creat: %s", name_.c_str());
        LOG_FATAL << buf;
    }
}

int Thread::join()
{
    assert(started_);
    assert(!joined_);
    joined_ = true;
    int val = threads::pthread_join(pthread_id_, NULL);
    if (val != 0)
    {
        char buf[128];
        snprintf(buf, sizeof(buf), "Failed in pthread_join: %s", name_.c_str());
        LOG_FATAL << buf;
    }
    return val;
}

void Thread::setDefaultName()
{
    int num = num_created_.incrementAndGet();
    if (name_.empty())
    {
        char buf[32];
        snprintf(buf, sizeof(buf), "Thread%d", num);
        name_ = buf;
    }
}

}  // namespace blink
