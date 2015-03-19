#ifndef __BLINK_ZLIBSTREAM_H__
#define __BLINK_ZLIBSTREAM_H__

#include <blink/Nocopyable.h>
#include <blink/Buffer.h>

#pragma GCC diagnostic ignored "-Wold-style-cast"
#include <zlib.h>

namespace blink
{

// input is zlib compressed data, output is uncompressed data
class ZlibInputStream : Nocopyable
{
public:
    explicit ZlibInputStream(Buffer* output)
        : output_(output),
          zerror_(Z_OK)
    {
        memset(&zstream_, 0, sizeof(zstream_));
        zerror_ = inflateInit(&zstream_);
    }

    ~ZlibInputStream()
    {
        finish();
    }

    bool write(StringPiece buf);
    bool write(Buffer* input);
    bool finish();

private:
    int decompress(int flush);

    Buffer*   output_;
    z_stream  zstream_;
    int       zerror_;
};

// input is uncompressed data, output zlib compressed data
class ZlibOutputStream : Nocopyable
{
public:
    explicit ZlibOutputStream(Buffer* output)
        : output_(output),
          zerror_(Z_OK),
          buffer_size_(1024)
    {
        memset(&zstream_, 0, sizeof(zstream_));
        zerror_ = deflateInit(&zstream_, Z_DEFAULT_COMPRESSION);
    }

    ~ZlibOutputStream()
    {
        finish();
    }

    // return last error message or NULL if no error
    const char* zlibErrorMessage() const
    {
        return zstream_.msg;
    }

    int zlibErrorCode() const
    {
        return zerror_;
    }

    int64_t inputBytes() const
    {
        return zstream_.total_in;
    }

    int64_t outputBytes() const
    {
        return zstream_.total_out;
    }

    int internalOutputBufferSize() const
    {
        return buffer_size_;
    }

    bool write(StringPiece buf)
    {
        if (zerror_ != Z_OK)
        {
            return false;
        }
        assert(zstream_.next_in == NULL && zstream_.avail_in == 0);
        void* in = const_cast<char*>(buf.data());
        zstream_.next_in = static_cast<Bytef*>(in);
        zstream_.avail_in = buf.size();
        while (zstream_.avail_in > 0 && zerror_ == Z_OK)
        {
            zerror_ = compress(Z_NO_FLUSH);
        }
        if (zstream_.avail_in == 0)
        {
            assert(static_cast<const void*>(zstream_.next_in) == buf.end());
            zstream_.next_in = NULL;
        }
        return zerror_ == Z_OK;
    }

    // compress input as much as possible, not guarantee consuming all data
    bool write(Buffer* input)
    {
        if (zerror_ != Z_OK)
        {
            return false;
        }
        void* in = const_cast<char*>(input->peek());
        zstream_.next_in = static_cast<Bytef*>(in);
        zstream_.avail_in = static_cast<int>(input->readableSize());
        if (zstream_.avail_in > 0 && zerror_ == Z_OK)
        {
            zerror_ = compress(Z_NO_FLUSH);
        }
        input->reset(input->readableSize() - zstream_.avail_in);
        return zerror_ = Z_OK;
    }

    bool finish()
    {
        if (zerror_ != Z_OK)
        {
            return false;
        }
        while (zerror_ == Z_OK)
        {
            zerror_ = compress(Z_FINISH);
        }
        zerror_ = deflateEnd(&zstream_);
        bool ok = zerror_ == Z_OK;
        zerror_ = Z_STREAM_END;
        return ok;
    }

private:
    int compress(int flush)
    {
        output_->ensureWriteableSize(buffer_size_);
        zstream_.next_out = reinterpret_cast<Bytef*>(output_->beginWrite());
        zstream_.avail_out = static_cast<int>(output_->writeableSize());
        int error = deflate(&zstream_, flush);
        output_->haveWritten(output_->writeableSize() - zstream_.avail_out);
        if (output_->writeableSize() == 0 && buffer_size_ < 65536)
        {
            buffer_size_ *= 2;
        }
        return error;
    }

    Buffer*   output_;
    z_stream  zstream_;
    int       zerror_;
    int       buffer_size_;
};

}  // namespace blink

#endif
