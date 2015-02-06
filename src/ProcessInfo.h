#ifndef __BLINK_PROCESSINFO_H__
#define __BLINK_PROCESSINFO_H__

#include "Timestamp.h"
#include "Types.h"
#include "StringPiece.h"

#include <vector>

namespace blink
{

pid_t pid();
string pidString();
string username();
Timestamp startTime();
int clockTicksPerSecond();
int pageSize();
bool isDebugBuild();
string hostName();
string procName();
StringPiece procName(const string& stat);

// read from /proc/self/status
string procStatus();

// read from /proc/self/status
string procStat();

// read from /proc/self/task/tid/stat
string threadStat();

// read from /proc/self/exe
string exePath();
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
