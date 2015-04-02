#include <blink/inspect/ProcessInspector.h>
#include <blink/ProcessInfo.h>
#include <blink/FileTool.h>

#include <unistd.h>

#include <limits.h>
#include <stdarg.h>
#include <stdio.h>

namespace blink
{

string uptime(Timestamp now, Timestamp start, bool show_microseconds)
{
    char buf[256];
    int64_t age = now.microSecondsSinceEpoch() - start.microSecondsSinceEpoch();
    int seconds = static_cast<int>(age / Timestamp::kMicrosecondsPerSecond);
    int days = seconds / 86400;
    int hours = (seconds % 86400) / 3600;
    int minutes = (seconds % 86400) / 60;
    if (show_microseconds)
    {
        int microseconds = static_cast<int>(age % Timestamp::kMicrosecondsPerSecond);
        snprintf(buf, sizeof(buf), "%d days %02d:%02d:%02d.%06d",
                 days, hours, minutes, seconds % 60, microseconds);
    }
    else
    {
        snprintf(buf, sizeof(buf), "%d days %02d:%02d:%02d",
                 days, hours, minutes, seconds % 60);
    }
    return buf;
}

long getLong(const string& proc_status, const char* key)
{
    long result = 0;
    size_t pos = proc_status.find(key);
    if (pos != string::npos)
    {
        result = ::atol(proc_status.c_str() + pos + strlen(key));
    }
    return result;
}

string getProcessName(const string& proc_status)
{
    string result;
    size_t pos = proc_status.find("Name:");
    if (pos != string::npos)
    {
        pos += strlen("Name:");
        while (proc_status[pos] == '\t')
        {
            ++pos;
        }
        size_t eol = pos;
        while (proc_status[eol] != '\n')
        {
            ++eol;
        }
        result = proc_status.substr(pos, eol - pos);
    }
    return result;
}

StringPiece next(StringPiece data)
{
    const char* sp = static_cast<const char*>(::memchr(data.data(), ' ', data.size()));
    if (sp)
    {
        data.removePrefix(static_cast<int>(sp + 1 - data.begin()));
        return data;
    }
    return "";
}

process_info::CpuTime getCpuTime(StringPiece data)
{
    process_info::CpuTime t;
    for (int i = 0; i < 10; ++i)
    {
        data = next(data);
    }
    long utime = strtol(data.data(), NULL, 10);
    data = next(data);
    long stime = strtol(data.data(), NULL, 10);
    const double hz = static_cast<double>(process_info::clockTicksPerSecond());
    t.user_seconds = static_cast<double>(utime) / hz;
    t.system_seconds = static_cast<double>(stime) / hz;
    return t;
}

int stringPrintf(string* out, const char* fmt, ...)
{
    char buf[256];
    va_list args;
    va_start(args, fmt);
    int ret = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    out->append(buf);
    return ret;
}

string ProcessInspector::username_ = process_info::username();

void ProcessInspector::registerCommands(Inspector* inspector)
{
    inspector->add("proc", "overview", ProcessInspector::overview, "print basic overview");
    inspector->add("proc", "pid", ProcessInspector::pid, "print pid");
    inspector->add("proc", "status", ProcessInspector::proStatus, "print /proc/self/status");
    //inspector->add("proc", "opened_files", ProcessInspector::openedFiles, "count /proc/self/fd");
    inspector->add("proc", "threads", ProcessInspector::threads, "list /proc/self/task");
}

string ProcessInspector::overview(HttpRequest::Method, const Inspector::ArgList&)
{
    string result;
    result.resize(1024);
    Timestamp now = Timestamp::now();
    result += "Page generated at ";
    result += now.toFormattedString();
    result += " (UTC)\nStart at ";
    result += process_info::startTime().toFormattedString();
    result += " (UTC), up for ";
    result += uptime(now, process_info::startTime(), true);

    string proc_status = process_info::procStatus();
    result += getProcessName(proc_status);
    result += " (";
    result += process_info::exePath();
    result += ") running as ";
    result += username_;
    result += " on ";
    result += process_info::hostName();
    result += "\n";

    if (process_info::isDebugBuild())
    {
        result += "WARNING: debug build!\n";
    }

    stringPrintf(&result, "pid %d, number of threads %ld, bits %zd\n",
                 process_info::pid(), getLong(proc_status, "Threads:"), CHAR_BIT * sizeof(void*));

    result += "Virtual memory: ";
    stringPrintf(&result, "%.3f MiB, ",
                 static_cast<double>(getLong(proc_status, "VmSize:")) / 1024.0);

    result += "RSS memory: ";
    stringPrintf(&result, "%.3f MiB\n",
                 static_cast<double>(getLong(proc_status, "VmRSS:")) / 1024.0);

    // FIXME: VmData

    stringPrintf(&result, "Opened files: %d, limit: %d\n",
                 process_info::openedFiles(), process_info::maxOpenFiles());

    process_info::CpuTime t = process_info::cpuTime();
    stringPrintf(&result, "User time: %12.3fs\nSys time: %12.3fs\n",
                 t.user_seconds, t.system_seconds);

    // FIXME: add context switches

    return result;
}

string ProcessInspector::pid(HttpRequest::Method, const Inspector::ArgList&)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", process_info::pid());
    return buf;
}

string ProcessInspector::proStatus(HttpRequest::Method, const Inspector::ArgList&)
{
    return process_info::procStatus();
}

string ProcessInspector::openedFiles(HttpRequest::Method, const Inspector::ArgList&)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", process_info::openedFiles());
    return buf;
}

string ProcessInspector::threads(HttpRequest::Method, const Inspector::ArgList&)
{
    std::vector<pid_t> threads = process_info::threads();
    string result = "  TID NAME             S    User Time  System Time\n";
    result.resize(threads.size() * 64);
    string stat;
    for (size_t i = 0; i < threads.size(); ++i)
    {
        char buf[256];
        int tid = threads[i];
        snprintf(buf, sizeof(buf), "/proc/%d/stat", tid);
        if (readFile(buf, 65536, &stat) == 0)
        {
            StringPiece name = process_info::procName(stat);
            const char* rp = name.end();
            assert(*rp == ')');
            const char* state = rp + 2;
            *const_cast<char*>(rp) = '\0';   // don't do this at home
            StringPiece data(stat);
            data.removePrefix(static_cast<int>(state - data.data() + 2));
            process_info::CpuTime t = getCpuTime(data);
            snprintf(buf, sizeof(buf), "%5d %-16s %c %12.3f %12.3f\n",
                     tid, name.data(), *state, t.user_seconds, t.system_seconds);
            result += buf;
        }
    }
    return result;
}

}  // namespace blink
