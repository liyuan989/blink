#ifndef __BLINK_PROTORPC_RPCCODEC_H__
#define __BLINK_PROTORPC_RPCCODEC_H__

#include <blink/protobuf/ProtobufCodecLite.h>
#include <blink/Timestamp.h>
#include <blink/Callbacks.h>
#include <blink/Nocopyable.h>

#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>

namespace blink
{

class Buffer;
class RpcMessage;
typedef boost::shared_ptr<RpcMessage> RpcMessagePtr;

extern const char rpctag[];  // = "RPC0";

// wire format
//
// Field     Length  Content
//
// size      4-byte  N+8
// "RPC0"    4-byte
// payload   N-byte
// checksum  4-byte  adler32 of "RPC0"+payload

typedef ProtobufCodecLiteT<RpcMessage, rpctag> RpcCodec;

}  // namespace blink

#endif
