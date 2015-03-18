#ifndef __EXAMPLE_PROTOBUF_CODEC_DISPATCHER_H__
#define __EXAMPLE_PROTOBUF_CODEC_DISPATCHER_H__

#include <blink/Callbacks.h>
#include <blink/Nocopyable.h>

#include <google/protobuf/message.h>

#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>

#ifndef NDEBUG
#include <boost/type_traits/is_base_of.hpp>
#include <boost/static_assert.hpp>
#endif

#include <map>

typedef boost::shared_ptr<google::protobuf::Message> MessagePtr;

class Callback : blink::Nocopyable
{
public:
    virtual ~Callback()
    {
    }

    virtual void onMessage(const blink::TcpConnectionPtr& connection,
                           const MessagePtr& message,
                           blink::Timestamp) const = 0;
};

template<typename T>
class CallbackT : public Callback
{
#ifndef NDEBUG
    BOOST_STATIC_ASSERT((boost::is_base_of<google::protobuf::Message, T>::value));
#endif
public:
    typedef boost::function<void (const blink::TcpConnectionPtr&,
                                  const boost::shared_ptr<T>&,
                                  blink::Timestamp)> ProtobufMessageCallback;

    CallbackT(const ProtobufMessageCallback& message_callback)
        : message_callback_(message_callback)
    {
    }

    virtual ~CallbackT()
    {
    }

    virtual void onMessage(const blink::TcpConnectionPtr& connection,
                           const MessagePtr& message,
                           blink::Timestamp receive_time) const
    {
        boost::shared_ptr<T> concrete = blink::down_pointer_cast<T>(message);
        assert(concrete != NULL);
        message_callback_(connection, concrete, receive_time);
    }

private:
    ProtobufMessageCallback  message_callback_;
};

class ProtobufDispatcher : blink::Nocopyable
{
public:
    typedef boost::function<void (const blink::TcpConnectionPtr&,
                                  const MessagePtr&,
                                  blink::Timestamp)> ProtobufMessageCallback;

    explicit ProtobufDispatcher(const ProtobufMessageCallback& message_callback)
        : default_message_callback_(message_callback)
    {
    }

    void onProtobufMessage(const blink::TcpConnectionPtr& connection,
                           const MessagePtr& message,
                           blink::Timestamp receive_time)
    {
        CallbackMap::const_iterator it = callbacks_.find(message->GetDescriptor());
        if (it != callbacks_.end())
        {
            it->second->onMessage(connection, message, receive_time);
        }
        else
        {
            default_message_callback_(connection, message, receive_time);
        }
    }

    template<typename T>
    void registerMessageCallback(const typename CallbackT<T>::ProtobufMessageCallback& message_callback)
    {
        boost::shared_ptr<CallbackT<T> > pd(new CallbackT<T>(message_callback));
        callbacks_[T::descriptor()] = pd;
    }

private:
    typedef std::map<const google::protobuf::Descriptor*, boost::shared_ptr<Callback> > CallbackMap;

    CallbackMap              callbacks_;
    ProtobufMessageCallback  default_message_callback_;
};

#endif
