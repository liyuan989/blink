#ifndef __BLINK_CALLBACKS_H__
#define __BLINK_CALLBACKS_H__

#include "Timestamp.h"

#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>

namespace blink
{

// Adapted from google-protobuf stubs/common.h
template<typename To, typename From>
inline boost::shared_ptr<To> down_pointer_cast(const boost::shared_ptr<From>& f)
{
    if (false)
    {
        implicit_cast<From, To>(0);
    }

#ifndef NDEBUG
    assert(f == NULL || dynamic_cast<To*>(boost::get_pointer(f)) != NULL);
#endif

    return boost::static_pointer_cast<To>(f);
}

class Buffer;
class TcpConnection;

typedef boost::shared_ptr<TcpConnection> TcpConnectionPtr;
typedef boost::function<void ()> TimerCallback;
typedef boost::function<void (const TcpConnectionPtr&)> ConnectionCallback;
typedef boost::function<void (const TcpConnectionPtr&)> CloseCallback;
typedef boost::function<void (const TcpConnectionPtr&)> WriteCompleteCallback;
typedef boost::function<void (const TcpConnectionPtr&, size_t)> HighWaterMarkCallback;

// The data has been read to (buf, len)
typedef boost::function<void (const TcpConnectionPtr&, Buffer*, Timestamp)> MessageCallback;

// Implement in TcpConnection.cpp
void defaultConnectionCallback(const TcpConnectionPtr& connection);
void defaultMessageCallback(const TcpConnectionPtr& connection, Buffer* buffer, Timestamp receive_time);

}  // namespace blink

#endif
