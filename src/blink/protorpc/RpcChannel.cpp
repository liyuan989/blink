#include <blink/protorpc/RpcChannel.h>
#include <blink/protorpc/rpc.pb.h>
#include <blink/Log.h>

#include <google/protobuf/descriptor.h>

#include <boost/scoped_ptr.hpp>
#include <boost/bind.hpp>

namespace blink
{

RpcChannel::RpcChannel()
    : codec_(boost::bind(&RpcChannel::onRpcMessage, this, _1, _2, _3)),
      services_(NULL)
{
    LOG_INFO << "RpcChannel::RpcChannel - " << this;
}

RpcChannel::RpcChannel(const TcpConnectionPtr& connection)
    : codec_(boost::bind(&RpcChannel::onRpcMessage, this, _1, _2, _3)),
      connection_(connection),
      services_(NULL)
{
    LOG_INFO << "RpcChannel::RpcChannel - " << this;
}

RpcChannel::~RpcChannel()
{
    LOG_INFO << "RpcChannel::~RpcChannel - " << this;
    for (std::map<int64_t, OutstandingCall>::iterator it = outstandings_.begin();
         it != outstandings_.end(); ++it)
    {
        OutstandingCall out = it->second;
        delete out.response;
        delete out.done;
    }
}

void RpcChannel::CallMethod(const ::google::protobuf::MethodDescriptor* method,
                            ::google::protobuf::RpcController* controller,
                            const ::google::protobuf::Message* request,
                            ::google::protobuf::Message* response,
                            ::google::protobuf::Closure* done)
{
    RpcMessage message;
    message.set_type(REQUEST);
    int64_t id = id_.incrementAndGet();
    message.set_id(id);
    message.set_service(method->service()->name());
    message.set_method(method->name());
    message.set_request(request->SerializeAsString());
    OutstandingCall out = {response, done};
    {
        MutexLockGuard guard(mutex_);
        outstandings_[id] = out;
    }
    codec_.send(connection_, message);
}

void RpcChannel::onMessage(const TcpConnectionPtr& connection,
                           Buffer* buf,
                           Timestamp receive_time)
{
    codec_.onMessage(connection, buf, receive_time);
}

void RpcChannel::onRpcMessage(const TcpConnectionPtr& connection,
                              const RpcMessagePtr& message_ptr,
                              Timestamp receive_time)
{
    assert(connection == connection_);
    RpcMessage message = *message_ptr;
    if (message.type() == RESPONSE)
    {
        int64_t id = message.id();
        assert(message.has_response() || message.has_error());
        OutstandingCall out = {NULL, NULL};

        {
            MutexLockGuard guard(mutex_);
            std::map<int64_t, OutstandingCall>::iterator it = outstandings_.find(id);
            if (it != outstandings_.end())
            {
                out = it->second;
                outstandings_.erase(it);
            }
        }

        if (out.response)
        {
            boost::scoped_ptr<google::protobuf::Message> d(out.response);
            if (message.has_response())
            {
                out.response->ParseFromString(message.response());
            }
            if (out.done)
            {
                out.done->Run();
            }
        }
    }
    else if (message.type() == REQUEST)
    {
        // FIXME: extract to a function
        ErrorCode error = WRONG_PROTO;
        if (services_)
        {
            std::map<std::string, google::protobuf::Service*>::const_iterator it =
                services_->find(message.service());
            if (it != services_->end())
            {
                google::protobuf::Service* service = it->second;
                assert(service != NULL);
                const google::protobuf::ServiceDescriptor* desc = service->GetDescriptor();
                const google::protobuf::MethodDescriptor* method = desc->FindMethodByName(message.method());
                if (method)
                {
                    boost::scoped_ptr<google::protobuf::Message> request(
                        service->GetRequestPrototype(method).New());
                    if (request->ParseFromString(message.request()))
                    {
                        google::protobuf::Message* response = service->GetResponsePrototype(method).New();
                        // response is deleted in doneCallback
                        int64_t id = message.id();
                        service->CallMethod(method, NULL, request.get(), response,
                                            NewCallback(this, &RpcChannel::doneCallback, response, id));
                        error = NO_ERROR;
                    }
                    else
                    {
                        error = INVALID_REQUEST;
                    }
                }
                else
                {
                    error = NO_METHOD;
                }
            }
            else
            {
                error = NO_SERVICE;
            }
        }
        else
        {
            error = NO_SERVICE;
        }
        if (error != NO_ERROR)
        {
            RpcMessage response;
            response.set_type(RESPONSE);
            response.set_id(message.id());
            response.set_error(error);
            codec_.send(connection_, response);
        }
    }
    else if (message.type() == ERROR)
    {
        // FIXME:
    }
}

void RpcChannel::doneCallback(::google::protobuf::Message* response, int64_t id)
{
    boost::scoped_ptr<google::protobuf::Message> d(response);
    RpcMessage message;
    message.set_type(RESPONSE);
    message.set_id(id);
    message.set_response(response->SerializeAsString());  // FIXME: check error
    codec_.send(connection_, message);
}

}  // namespace blink
