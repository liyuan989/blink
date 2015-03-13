#ifndef __BLINK_FILETOOL_H__
#define __BLINK_FILETOOL_H__

#include <blink/Nocopyable.h>
#include <blink/Types.h>
#include <blink/StringPiece.h>

#include <stdint.h>

namespace blink
{

class ReadSmallFile : Nocopyable
{
public:
    static const int kBufferSize = 1024*64;

    explicit ReadSmallFile(StringArg filename);
    ~ReadSmallFile();

    int readToBuffer(int* size);

    template<typename String>
    int readToString(int maxsize,
                     String* destination,
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

template<typename String>
int readFile(StringArg filename,
             int maxsize,
             String* destination,
             int64_t* filesize = NULL,
             int64_t* creat_time = NULL,
             int64_t* modify_time = NULL)
{
    ReadSmallFile file(filename);
    return file.readToString(maxsize, destination, filesize, creat_time, modify_time);
}

class AppendFile
{
public:
    const static int kBufferSize = 1024*64;

    explicit AppendFile(StringArg filename);
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
