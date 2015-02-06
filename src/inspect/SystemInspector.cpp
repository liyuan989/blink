#include "inspect/SystemInspector.h"
#include "inspect/ProcessInspector.h"
#include "FileTool.h"

#include <sys/utsname.h>

namespace blink
{

void SystemInspector::registerCommands(Inspector* inspector)
{
    inspector->add("sys", "overview", SystemInspector::overview, "print system overview");
    inspector->add("sys", "loadavg", SystemInspector::loadavg, "print /proc/loadavg");
    inspector->add("sys", "version", SystemInspector::version, "print /proc/version");
    inspector->add("sys", "cpuinfo", SystemInspector::cpuinfo, "print /proc/cpuinfo");
    inspector->add("sys", "meminfo", SystemInspector::meminfo, "print /proc/meminfo");
    inspector->add("sys", "stat", SystemInspector::stat, "print /proc/stat");
}

// struct utsname
// {
//     char sysname[];       /* Operating system name (e.g., "Linux") */
//     char nodename[];      /* Name within "some implementation-defined
//                              network" */
//     char release[];       /* Operating system release (e.g., "2.6.28") */
//     char version[];       /* Operating system version */
//     char machine[];       /* Hardware identifier */
// #ifdef _GNU_SOURCE
//     char domainname[];    /* NIS or YP domain name */
// #endif
// };

string SystemInspector::overview(HttpRequest::Method, const Inspector::ArgList&)
{
    string result;
    result.resize(1024);
    Timestamp now = Timestamp::now();
    result += "Page generated at ";
    result += now.toFormattedString();
    result += " (UTC)\n";

    // hardware and OS
    {
        struct utsname un;
        if (::uname(&un) == 0)
        {
            stringPrintf(&result, "Hostname: %s", un.nodename);
            stringPrintf(&result, "Machine: %s", un.machine);
            stringPrintf(&result, "OS: %s %s %s\n", un.sysname, un.release, un.version);
        }
    }
    string stat;
    readFile("/proc/stat", 65536, &stat);
    Timestamp boot_time(Timestamp::kMicrosecondsPerSecond * getLong(stat, "btime "));
    result += "Boot time: ";
    result += boot_time.toFormattedString(false);
    result += " (UTC)\n";
    result += "Up time: ";
    result += uptime(now, boot_time, false);
    result += "\n";

    // CPU load
    {
        string loadavg;
        readFile("/proc/loadavg", 65536, &loadavg);
        stringPrintf(&result, "Processes created: %ld\n", getLong(stat, "processes "));
        stringPrintf(&result, "Loadavg: %s\n", loadavg.c_str());
    }

    // Memory
    {
        string meminfo;
        readFile("/proc/meminfo", 65536, &meminfo);
        long total_kb = getLong(meminfo, "MemTotal:");
        long free_kb = getLong(meminfo, "MemFree:");
        long buffers_kb = getLong(meminfo, "Buffers:");
        long cached_kb = getLong(meminfo, "Cached:");

        stringPrintf(&result, "Total Memory: %6ld MiB\n", total_kb / 1024);
        stringPrintf(&result, "Free Memory:  %6ld MiB\n", free_kb / 1024);
        stringPrintf(&result, "Buffers:      %6ld MiB\n", buffers_kb / 1024);
        stringPrintf(&result, "Cached:       %6ld MiB\n", cached_kb / 1024);
        stringPrintf(&result, "Real Used:    %6ld MiB\n", (total_kb - free_kb - buffers_kb - cached_kb) / 1024);
        stringPrintf(&result, "Real Free:    %6ld MiB\n", (free_kb + buffers_kb + cached_kb) / 1024);
    }
    // FIXME: Swap
    // FIXME: Disk
    // FIXME: Network
    return result;
}

string SystemInspector::loadavg(HttpRequest::Method, const Inspector::ArgList&)
{
    string loadavg;
    readFile("/proc/loadavg", 65536, &loadavg);
    return loadavg;
}

string SystemInspector::version(HttpRequest::Method, const Inspector::ArgList&)
{
    string version;
    readFile("/proc/version", 65536, &version);
    return version;
}

string SystemInspector::cpuinfo(HttpRequest::Method, const Inspector::ArgList&)
{
    string cpuinfo;
    readFile("/proc/cpuinfo", 65536, &cpuinfo);
    return cpuinfo;
}

string SystemInspector::meminfo(HttpRequest::Method, const Inspector::ArgList&)
{
    string meminfo;
    readFile("/proc/meminfo", 65536, &meminfo);
    return meminfo;
}

string SystemInspector::stat(HttpRequest::Method, const Inspector::ArgList)
{
    string stat;
    readFile("/proc/stat", 65536, &stat);
    return stat;
}

}  // namespace blink
