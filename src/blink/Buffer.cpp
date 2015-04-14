#include <blink/Buffer.h>
#include <blink/Types.h>
#include <blink/SocketBase.h>

#include <sys/uio.h>
#include <errno.h>

namespace blink
{

const int Buffer::kPrependSize;
const int Buffer::kBufferSize;
const char Buffer::kCRLF[] = "\r\n";

//  struct iovec
//  {
//      void*   iov_base;   /* Starting address */
//      size_t  iov_len;    /* Length in bytes */
//  };

ssize_t Buffer::readData(int fd, int* err)
{
    char stack_buf[1024 * 64];
    struct iovec iov[2];
    const size_t writeable_size = writeableSize();
    iov[0].iov_base = beginWrite();
    iov[0].iov_len = writeable_size;
    iov[1].iov_base = stack_buf;
    iov[1].iov_len = sizeof(stack_buf);
    const int iov_count = writeable_size < sizeof(stack_buf) ? 2 : 1;
    const ssize_t n = sockets::readv(fd, iov, iov_count);
    if (n < 0)
    {
        *err = errno;
    }
    else if (implicit_cast<size_t>(n) <= writeable_size)
    {
        write_index_ += n;
    }
    else
    {
        write_index_ = buffer_.size();
        append(stack_buf, n - writeable_size);
    }
    return n;
}

}  // namespace blink
