#ifndef __BLINK_PROTORPC_RPCCHANNEL_H__
#define __BLINK_PROTORPC_RPCCHANNEL_H__

#include <blink/protorpc/RpcCodec.h>
#include <blink/MutexLock.h>
#include <blink/Atomic.h>

#include <google/protobuf/service.h>

#include <boost/shared_ptr.hpp>

#include <map>

// Service and RpcChannel classes are incorporated from
// google/protobuf/service.h

// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://code.google.com/p/protobuf/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

namespace google
{

namespace protobuf
{

// Forward declaration
// Defined in other files
class Descriptor;          // /google/protobuf/descriptor.h
class ServiceDescriptor;   // /google/protobuf/descriptor.h
class MethodDescriptor;    // /google/protobuf/descriptor.h
class Message;             // /google/protobuf/message.h
class Closure;             // /google/protobuf/stubs/common.h
class RpcController;       // /google/protobuf/service.h
class Service;             // /google/protobuf/service.h

}  // namespace protobuf

}  // namespace google

namespace blink
{

// Abstract interface for an RPC channel. An RpcChannel represent a
// communication line to a Service which can be used to call that Service's
// method. The Service may be running on another machine. Normally, you
// should not call an RpcChannel directly, but instead construct a stub Service
// wrapping it. Example:
// FIXME: update here
//     RpcChennel* chennel = new MyRpcChannel("remotehost.example.com:1234");
//     MySerivice* service = new MyService::Stub(channel);
//     service->MyMethod(request, &response, callback);
class RpcChannel : public ::google::protobuf::RpcChannel
{
public:
    RpcChannel();
    explicit RpcChannel(const TcpConnectionPtr& connection);
    virtual ~RpcChannel();

    void setConnection(const TcpConnectionPtr& connection)
    {
        connection_ = connection;
    }

    void setServices(const std::map<std::string, ::google::protobuf::Service*>* services)
    {
        services_ = services;
    }

    // Call the given method of the remote service, The signature of this
    // procedure looks the same as Service::CallMethod(), but the requirements
    // are less strict in one important way:  the request and response objects
    // need not be of any specify class as long as their descriptor are
    // method->input_type() and method->output_type() .
    void CallMethod(const ::google::protobuf::MethodDescriptor* method,
                    ::google::protobuf::RpcController* controller,
                    const ::google::protobuf::Message* request,
                    ::google::protobuf::Message* response,
                    ::google::protobuf::Closure* done);

    void onMessage(const TcpConnectionPtr& connection,
                   Buffer* buf,
                   Timestamp receive_time);

private:
    void doneCallback(::google::protobuf::Message* response, int64_t id);

    void onRpcMessage(const TcpConnectionPtr& connection,
                      const RpcMessagePtr& message_ptr,
                      Timestamp receive_time);

    struct OutstandingCall
    {
        ::google::protobuf::Message*  response;
        ::google::protobuf::Closure*  done;
    };

    RpcCodec                                                    codec_;
    TcpConnectionPtr                                            connection_;
    AtomicInt64                                                 id_;
    MutexLock                                                   mutex_;
    std::map<int64_t, OutstandingCall>                          outstandings_;
    const std::map<std::string, ::google::protobuf::Service*>*  services_;
};

typedef boost::shared_ptr<RpcChannel> RpcChannelPtr;

}  // namespace blink

#endif
