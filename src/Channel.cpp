#include "Channel.h"
#include "Log.h"
#include "EventLoop.h"

#include <poll.h>

#include <sstream>
#include <assert.h>

namespace blink
{

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = POLLIN | POLLPRI;
const int Channel::kWriteEvent = POLLOUT;

Channel::Channel(EventLoop* loop, int file_descriptor)
    : loop_(loop),
      fd_(file_descriptor),
      events_(0),
      revents_(0),
      index_(-1),
      log_hup_(true),
      tied_(false),
      event_handling_(false),
      added_to_loop_(false)
{
}

Channel::~Channel()
{
    assert(!event_handling_);
    assert(!added_to_loop_);
    if (loop_->isInLoopThread())
    {
        assert(!loop_->hasChannel(this));
    }
}

void Channel::handleEvent(Timestamp receive_time)
{
    boost::shared_ptr<void> guard;
    if (tied_)
    {
        guard = tie_.lock();
        if (guard)
        {
            handleEventWithGuard(receive_time);
        }
    }
    else
    {
        handleEventWithGuard(receive_time);
    }
}

void Channel::tie(const boost::shared_ptr<void>& rhs)
{
    tie_ = rhs;
    tied_ = true;
}

void Channel::remove()
{
    assert(isNoneEvent());
    added_to_loop_ = false;
    loop_->removeChannel(this);
}

string Channel::reventsToString() const
{
    return eventsToString(fd_, revents_);
}

string Channel::eventsToString() const
{
    return eventsToString(fd_, events_);
}

string Channel::eventsToString(int fd, int ev)
{
    std::ostringstream os;
    os << fd << ": ";
    if (ev & POLLIN)
    {
        os << "IN ";
    }
    if (ev & POLLPRI)
    {
        os << "PRI ";
    }
    if (ev & POLLOUT)
    {
        os << "OUT ";
    }
    if (ev & POLLHUP)
    {
        os << "HUP ";
    }
    if (ev & POLLRDHUP)
    {
        os << "RDHUP ";
    }
    if (ev & POLLERR)
    {
        os << "ERR ";
    }
    if (ev & POLLNVAL)
    {
        os << "NVAL ";
    }
    return os.str().c_str();
}

void Channel::update()
{
    added_to_loop_ = true;
    loop_->updateChannel(this);
}

void Channel::handleEventWithGuard(Timestamp receive_time)
{
    event_handling_ = true;
    LOG_TRACE << reventsToString();
    if ((revents_ & POLLHUP) && !(revents_ & POLLIN))
    {
        if (log_hup_)
        {
            LOG_WARN << "fd = " << fd_ << " Channel::handle_event() POLLHUP";
        }
        if (close_callback_)
        {
            close_callback_();
        }
    }
    if (revents_ & POLLNVAL)
    {
        LOG_WARN << "fd = " << fd_ << " Channel::handle_event() POLLNVAL";
    }
    if (revents_ & (POLLERR | POLLNVAL))
    {
        if (error_callback_)
        {
            error_callback_();
        }
    }
    if (revents_ & (POLLIN | POLLPRI | POLLRDHUP))
    {
        if (read_callback_)
        {
            read_callback_(receive_time);
        }
    }
    if (revents_ & POLLOUT)
    {
        if (write_callback_)
        {
            write_callback_();
        }
    }
    event_handling_ = false;
}

}  // namespace blink
