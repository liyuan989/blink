#include <blink/protorpc/rpc.pb.h>
#include <blink/protorpc/google-inl.h>
#include <blink/protorpc/RpcCodec.h>
#include <blink/Endian.h>
#include <blink/Log.h>

#include <boost/bind.hpp>

namespace blink
{

namespace
{

int protobufVersionCheck()
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    return 0;
}

int dummy = protobufVersionCheck();

}  // anonymous namespace

const char rpctag[] = "RPC0";

}  // namespace blink
