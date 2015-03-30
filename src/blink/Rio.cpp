#include <blink/Rio.h>

#include <unistd.h>
#include <fcntl.h>

#include <vector>
#include <stdio.h>
#include <string.h>
#include <errno.h>

namespace blink
{

void Rio::setBufferSize(size_t n)
{
    buf_.resize(n);
}

ssize_t Rio::readBytes(void* usrbuf, size_t n)
{
    ssize_t n_left = n;
    ssize_t n_read;
    char* bufp = static_cast<char*>(usrbuf);

    while (n_left > 0)
    {
        if ((n_read = ::read(fd_, bufp, n_left)) < 0)
        {
            if (errno == EINTR)
            {
                n_read = 0;
            }
            else
            {
                return -1;
            }
        }
        else if (n_read == 0)
        {
            break;
        }
        n_left -= n_read;
        bufp += n_read;
    }
    return n - n_left;
}

ssize_t Rio::writeBytes(void* usrbuf, size_t n)
{
    ssize_t n_left = n;
    ssize_t n_write;
    char* bufp = static_cast<char*>(usrbuf);

    while (n_left > 0)
    {
        if ((n_write = ::write(fd_, bufp, n_left)) <= 0)
        {
            if (errno == EINTR)
            {
                n_write = 0;
            }
            else
            {
                return -1;
            }
        }
        n_left -= n_write;
        bufp += n_write;
    }
    return n;
}

ssize_t Rio::readLineBuffer(void* usrbuf, size_t maxlen)
{
    if (maxlen > buffer_size_)
    {
        fprintf(stderr, "error: run out of buffsize range\n");
        return -1;
    }
    size_t n;
    ssize_t rc;
    char c;
    char* bufp = static_cast<char*>(usrbuf);

    for (n = 1; n < maxlen; ++n)
    {
        if ((rc = rioRead(&c, 1)) == 1)
        {
            *bufp = c;
            ++bufp;
            if (c == '\n')
            {
                break;
            }
        }
        else if (rc == 0)
        {
            if (n == 1)
            {
                return 0;
            }
            else
            {
                break;
            }
        }
        else
        {
            return -1;
        }
    }
    *bufp = 0;
    bufptr_ = &*buf_.begin();
    return n;
}

ssize_t Rio::readBytesBuffer(void* usrbuf, size_t n)
{
    if (n > buffer_size_)
    {
        fprintf(stderr, "error: run out of buffsize range\n");
        return -1;
    }
    ssize_t n_left = n;
    ssize_t n_read;
    char* bufp = static_cast<char*>(usrbuf);

    while (n_left > 0)
    {
        if ((n_read = rioRead(bufp, n_left)) < 0)
        {
            if (errno == EINTR)
            {
                n_read = 0;
            }
            else
            {
                return -1;
            }
        }
        else if (n_read == 0)
        {
            break;
        }
        n_left -= n_read;
        bufp += n_read;
    }
    return n - n_left;
}

ssize_t Rio::rioRead(char* usrbuf, size_t n)
{
    if (n > buffer_size_)
    {
        fprintf(stderr, "error: run out of buffsize range\n");
        return -1;
    }
    while (count_ <= 0)
    {
        if ((count_ = static_cast<int>(::read(fd_, bufptr_, buf_.size()))) < 0)
        {
            if (errno != EINTR)
            {
                fprintf(stderr, "rioRead error: %s\n", ::strerror(errno));
                return -1;
            }
        }
        else if (count_ == 0)
        {
            return 0;
        }
        else
        {
            bufptr_ = &*buf_.begin();
        }
    }

    int count = static_cast<int>(n);
    if (count_ < static_cast<int>(n))
    {
        count = count_;
    }
    memcpy(usrbuf, bufptr_, count);
    bufptr_ += count;
    count_ -= count;
    if (count_ == 0)
    {
        bufptr_ = &*buf_.begin();
    }
    return count;
}

off_t Rio::seekBytes(off_t offset, Whence whence)
{
    return ::lseek(fd_, offset, whence);
}

}  //namespace blink
