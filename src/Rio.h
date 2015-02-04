#ifndef __BLINK_RIO_H__
#define __BLINK_RIO_H__

#include "Nocopyable.h"

#include <unistd.h>

#include <vector>

namespace blink
{

class Rio : Nocopyable
{
public:
    enum Whence
    {
        kBegin = SEEK_SET,
        kCurrent = SEEK_CUR,
        kEnd = SEEK_END,
    };

    Rio(int fd, size_t buffer_size = 8192)
        : fd_(fd),
          count_(0),
          buffer_size_(buffer_size),
          buf_(buffer_size),
          bufptr_(&*buf_.begin())
    {
    }

    void setFd(int fd)
    {
        fd_ = fd;
    }

    void setBufferSize(size_t n);
    ssize_t readBytes(void* usrbuf, size_t n);
    ssize_t writeBytes(void* usrbuf, size_t n);
    ssize_t readLineBuffer(void* usrbuf, size_t maxlen);
    ssize_t readBytesBuffer(void* usrbuf, size_t n);
    off_t seekBytes(off_t offset, Whence whence);

private:
    ssize_t rioRead(char* usrbuf, size_t n);

    int                fd_;
    int                count_;
    size_t             buffer_size_;
    std::vector<char>  buf_;
    char*              bufptr_;
};

}  //namespace blink

#endif
