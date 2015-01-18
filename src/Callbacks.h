#ifndef __BLINK_CALLBACKS_H__
#define __BLINK_CALLBACKS_H__

#include "Timestamp.h"

#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>

namespace blink
{

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
