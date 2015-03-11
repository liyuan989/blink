#ifndef __BLINK_CHANNEL_H__
#define __BLINK_CHANNEL_H__

#include "Nocopyable.h"
#include "Timestamp.h"

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/function.hpp>

namespace blink
{

class EventLoop;

// This class doesn't own the file descriptor.
// The file descriptor could be a socket, an eventfd, a timerfd, or a signalfd.
class Channel : Nocopyable
{
public:
    typedef boost::function<void ()> EventCallback;
    typedef boost::function<void (Timestamp)> ReadEventCallback;

    Channel(EventLoop* loop, int file_descriptor);
    ~Channel();

    void handleEvent(Timestamp receive_time);
    void tie(const boost::shared_ptr<void>& rhs);
    void remove();

    // for debug
    string reventsToString() const;
    string eventsToString() const;

    void setReadCallback(const ReadEventCallback callback)
    {
        read_callback_ = callback;
    }

    void setWriteCallback(const EventCallback callback)
    {
        write_callback_ = callback;
    }

    void setCloseCallback(const EventCallback callback)
    {
        close_callback_ = callback;
    }

    void setErrorCallback(const EventCallback callback)
    {
        error_callback_ = callback;
    }

    int fd() const
    {
        return fd_;
    }

    int events() const
    {
        return events_;
    }

    void setReceiveEvents(int revents)
    {
        revents_ = revents;
    }

    bool isNoneEvent() const
    {
        return events_ == kNoneEvent;
    }

    void enableReading()
    {
        events_ |= kReadEvent;
        update();
    }

    void disableReading()
    {
        events_ &= ~kReadEvent;
        update();
    }

    void enableWriting()
    {
        events_ |= kWriteEvent;
        update();
    }

    void disableWriting()
    {
        events_ &= ~kWriteEvent;
        update();
    }

    void disableAll()
    {
        events_ = kNoneEvent;
        update();
    }

    bool isWriting() const
    {
        return events_ & kWriteEvent;
    }

    int index() const
    {
        return index_;
    }

    void setIndex(int idx)
    {
        index_ = idx;
    }

    void doNotLogHup()
    {
        log_hup_ = false;
    }

    EventLoop* ownerLoop() const
    {
        return loop_;
    }

private:
    void update();
    void handleEventWithGuard(Timestamp receive_time);

    static string eventsToString(int fd, int ev);

    EventLoop*             loop_;
    const int              fd_;
    int                    events_;
    int                    revents_;
    int                    index_;
    bool                   log_hup_;
    boost::weak_ptr<void>  tie_;
    bool                   tied_;
    bool                   event_handling_;
    bool                   added_to_loop_;
    ReadEventCallback      read_callback_;
    EventCallback          write_callback_;
    EventCallback          close_callback_;
    EventCallback          error_callback_;

    static const int       kNoneEvent;
    static const int       kReadEvent;
    static const int       kWriteEvent;
};

}  // namespace blink

#endif
