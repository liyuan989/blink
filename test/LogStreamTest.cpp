#include "LogStream.h"

#include <stdio.h>

using namespace blink;

int main(int argc, char const *argv[])
{
    LogStream logstream;
    char* p = NULL;
    logstream << p;
    string s = logstream.buffer().toString();
    printf("%s\n", s.c_str());
    logstream.reset();

    logstream << 201412201415;
    s = logstream.buffer().toString();
    printf("%s\n", s.c_str());
    logstream.reset();

    logstream << Format("2014: %lld", 20141220520);
    s = logstream.buffer().toString();
    printf("%s\n", s.c_str());

    Format fmt("2014-12-20: %lld", 201412205220);
    s = fmt.data();
    printf("%s\n", s.c_str());

    return 0;
}
