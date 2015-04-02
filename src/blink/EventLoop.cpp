#include <blink/EventLoop.h>
#include <blink/SocketBase.h>
#include <blink/ProcessBase.h>
#include <blink/MutexLock.h>
#include <blink/Channel.h>
#include <blink/TimerQueue.h>
#include <blink/Poller.h>
#include <blink/Log.h>

#include <boost/bind.hpp>

#include <sys/eventfd.h>
#include <signal.h>

#include <assert.h>
#include <stdlib.h>

namespace
{

__thread blink::EventLoop* t_loopInThisThread = NULL;
const int kPollTimeMs = 10000;

int createEventfd()
{
    int event_fd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (event_fd < 0)
    {
        LOG_ERROR << "Failed in eventfd";
        abort();
    }
    return event_fd;
}

#pragma GCC diagnostic ignored "-Wold-style-cast"

class IgnoreSigPipe
{
public:
    IgnoreSigPipe()
    {
        blink::processes::signal(SIGPIPE, SIG_IGN);
    }
};

IgnoreSigPipe g_initObject;

}  // anonymous namespace

namespace blink
{

EventLoop* EventLoop::getEventLoopOfCurrentThread()
{
    return t_loopInThisThread;
}

EventLoop::EventLoop()
    : looping_(false),
      quit_(false),
      event_handling_(false),
      calling_pending_functors_(false),
      iteration_(0),
      thread_id_(current_thread::tid()),
      poller_(Poller::newDefualtPoller(this)),
      timer_queue_(new TimerQueue(this)),
      wakeup_fd_(createEventfd()),
      wakeup_channel_(new Channel(this, wakeup_fd_)),
      current_active_channel_(NULL)
{
    LOG_DEBUG << "EventLoop created " << this << " in thread " << thread_id_;
    if (t_loopInThisThread)
    {
        LOG_FATAL << "Another EventLoop " << t_loopInThisThread
                  << " exists in this thread " << thread_id_;
    }
    else
    {
        t_loopInThisThread = this;
    }
    wakeup_channel_->setReadCallback(boost::bind(&EventLoop::handleRead, this));
    wakeup_channel_->enableReading();
}

EventLoop::~EventLoop()
{
    LOG_DEBUG << "EventLoop " << this << " of thread " << thread_id_
              << " destructs in thread " << current_thread::tid();
    wakeup_channel_->disableAll();
    wakeup_channel_->remove();
    ::close(wakeup_fd_);
    t_loopInThisThread = NULL;
}

void EventLoop::loop()
{
    assert(!looping_);
    assertInLoopThread();
    looping_ = true;
    quit_ = false;
    LOG_TRACE << "EventLoop " << this << " start looping";
    while (!quit_)
    {
        active_channels_.clear();
        poll_return_time_ = poller_->poll(kPollTimeMs, &active_channels_);
        ++iteration_;
        if (Log::LogLevel() <= Log::TRACE)
        {
            printActiveChannels();
        }
        event_handling_ = true;
        for (ChannelList::iterator it = active_channels_.begin(); it != active_channels_.end(); ++it)
        {
            current_active_channel_ = *it;
            current_active_channel_->handleEvent(poll_return_time_);
        }
        current_active_channel_ = NULL;
        event_handling_ = false;
        doPendingFunctors();
    }
    LOG_TRACE << "EventLoop " << this << " stop looping";
    looping_ = false;
}

void EventLoop::quit()
{
    quit_ = true;
    if (!isInLoopThread())
    {
        wakeup();
    }
}

void EventLoop::runInLoop(const Functor& cb)
{
    if (isInLoopThread())
    {
        cb();
    }
    else
    {
        queueInLoop(cb);
    }
}

void EventLoop::queueInLoop(const Functor& cb)
{
    {
        MutexLockGuard guard(mutex_);
        pending_functors_.push_back(cb);
    }
    if (!isInLoopThread() || calling_pending_functors_)
    {
        wakeup();
    }
}

TimerId EventLoop::runAt(const Timestamp& time, const TimerCallback& cb)
{
    return timer_queue_->addTimer(cb, time, 0.0);
}

TimerId EventLoop::runAfter(double delay, const TimerCallback& cb)
{
    Timestamp time(addTime(Timestamp::now(), delay));
    return runAt(time, cb);
}

TimerId EventLoop::runEvery(double interval, const TimerCallback& cb)
{
    Timestamp time(addTime(Timestamp::now(), interval));
    return timer_queue_->addTimer(cb, time, interval);
}

void EventLoop::cancel(TimerId timer_id)
{
    return timer_queue_->cancel(timer_id);
}

void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = sockets::write(wakeup_fd_, &one, sizeof(one));
    if (n != sizeof(one))
    {
        LOG_ERROR << "EventLoop::wakeup() writes " << n << " bytes instead of 8";
    }
}

void EventLoop::updateChannel(Channel* channel)
{
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel* channel)
{
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    if (event_handling_)
    {
        assert(current_active_channel_ == channel ||
            std::find(active_channels_.begin(), active_channels_.end(), channel) == active_channels_.end());
    }
    poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel* channel)
{
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    return poller_->hasChannel(channel);
}

void EventLoop::abortNotInLoopThread()
{
    LOG_FATAL << "EventLoop::abortNotInLoopThread - EventLoop " << this
              << " was created in thread_id_ = " << thread_id_
              << ", current thread id = " << current_thread::tid();
}

void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = sockets::read(wakeup_fd_, &one, sizeof(one));
    if (n != sizeof(one))
    {
        LOG_ERROR << "EventLoop::handleRead() reads " << n << " bytes instead of 8";
    }
}

void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;
    calling_pending_functors_ = true;
    {
        MutexLockGuard guard(mutex_);
        functors.swap(pending_functors_);
    }
    for (size_t i = 0; i < functors.size(); ++i)
    {
        functors[i]();
    }
    calling_pending_functors_ = false;
}

void EventLoop::printActiveChannels() const
{
    for (ChannelList::const_iterator it = active_channels_.begin(); it != active_channels_.end(); ++it)
    {
        const Channel* channel = *it;
        LOG_TRACE << "{ " << channel->reventsToString() << " }";
    }
}

}  // namespace blink
