#include <blink/FileTool.h>

#include <stdint.h>
#include <string.h>
#include <stdio.h>

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

using namespace blink;

int main(int argc, char const *argv[])
{
    const char* s = "hey, make it happy!";
    string str;
    ReadSmallFile read_file(string("1.txt"));
    int n;
    read_file.readToBuffer(&n);
    int64_t filesize;
    int64_t creat_time;
    int64_t modify_time;
    read_file.readToString(1024*64, &str, &filesize, &creat_time, &modify_time);
    printf("str = %s\n", str.c_str());
    printf("buf = %s\n", read_file.buffer());
    printf("filesize = %" PRIu64 "\n", filesize);
    printf("creat_time = %" PRIu64 "\n", creat_time);
    printf("modify_time = %" PRIu64 "\n", modify_time);
    printf("readbytes = %d\n", n);

    AppendFile append_file(string("1.txt"));
    append_file.appendFile(s, strlen(s));
    append_file.flush();
    printf("writebytes = %zd\n", append_file.writtenBytes());
    return 0;
}
