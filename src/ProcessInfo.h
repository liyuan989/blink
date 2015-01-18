#ifndef __BLINK_PROCESSINFO_H__
#define __BLINK_PROCESSINFO_H__

#include "Timestamp.h"

#include <string>
#include <vector>

namespace blink
{

std::string pidString();
std::string username();
Timestamp startTime();
int clockTicksPerSecond();
int pageSize();
bool isDebugBuild();
std::string hostName();
std::string procName();
std::string procStatus();  // read from /proc/self/status
std::string procStat();    // read from /proc/self/stat
std::string threadStat();  // read from /proc/self/task/tid/stat
std::string exePath();     // read from /proc/self/exe
int openedFiles();
int maxOpenFiles();

struct CpuTime
{
	double  user_seconds;
	double  system_seconds;

	CpuTime()
		: user_seconds(0), system_seconds(0)
	{
	}
};

CpuTime cpuTime();
int threadsNumber();
std::vector<pid_t> threads();

}  // namespace blink

#endif
