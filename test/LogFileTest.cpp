#include "LogFile.h"
#include "Log.h"
#include "TimeZone.h"

#include <boost/scoped_ptr.hpp>

#include <unistd.h>

#include <string>
#include <stdio.h>

using namespace blink;

boost::scoped_ptr<LogFile> logfile;

void outputFunc(const char* message, size_t len)
{
    logfile->append(message, len);
}

void flushFunc()
{
    logfile->flush();
}

int main(int argc, char *argv[])
{
    char buf[256];
    snprintf(buf, sizeof (buf), "%s", argv[0]);
    LogFile::setTimeZone(TimeZone(3600 * 8, "CST"));
    logfile.reset(new LogFile(::basename(buf), 1024*200));
    Log::setOutput(outputFunc);
    Log::setFlush(flushFunc);

    std::string s = "1234567890 QWERTYUUIOPASDFGHJKLZXCVBN sadasdqwewqsadadasd";
    for (int i = 0; i < 1024 * 10; ++i)
    {
        LOG_INFO << s;
        usleep(1000);
    }
    return 0;
}
