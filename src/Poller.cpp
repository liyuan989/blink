#include "Poller.h"
#include "Timestamp.h"
#include "EventLoop.h"
#include "Channel.h"
#include "Log.h"

#include <boost/static_assert.hpp>

#include <sys/epoll.h>
#include <poll.h>
#include <unistd.h>

#include <algorithm>
#include <errno.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

namespace
{

const int kNew = -1;
const int kAdded = 1;
const int kDeleted = 2;

}

namespace blink
{

Poller::Poller(EventLoop* loop)
    : owner_loop_(loop)
{
}

Poller::~Poller()
{
}

bool Poller::hasChannel(Channel* channel) const
{
    assertInLoopThread();
    ChannelMap::const_iterator iter = channels_.find(channel->fd());
    return (iter != channels_.end()) && (iter->second == channel);
}

Poller* Poller::newDefualtPoller(EventLoop* loop)
{
    if (::getenv("USE_POLL"))
    {
        return new PollPoller(loop);
    }
    else
    {
        return new EPollPoller(loop);
    }
}

BOOST_STATIC_ASSERT(EPOLLIN == POLLIN);
BOOST_STATIC_ASSERT(EPOLLPRI == POLLPRI);
BOOST_STATIC_ASSERT(EPOLLOUT == POLLOUT);
BOOST_STATIC_ASSERT(EPOLLRDHUP == POLLRDHUP);
BOOST_STATIC_ASSERT(EPOLLERR == POLLERR);
BOOST_STATIC_ASSERT(EPOLLHUP == POLLHUP);

const int EPollPoller::kInitEventListSize;

EPollPoller::EPollPoller(EventLoop* loop)
    : Poller(loop),
      epollfd_(::epoll_create1(EPOLL_CLOEXEC)),
      events_(kInitEventListSize)
{
    if (epollfd_ < 0)
    {
        LOG_FATAL << "EPollPoller:: EPollPoller";
    }
}

EPollPoller::~EPollPoller()
{
    ::close(epollfd_);
}

Timestamp EPollPoller::poll(int timeout_ms, ChannelList* active_channels)
{
    int number_events = ::epoll_wait(epollfd_, &*events_.begin(), static_cast<int>(events_.size()), timeout_ms);
    int saved_errno = errno;
    Timestamp now(Timestamp::now());
    if (number_events > 0)
    {
        LOG_TRACE << "events happended";
        fillActiveChannels(number_events, active_channels);
        if (static_cast<size_t>(number_events) == events_.size())
        {
            events_.resize(2 * events_.size());
        }
    }
    else if (number_events == 0)
    {
        LOG_TRACE << "nothing happended";
    }
    else
    {
        if (saved_errno != EINTR)
        {
            errno = saved_errno;
            LOG_ERROR << "EPollPoller::poll()";
        }
    }
    return now;
}

void EPollPoller::updateChannel(Channel* channel)
{
    Poller::assertInLoopThread();
    LOG_TRACE << "fd = " << channel->fd() << " events = " << channel->events();
    const int index = channel->index();
    if (index == kNew || index == kDeleted)
    {
        int fd = channel->fd();
        if (index == kNew)
        {
            assert(channels_.find(fd) == channels_.end());
            channels_[fd] = channel;
        }
        else
        {
            assert(channels_.find(fd) != channels_.end());
            assert(channels_[fd] == channel);
        }
        channel->setIndex(kAdded);
        update(EPOLL_CTL_ADD, channel);
    }
    else
    {
        int fd = channel->fd();
        (void)fd;
        assert(channels_.find(fd) != channels_.end());
        assert(channels_[fd] == channel);
        assert(index == kAdded);
        if (channel->isNoneEvent())
        {
            update(EPOLL_CTL_DEL, channel);
            channel->setIndex(kDeleted);
        }
        else
        {
            // FIXME: Use EPOLL_CTL_ADD and ignore the return error.
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

void EPollPoller::removeChannel(Channel* channel)
{
    Poller::assertInLoopThread();
    int fd = channel->fd();
    LOG_TRACE << "fd = " << fd;
    assert(channels_.find(fd) != channels_.end());
    assert(channels_[fd] == channel);
    assert(channel->isNoneEvent());
    int index = channel->index();
    assert(index == kAdded || index == kDeleted);
    size_t n = channels_.erase(fd);
    (void)n;
    assert(n == 1);
    if (index == kAdded)
    {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->setIndex(kNew);
}

void EPollPoller::fillActiveChannels(int number_events, ChannelList* active_channels) const
{
    assert(static_cast<size_t>(number_events) <= events_.size());
    for (int i = 0; i < number_events; ++i)
    {
        Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
#ifndef NDEBUG
        int fd = channel->fd();
        ChannelMap::const_iterator iter = channels_.find(fd);
        assert(iter != channels_.end());
        assert(iter->second == channel);
#endif
        channel->setReceiveEvents(events_[i].events);
        active_channels->push_back(channel);
    }
}

void EPollPoller::update(int operation, Channel* channel)
{
    struct epoll_event event;
    memset(&event, 0, sizeof(event));
    event.events = channel->events();
    event.data.ptr = channel;
    int fd = channel->fd();
    if (::epoll_ctl(epollfd_, operation, fd, &event) < 0)
    {
        if (operation == EPOLL_CTL_DEL)
        {
            LOG_ERROR << "epoll_ctl op=" << operation << " fd=" << fd;
        }
        else
        {
            LOG_FATAL << "epoll_ctl op=" << operation << " fd=" << fd;
        }
    }
}

PollPoller::PollPoller(EventLoop* loop)
    : Poller(loop)
{
}

PollPoller::~PollPoller()
{
}

Timestamp PollPoller::poll(int timeout_ms, ChannelList* active_channels)
{
    int number_events = ::poll(&*pollfds_.begin(), pollfds_.size(), timeout_ms);
    int saved_errno = errno;
    Timestamp now(Timestamp::now());
    if (number_events > 0)
    {
        LOG_TRACE << number_events << " events happended";
        fillActiveChannels(number_events, active_channels);
    }
    else if (number_events == 0)
    {
        LOG_TRACE << " nothing happended";
    }
    else
    {
        if (saved_errno != EINTR)
        {
            errno = saved_errno;
            LOG_ERROR << "PollPoller::poll()";
        }
    }
    return now;
}

void PollPoller::updateChannel(Channel* channel)
{
    Poller::assertInLoopThread();
    LOG_TRACE << "fd = " << channel->fd() << " events = " << channel->events();
    if (channel->index() < 0)
    {
        assert(channels_.find(channel->fd()) == channels_.end());
        struct pollfd pfd;
        pfd.fd = channel->fd();
        pfd.events = static_cast<short>(channel->events());
        pfd.revents = 0;
        pollfds_.push_back(pfd);
        int index = static_cast<int>(pollfds_.size()) - 1;
        channel->setIndex(index);
        channels_[pfd.fd] = channel;
    }
    else
    {
        assert(channels_.find(channel->fd()) == channels_.end());
        assert(channels_[channel->fd()] == channel);
        int index = channel->index();
        assert(0 <= index && index < static_cast<int>(pollfds_.size()));
        struct pollfd& pfd = pollfds_[index];
        assert(pfd.fd == channel->fd() || pfd.fd == -channel->fd() - 1);
        pfd.events = static_cast<short>(channel->events());
        pfd.revents = 0;
        if (channel->isNoneEvent())
        {
            pfd.fd = -channel->fd() - 1;
        }
    }
}

void PollPoller::removeChannel(Channel* channel)
{
    Poller::assertInLoopThread();
    LOG_TRACE << "fd = " << channel->fd();
    assert(channels_.find(channel->fd()) != channels_.end());
    assert(channels_[channel->fd()] == channel);
    assert(channel->isNoneEvent());
    int index = channel->index();
    assert(0 <= index && index < static_cast<int>(pollfds_.size()));
    const struct pollfd& pfd = pollfds_[index];
    (void)pfd;
    assert(pfd.fd == -channel->fd() - 1 && pfd.events == channel->events());
    size_t n = channels_.erase(channel->fd());
    assert(n == 1);
    (void)n;
    if (static_cast<size_t>(index) == pollfds_.size() - 1)
    {
        pollfds_.pop_back();
    }
    else
    {
        int endfd = pollfds_.back().fd;
        std::iter_swap(pollfds_.begin() + index, pollfds_.end() - 1);
        if (endfd < 0)
        {
            endfd = -endfd - 1;
        }
        channels_[endfd]->setIndex(index);
        pollfds_.pop_back();
    }
}

void PollPoller::fillActiveChannels(int number_events, ChannelList* active_channels) const
{
    for (PollFdList::const_iterator iter = pollfds_.begin(); iter != pollfds_.end() && number_events > 0; ++iter)
    {
        if (iter->revents > 0)
        {
            --number_events;
            ChannelMap::const_iterator pmap = channels_.find(iter->fd);
            assert(pmap != channels_.end());
            Channel* channel = pmap->second;
            assert(channel->fd() == iter->fd);
            channel->setReceiveEvents(iter->revents);
            //iter->revents = 0;
            active_channels->push_back(channel);
        }
    }
}

}  // namespace blink
