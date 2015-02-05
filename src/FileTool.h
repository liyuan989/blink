#ifndef __BLINK_FILETOOL_H__
#define __BLINK_FILETOOL_H__

#include "Nocopyable.h"
#include "Types.h"

#include <stdint.h>

namespace blink
{

class ReadSmallFile : Nocopyable
{
public:
    static const int kBufferSize = 1024*64;

    explicit ReadSmallFile(const string& filename);
    ~ReadSmallFile();

    int readToBuffer(int* size);
    int readToString(int maxsize,
                     string* destination,
                     int64_t* filesize,
                     int64_t* creat_time,
                     int64_t* modify_time);

    const char* buffer() const
    {
        return buf_;
    }

private:
    int   fd_;
    int   errno_;
    char  buf_[kBufferSize];
};

int readFile(const string& filename,
             int maxsize,
             string* destination,
             int64_t* filesize = NULL,
             int64_t* creat_time = NULL,
             int64_t* modify_time = NULL);

class AppendFile
{
public:
    const static int kBufferSize = 1024*64;

    explicit AppendFile(const string& filename);
    ~AppendFile();

    void appendFile(const char* destination, size_t len);
    void flush();

    size_t writtenBytes() const
    {
        return bytes_;
    }

private:
    FILE*   fp_;
    size_t  bytes_;
    char    buf_[kBufferSize];
};

}  // namespace blink

#endif
