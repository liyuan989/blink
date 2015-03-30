#include <blink/FileTool.h>
#include <blink/Types.h>
#include <blink/Log.h>

#include <boost/static_assert.hpp>

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#include <algorithm>
#include <assert.h>
#include <errno.h>
#include <stdio.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

namespace blink
{

const int ReadSmallFile::kBufferSize;

ReadSmallFile::ReadSmallFile(StringArg filename)
    : fd_(::open(filename.c_str(), O_RDONLY | O_CLOEXEC)), errno_(0)
{
    buf_[0] = '\0';
    if (fd_ < 0)
    {
        errno_ = errno;
    }
}

ReadSmallFile::~ReadSmallFile()
{
    if (fd_ >= 0)
    {
        ::close(fd_);
    }
}

int ReadSmallFile::readToBuffer(int* size)
{
    int err = errno_;
    if (fd_ >= 0)
    {
        ssize_t n = ::pread(fd_, buf_, kBufferSize, 0);
        if (n >= 0)
        {
            buf_[n] = '\0';
            if (size)
            {
                *size = static_cast<int>(n);
            }
        }
        else
        {
            err = errno;
        }
    }
    return err;
}

//  struct stat
//  {
//      dev_t     st_dev;     /* ID of device containing file */
//      ino_t     st_ino;     /* inode number */
//      mode_t    st_mode;    /* protection */
//      nlink_t   st_nlink;   /* number of hard links */
//      uid_t     st_uid;     /* user ID of owner */
//      gid_t     st_gid;     /* group ID of owner */
//      dev_t     st_rdev;    /* device ID (if special file) */
//      off_t     st_size;    /* total size, in bytes */
//      blksize_t st_blksize; /* blocksize for filesystem I/O */
//      blkcnt_t  st_blocks;  /* number of blocks allocated */
//      time_t    st_atime;   /* time of last access */
//      time_t    st_mtime;   /* time of last modification */
//      time_t    st_ctime;   /* time of last status change */
//  };

template<typename String>
int ReadSmallFile::readToString(int maxsize,
                            String* destination,
                            int64_t* filesize,
                            int64_t* creat_time,
                            int64_t* modify_time)
{
    //BOOST_STATIC_CAST(sizoef(off_t) == 8);
    assert(destination != NULL);
    int err = errno_;
    if (fd_ >= 0)
    {
        destination->clear();
        if (filesize)
        {
            struct stat stat_info;
            if (::fstat(fd_, &stat_info) == 0)
            {
                if (S_ISREG(stat_info.st_mode))
                {
                    *filesize = stat_info.st_size;
                    destination->reserve(static_cast<int>(std::min(implicit_cast<int64_t>(maxsize), *filesize)));
                }
                if (S_ISDIR(stat_info.st_mode))
                {
                    err = EISDIR;
                }
                if (creat_time)
                {
                    *creat_time = stat_info.st_ctime;
                }
                if (modify_time)
                {
                    *modify_time = stat_info.st_mtime;
                }
            }
            else
            {
                err = errno;
            }
        }
        while (destination->size() < implicit_cast<size_t>(maxsize))
        {
            size_t nread = std::min(implicit_cast<size_t>(maxsize) - destination->size(), sizeof(buf_));
            ssize_t n = ::read(fd_, buf_, nread);
            if (n > 0)
            {
                destination->append(buf_, n);
            }
            else
            {
                if (n < 0)
                {
                    err = errno;
                }
                break;
            }
        }
    }
    return err;
}

const int AppendFile::kBufferSize;

AppendFile::AppendFile(StringArg filename)
    : fp_(::fopen(filename.c_str(), "ae")), bytes_(0) //'e' for O_CLOEXEC
{
    assert(fp_);
    ::setbuffer(fp_, buf_, sizeof(buf_));
}

AppendFile::~AppendFile()
{
    ::fclose(fp_);
}

void AppendFile::appendFile(const char* destination, size_t len)
{
    size_t n = ::fwrite(destination, 1, len, fp_);
    size_t left = len - n;
    while (left > 0)
    {
        size_t nwrite = ::fwrite(destination + n, 1, left, fp_);
        if (nwrite == 0)
        {
            int err = ::ferror(fp_);
            if (err)
            {
                fprintf(stderr, "AppendFile::appendFile() failed: %s\n", strerror_rb(err));
                break;
            }
        }
        n += nwrite;
        left -= nwrite;
    }
    bytes_ += len;
}

void AppendFile::flush()
{
    ::fflush(fp_);
}

template int readFile(StringArg filename,
                      int maxsize,
                      string* destination,
                      int64_t* filesize,
                      int64_t* creat_time,
                      int64_t* modify_time);

template int ReadSmallFile::readToString(int maxsize,
                                         string* destination,
                                         int64_t* filesize,
                                         int64_t* creat_time,
                                         int64_t* modify_time);

#ifndef BLINK_STD_STRING

template int readFile(StringArg filename,
                      int maxsize,
                      std::string* destination,
                      int64_t* filesize,
                      int64_t* creat_time,
                      int64_t* modify_time);

template int ReadSmallFile::readToString(int maxsize,
                                         std::string* destination,
                                         int64_t* filesize,
                                         int64_t* creat_time,
                                         int64_t* modify_time);

#endif

}  // namespace blink
