#ifndef __BLINK_GZIPFILE_H__
#define __BLINK_GZIPFILE_H__

#include <blink/Nocopyable.h>
#include <blink/StringPiece.h>

#include <zlib.h>

namespace blink
{


// only support for c++ version >= c++11
class GzipFile : Nocopyable
{
public:
    GzipFile(GzipFile&& rhs)
        : file_(rhs.file_)
    {
        rhs.file_ = NULL;
    }

    ~GzipFile()
    {
        if (file_)
        {
            ::gzclose(file_);
        }
    }

    GzipFile& operator=(GzipFile&& rhs)
    {
        swap(rhs);
        return *this;
    }

    bool valid() const
    {
        return file_ != NULL;
    }

    void swap(GzipFile& rhs)
    {
        std::swap(file_, rhs.file_);
    }

#if ZLIB_VERNUM >= 0x1240
    bool setBuffer(int size)
    {
        return ::gzbuffer(file_, size);
    }
#endif

    // return the number of uncompressed bytes actually read,
    // 0 for eof, -1 for error
    int read(void* buf, int len)
    {
        return ::gzread(file_, buf, len);
    }

    // return the number of uncompressed bytes actually writen
    int write(StringPiece buf)
    {
        return ::gzwrite(file_, buf.data(), buf.size());
    }

    // number of uncompressed bytes
    off_t tell() const
    {
        return ::gztell(file_);
    }

#if ZLIB_VERNUM >= 0x1240
    // number of compressed bytes
    off_t offset() const
    {
        return ::gzoffset(file_);
    }
#endif

    int flush(int f)
    {
        return ::gzflush(file_, f);
    }

    static GzipFile openForRead(StringArg filename)
    {
        return GzipFile(::gzopen(filename.c_str(), "rbe"));
    }

    static GzipFile openForAppend(StringArg filename)
    {
        return GzipFile(::gzopen(filename.c_str(), "abe"));
    }

    static GzipFile openForWriteExclusive(StringArg filename)
    {
        return GzipFile(::gzopen(filename.c_str(), "wbxe"));
    }

    static GzipFile openForWriteTruncate(StringArg filename)
    {
        return GzipFile(::gzopen(filename.c_str(), "wbe"));
    }

private:
    explicit GzipFile(gzFile file)
        : file_(file)
    {
    }

    gzFile  file_;
};

}  // namespace blink

#endif
