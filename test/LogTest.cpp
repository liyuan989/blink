#include <blink/Log.h>

#include <string>
#include <stdlib.h>
#include <stdio.h>

using namespace blink;

int main(int argc, char const *argv[])
{
    //Log::setLogLevel(Log::TRACE);
    LOG_TRACE << "hey!";
    LOG_DEBUG << "GO! GO! GO!";
    LOG_INFO << "hello, world";
    LOG_WARN << "HOW ARE YOU";
    LOG_ERROR << "ERROR!";

    char* p = NULL;
    CHECK_NOTNULL(p);
    return 0;
}
