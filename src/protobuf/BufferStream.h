#ifndef __BLINK_PROTOBUF_BUFFERSTREAN_H__
#define __BLINK_PROTOBUF_BUFFERSTREAN_H__

#include "Buffer.h"
#include "Log.h"

#include <google/protobuf/io/zero_copy_stream.h>

namespace blink
{

// FIXME:
// class BufferInputStream
// {
// };

class BufferOutputStream : public google::protobuf::io::ZeroCopyOutputStream
{
public:
    BufferOutputStream(Buffer* buf)
        : buffer_(CHECK_NOTNULL(buf))
          original_size_(buf->readableSize())
    {
    }

    virtual ~BufferOutputStream()  // override
    {
    }

    virtual bool Next(void** data, int* size)  // override
    {
        buffer_->ensureWriteableSize(4096);
        *data = buffer_->beginWrite();
        *size = static_cast<int>(buffer_->writeableSize());
        buffer_->haveWritten(*size);
        return true;
    }

    virtual void BackUp(int count)  //override
    {
        buffer_->unwrite(count);
    }

    virtual int64_t ByteCount() const  // override
    {
        return buffer_->readableSize() - original_size_;
    }

private:
    Buffer*  buffer_;
    size_t   original_size_;
};

}  // namespace blink

#endif
