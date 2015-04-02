#include <blink/ProcessInfo.h>

#include <stdio.h>

using namespace blink;
using namespace blink::process_info;

int main(int argc, char const *argv[])
{
    printf("pidString : %s\n", pidString().c_str());
    printf("usrname : %s\n", username().c_str());
    printf("startTime : %s\n", startTime().toString().c_str());
    printf("startTime(format) : %s\n", startTime().toFormattedString().c_str());
    printf("clockTicksPerSecond : %d\n", clockTicksPerSecond());
    printf("pageSize : %d\n", pageSize());
    printf("isDebugBuild : %s\n", (isDebugBuild() ? "true" : "false"));
    printf("hostName : %s\n", hostName().c_str());
    printf("procName : %s\n", procName().c_str());
    printf("procStatus : %s\n", procStatus().c_str());
    printf("procStat : %s\n", procStat().c_str());
    printf("threadStat : %s\n", threadStat().c_str());
    printf("exePath: %s\n", exePath().c_str());
    printf("openedFiles : %d\n", openedFiles());
    printf("maxOpenFiles : %d\n", maxOpenFiles());
    printf("cpuTime: user_seconds = %.12g system_seconds = %.12g\n",
           cpuTime().user_seconds, cpuTime().system_seconds);
    printf("threadsNumber : %d\n", threadsNumber());
    printf("threads: ");
    std::vector<pid_t> vec = threads();
    for (size_t i = 0; i < vec.size(); ++i)
    {
        printf("%d\n", vec[i]);
    }
    return 0;
}
