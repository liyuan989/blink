#ifndef __EXAMPLE_PROTOBUF_CODEC_DISPATCHER_LITE_H__
#define __EXAMPLE_PROTOBUF_CODEC_DISPATCHER_LITE_H__

#include <blink/Callbacks.h>
#include <blink/Nocopyable.h>

#include <google/protobuf/message.h>

#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>

#include <map>

typedef boost::shared_ptr<google::protobuf::Message> MessagePtr;

class ProtobufDispatcherLite : blink::Nocopyable
{
public:
    typedef boost::function<void (const blink::TcpConnectionPtr&,
                                  const MessagePtr&,
                                  blink::Timestamp)>  ProtobufMessageCallback;

    explicit ProtobufDispatcherLite(const ProtobufMessageCallback& message_callback)
        : default_message_callbcak_(message_callback)
    {
    }

    void onProtobufMessage(const blink::TcpConnectionPtr& connection,
                           const MessagePtr& message,
                           blink::Timestamp receive_time) const
    {
        CallbackMap::const_iterator it = callbacks_.find(message->GetDescriptor());
        if (it != callbacks_.end())
        {
            it->second(connection, message, receive_time);
        }
        else
        {
            default_message_callbcak_(connection, message, receive_time);
        }
    }

    void registerMessageCallback(const google::protobuf::Descriptor* descriptor,
                                 const ProtobufMessageCallback& message_callback)
    {
        callbacks_[descriptor] = message_callback;
    }

private:
    typedef std::map<const google::protobuf::Descriptor* , ProtobufMessageCallback> CallbackMap;

    CallbackMap              callbacks_;
    ProtobufMessageCallback  default_message_callbcak_;
};

#endif
