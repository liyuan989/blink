#ifndef __BLINK_POLLER_H__
#define __BLINK_POLLER_H__

#include "Nocopyable.h"
#include "EventLoop.h"

#include <sys/types.h>

#include <map>
#include <vector>

struct epoll_event;
struct pollfd;

namespace blink
{

class Timestamp;
class Channel;

class Poller : Nocopyable
{
public:
    typedef std::vector<Channel*> ChannelList;

    Poller(EventLoop* loop);
    virtual ~Poller();

    virtual Timestamp poll(int timeout_ms, ChannelList* active_channels) = 0;
    virtual void updateChannel(Channel* channel) = 0;
    virtual void removeChannel(Channel* channel) = 0;

    virtual bool hasChannel(Channel* channel) const;

    static Poller* newDefualtPoller(EventLoop* loop);

    void assertInLoopThread() const
    {
        owner_loop_->assertInLoopThread();
    }

protected:
    typedef std::map<int, Channel*> ChannelMap;
    ChannelMap channels_;

private:
    EventLoop* owner_loop_;
};

//  typedef union epoll_data
//  {
//      void*       ptr;
//      int         fd;
//      __uint32_t  u32;
//      __uint64_t  u64;
//  } epoll_data_t;
//
//  struct epoll_event
//  {
//      __uint32_t events;    /* Epoll events */
//      epoll_data_t data;    /* User data variable */
//  };

class EPollPoller : public Poller
{
public:
    EPollPoller(EventLoop* loop);
    virtual ~EPollPoller();

    virtual Timestamp poll(int timeout_ms, ChannelList* active_channels);
    virtual void updateChannel(Channel* channel);
    virtual void removeChannel(Channel* channel);

private:
    void fillActiveChannels(int number_events, ChannelList* active_channels) const;
    void update(int operation, Channel* channel);

    typedef std::vector<epoll_event> EventList;

    int        epollfd_;
    EventList  events_;

    static const int kInitEventListSize = 16;
};

//  struct pollfd
//  {
//      int    fd;       /* file descriptor */
//      short  events;   /* requested events */
//      short  revents;  /* returned events*/
//  };

class PollPoller : public Poller
{
public:
    PollPoller(EventLoop* loop);
    virtual ~PollPoller();

    virtual Timestamp poll(int timeout_ms, ChannelList* active_channels);
    virtual void updateChannel(Channel* channel);
    virtual void removeChannel(Channel* channel);

private:
    void fillActiveChannels(int number_events, ChannelList* active_channels) const;

    typedef std::vector<struct pollfd> PollFdList;

    PollFdList  pollfds_;
};

}  // namespace blink

#endif
