#include <blink/ProcessInfo.h>
#include <blink/ProcessBase.h>
#include <blink/CurrentThread.h>
#include <blink/Timestamp.h>
#include <blink/FileTool.h>

#include <sys/types.h>
#include <sys/times.h>
#include <sys/resource.h>
#include <dirent.h>
#include <pwd.h>
#include <unistd.h>

#include <algorithm>
#include <vector>
#include <time.h>
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

namespace blink
{

__thread int t_opened_file_num = 0;

//  struct dirent
//  {
//      ino_t  d_ino;                 /* i-node number */
//      char   d_name[NAME_MAX + 1];  /* null-terminated filename */
//  };

int fdDirFilter(const struct dirent* dir)
{
    if (::isdigit(dir->d_name[0]))
    {
        ++t_opened_file_num;
    }
    return 0;
}

__thread std::vector<pid_t>* t_pids = NULL;

int taskDirFilter(const struct dirent* dir)
{
    if (::isdigit(dir->d_name[0]))
    {
        t_pids->push_back(::atoi(dir->d_name));
    }
    return 0;
}

int scanDir(const char* dirpath, int (*filter)(const struct dirent*))
{
    struct dirent** namelist = NULL;
    int result = ::scandir(dirpath, &namelist, filter, alphasort);
    assert(namelist == NULL);
    return result;
}

Timestamp g_startTime = Timestamp::now();
int       g_clockTicks = static_cast<int>(::sysconf(_SC_CLK_TCK));
int       g_pageSize = static_cast<int>(::sysconf(_SC_PAGESIZE));

pid_t pid()
{
    return processes::getpid();
}

string pidString()
{
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", processes::getpid());
    return buf;
}

//  struct passwd
//  {
//      char   *pw_name;       /* username */
//      char   *pw_passwd;     /* user password */
//      uid_t   pw_uid;        /* user ID */
//      gid_t   pw_gid;        /* group ID */
//      char   *pw_gecos;      /* user information */
//      char   *pw_dir;        /* home directory */
//      char   *pw_shell;      /* shell program */
//  };

string username()
{
    struct passwd pwd;
    struct passwd* result = NULL;
    char buf[8192];
    const char* name = "unknownuer";
    getpwuid_r(processes::getuid(), &pwd, buf, sizeof(buf), &result);
    if (result)
    {
        name = pwd.pw_name;
    }
    return name;
}

Timestamp startTime()
{
    return g_startTime;
}

int clockTicksPerSecond()
{
    return g_clockTicks;
}

int pageSize()
{
    return g_pageSize;
}

bool isDebugBuild()
{
#ifndef NDEBUG
    return false;
#else
    return true;
#endif
}

string hostName()
{
    char buf[256];
    if (::gethostname(buf, sizeof(buf)) == 0)
    {
        buf[sizeof(buf) - 1] = '\0';
        return buf;
    }
    else
    {
        return "unknownhost";
    }
}

string procName()
{
    return procName(procStat()).asString();
}

StringPiece procName(const string& stat)
{
    StringPiece name;
    size_t left_pos = stat.find('(');
    size_t right_pos = stat.rfind(')');
    if (left_pos != string::npos && right_pos != string::npos && left_pos < right_pos)
    {
        size_t len = right_pos - (left_pos + 1);
        name.set(stat.data() + left_pos + 1, len);
    }
    return name;
}

string procStatus()
{
    string result;
    readFile("/proc/self/status", 65536, &result);
    return result;
}

string procStat()
{
    string result;
    readFile("proc/self/stat", 65536, &result);
    return result;
}

string threadStat()
{
    char buf[64];
    snprintf(buf, sizeof(buf), "/proc/self/task/%d/stat", tid());
    string result;
    readFile(buf, 65536, &result);
    return result;
}

string exePath()
{
    string result;
    char buf[1024];
    ssize_t n = ::readlink("/proc/self/exe", buf, sizeof(buf));
    if (n > 0)
    {
        result.assign(buf, n);
    }
    return result;
}

int openedFiles()
{
    t_opened_file_num = 0;
    scanDir("/proc/self/fd", fdDirFilter);
    return t_opened_file_num;
}

//  struct rlimit
//  {
//      rlim_t rlim_cur;    /* Soft limit */       /* rlim_t may equal to int,
//      rlim_t rlim_max;    /* Hard limit (ceiling for rlim_cur) */
//  };

int maxOpenFiles()
{
    struct rlimit r_limit;
    if (::getrlimit(RLIMIT_NOFILE, &r_limit) == 0)
    {
        return static_cast<int>(r_limit.rlim_cur);
    }
    else
    {
        return openedFiles();
    }
}

//  struct tms
//  {
//      clock_t tms_utime;    /* user time */
//      clock_t tms_stime;    /* system time */
//      clock_t tms_cutime;   /* user time of children */
//      clock_t tms_cstime;   /* system time of children */
//  };

CpuTime cpuTime()
{
    CpuTime cpu_time;
    struct tms tms_time;
    if (::times(&tms_time) >= 0)
    {
        const double clock_ticks_per_second = clockTicksPerSecond();
        cpu_time.user_seconds = static_cast<double>(tms_time.tms_utime) / clock_ticks_per_second;
        cpu_time.system_seconds = static_cast<double>(tms_time.tms_stime) / clock_ticks_per_second;
    }
    return cpu_time;
}

int threadsNumber()
{
    int result = 0;
    string status = procStatus();
    size_t pos = status.find("Threads:");
    if (pos != string::npos)
    {
        result = atoi(status.c_str() + pos + 8);
    }
    return result;
}

std::vector<pid_t> threads()
{
    std::vector<pid_t> result;
    t_pids = &result;
    scanDir("/proc/self/task", taskDirFilter);
    t_pids = NULL;
    return result;
}

}  // namespace blink
