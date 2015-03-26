#include <example/procmon/plot.h>

#include <blink/http/HttpResponse.h>
#include <blink/http/HttpRequest.h>
#include <blink/http/HttpServer.h>
#include <blink/ProcessInfo.h>
#include <blink/EventLoop.h>
#include <blink/FileTool.h>

#include <boost/algorithm/string/replace.hpp>
#include <boost/type_traits/is_pod.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/bind.hpp>

#include <sstream>
#include <stdarg.h>
#include <stdio.h>

using namespace blink;

// Represent parsed /proc/pid/stat
struct StatData
{
    //int   pid;
    char  state;
    int   ppid;
    int   pgrp;
    int   session;
    int   tty_nr;
    int   tpgid;
    int   flags;

    long  min_flt;
    long  cmin_flt;
    long  maj_flt;
    long  cmaj_flt;

    long  utime;
    long  stime;
    long  cutime;
    long  cstime;

    long  priority;
    long  nice;
    long  num_threads;
    long  it_real_value;
    long  start_time;

    long  vsize_kb;
    long  rss_kb;
    long  rsslim;

    //            0    1    2    3     4    5       6   7 8 9  11  13   15
    // 3770 (cat) R 3718 3770 3718 34818 3770 4202496 214 0 0 0 0 0 0 0 20
    // 16  18     19      20 21                   22      23      24              25
    //  0 1 0 298215 5750784 81 18446744073709551615 4194304 4242836 140736345340592
    //              26
    // 140736066274232 140575670169216 0 0 0 0 0 0 0 17 0 0 0 0 0 0
    void parse(const char* start_at_state, int kb_per_page)
    {
        // istringstream is probably not the most efficent way to parse it.
        std::istringstream iss(start_at_state);

        iss >> state;
        iss >> ppid >> pgrp >> session >> tty_nr >> tpgid >> flags;
        iss >> min_flt >> cmin_flt >> maj_flt >> cmaj_flt;
        iss >> utime >> stime >> cutime >> cstime;
        iss >> priority >> nice >> num_threads >> it_real_value >> start_time;

        long vsize;
        long rss;
        iss >> vsize >> rss >> rsslim;
        vsize_kb = vsize / 1024;
        rss_kb = rss * kb_per_page;
    }
};

BOOST_STATIC_ASSERT(boost::is_pod<StatData>::value);

class Procmon : Nocopyable
{
public:
    Procmon(EventLoop* loop, pid_t pid, uint16_t port, const char* procname)
        : kClockTicksPerSecond_(clockTicksPerSecond()),
          kb_per_page_(pageSize() / 1024),
          kBootTime_(getBootTime()),
          pid_(pid),
          server_(loop, InetAddress(port), getName()),
          procname_(procName(readProcFile("stat")).asString()),
          hostname_(hostName()),
          cmdline_(getCmdline()),
          ticks_(0),
          cpu_usage_(600 / kPeriod_),  // 10 minutes
          cpu_chart_(600, 100, 600, kPeriod_),
          ram_chart_(640, 100, 7200, 30)
    {
        memset(&last_stat_data_, 0, sizeof(last_stat_data_));
        server_.setHttpCallback(boost::bind(&Procmon::onRequest, this, _1, _2));
    }

    void start()
    {
        tick();
        server_.getLoop()->runEvery(kPeriod_, boost::bind(&Procmon::tick, this));
        server_.start();
    }

private:
    string getName() const
    {
        char name[256];
        snprintf(name, sizeof(name), "procmon-%d", pid_);
        return name;
    }

    void onRequest(const HttpRequest& request, HttpResponse* response)
    {
        response->setStatusCode(HttpResponse::kOk);
        response->setStatusMessage("OK");
        response->setContextType("text/plain");
        response->addHeader("Server", "Blink-Procmon");

        //if (!processExists(pid_))
        //{
        //    response->setStatusCode(HttpResponse::kNotFound);
        //    response->setStatusMessage("Not Found");
        //    response->setCloseConnection(true);
        //    return;
        //}

        if (request.path() == "/")
        {
            response->setContextType("text/html");
            fillOverview(request.query());
            response->setBody(response_.resetAllToString());
        }
        else if (request.path() == "/cmdline")
        {
            response->setBody(cmdline_);
        }
        else if (request.path() == "/cpu.png")
        {
            std::vector<double> cpu_usage;
            for (size_t i = 0; i < cpu_usage_.size(); ++i)
            {
                cpu_usage.push_back(cpu_usage_[i].cpuUsage(kPeriod_, kClockTicksPerSecond_));
            }
            string png = cpu_chart_.plotCpu(cpu_usage);
            response->setContextType("image/png");
            response->setBody(png);
        }
        else if (request.path() == "/environ")  // FIXME: replace with a map
        {
            response->setBody(getEnviron());
        }
        else if (request.path() == "/io")
        {
            response->setBody(readProcFile("io"));
        }
        else if (request.path() == "/limits")
        {
            response->setBody(readProcFile("limits"));
        }
        else if (request.path() == "/maps")
        {
            response->setBody(readProcFile("maps"));
        }
        else if (request.path() == "/smaps")  // numa_maps
        {
            response->setBody(readProcFile("smaps"));
        }
        else if (request.path() == "/status")
        {
            response->setBody(readProcFile("status"));
        }
        else if (request.path() == "threads")
        {
            fillThreads();
            response->setBody(response_.resetAllToString());
        }
        else
        {
            response->setStatusCode(HttpResponse::kNotFound);
            response->setStatusMessage("Not Found");
            response->setCloseConnection(true);
        }
    }

    void fillOverview(const string& query)
    {
        response_.resetAll();
        Timestamp now = Timestamp::now();
        appendReponse("<html><head><title>%s on %s</title>\n",
                      procname_.c_str(), hostname_.c_str());
        fillRefresh(query);
        appendReponse("</head><body>\n");

        string stat = readProcFile("stat");
        if (stat.empty())
        {
            appendReponse("<h1>PID %d doesn't exist.</h1></body></html>", pid_);
            return;
        }
        int pid = atoi(stat.c_str());
        assert(pid == pid_);
        StringPiece procname = procName(stat);
        appendReponse("<h1>%s on %s</h1>\n",
                      procname.asString().c_str(), hostname_.c_str());
        response_.append("<p>Refresh <a href=\"?refresh=1\">1s</a> ");
        response_.append("<a href=\"?refresh=2\">2s</a> ");
        response_.append("<a href=\"?refresh=5\">5s</a> ");
        response_.append("<a href=\"?refresh=15\">15s</a> ");
        response_.append("<a href=\"?refresh=60\">60s</a>\n");
        response_.append("<p><a href=\"/cmdline\">Command line</a>\n");
        response_.append("<a href=\"/environ\">Environment variables</a>\n");
        response_.append("<a href=\"/threads\">Threads</a>\n");

        appendReponse("<p>Page generated at %s (UTC)", now.toFormattedString().c_str());

        response_.append("<p><table>");
        StatData stat_data;  // how about use last_stat_data ?
        memset(&stat_data, 0, sizeof(stat_data));
        stat_data.parse(procname.end() + 1, kb_per_page_);  // end is ')'

        appendTableRow("PID", pid);
        Timestamp started(getStartTime(stat_data.start_time));  // FIXME: cache it
        appendTableRow("Started at", started.toFormattedString(false /* show_microseconds */) + " (UTC)");
        appendTableRowFloat("Uptime (s)", timeDifference(now, started));  // FIXME: format as days+H:M:S
        appendTableRow("Executable", readLink("exe"));
        appendTableRow("Current dir", readLink("cwd"));

        appendTableRow("State", getState(stat_data.state));
        appendTableRowFloat("User time (s)", getSeconds(stat_data.utime));
        appendTableRowFloat("System time (s)", getSeconds(stat_data.stime));

        appendTableRow("VmSize (KiB)", stat_data.vsize_kb);
        appendTableRow("VmRSS (KiB)", stat_data.rss_kb);
        appendTableRow("Threads", stat_data.num_threads);
        appendTableRow("CPU usage", "<img src=\"/cpu.png\" height=\"100\" width=\"640\">");

        appendTableRow("Priority", stat_data.priority);
        appendTableRow("Nice", stat_data.nice);

        appendTableRow("Minor page faults", stat_data.min_flt);
        appendTableRow("Major page faults", stat_data.maj_flt);
        // TODO: user

        response_.append("</table>");
        response_.append("</body></html>");
    }

    void fillRefresh(const string& query)
    {
        size_t p = query.find("refresh=");
        if (p != string::npos)
        {
            int seconds = atoi(query.c_str() + p + 8);
            if (seconds > 0)
            {
                appendReponse("<meta http-equiv=\"refresh\" content=\"%d\">\n", seconds);
            }
        }
    }

    void fillThreads()
    {
        response_.resetAll();
        // FIXME: implement this
    }

    string readProcFile(const char* basename)
    {
        char filename[256];
        snprintf(filename, sizeof(filename), "/proc/%d/%s", pid_, basename);
        string content;
        readFile(filename, 1024 * 1024, &content);
        return content;
    }

    string readLink(const char* basename)
    {
        char filename[256];
        snprintf(filename, sizeof(filename), "/proc/%d/%s", pid_, basename);
        char link[1024];
        ssize_t len = ::readlink(filename, link, sizeof(link));
        string result;
        if (len > 0)
        {
            result.assign(link, len);
        }
        return result;
    }

    int appendReponse(const char* fmt, ...) __attribute__ ((format (printf, 2, 3)));

    void appendTableRow(const char* name, long value)
    {
        appendReponse("<tr><td>%s</td><td>%ld</td></tr>\n", name, value);
    }

    void appendTableRowFloat(const char* name, double value)
    {
        appendReponse("<tr><td>%s</td><td>%.2f</td></tr>\n", name, value);
    }

    void appendTableRow(const char* name, StringArg value)
    {
        appendReponse("<tr><td>%s</td><td>%s</td></tr>\n", name, value.c_str());
    }

    string getCmdline()
    {
        return boost::replace_all_copy(readProcFile("cmdline"), string(1, '\0'), "\n\t");
    }

    string getEnviron()
    {
        return boost::replace_all_copy(readProcFile("environ"), string(1, '\0'), "\n");
    }

    Timestamp getStartTime(long start_time)
    {
        return Timestamp(Timestamp::kMicrosecondsPerSecond * kBootTime_ +
                         Timestamp::kMicrosecondsPerSecond * start_time / kClockTicksPerSecond_);
    }

    double getSeconds(long ticks)
    {
        return static_cast<double>(ticks) / kClockTicksPerSecond_;
    }

    void tick()
    {
        string stat = readProcFile("stat");  // FIXME: cache file decriptor
        if (stat.empty())
        {
            return;
        }
        StringPiece procname = procName(stat);
        StatData stat_data;
        memset(&stat_data, 0, sizeof(stat_data));
        stat_data.parse(procname.end() + 1, kb_per_page_);  // end is ')'
        if (ticks_ > 0)
        {
            CpuTime time;
            time.user_time = std::max(0, static_cast<int>(stat_data.utime - last_stat_data_.utime));
            time.sys_time = std::max(0, static_cast<int>(stat_data.stime - last_stat_data_.stime));
            cpu_usage_.push_back(time);
        }
        last_stat_data_ = stat_data;
        ++ticks_;
    }

    // ------------------ static member functions ------------------

    static const char* getState(char state)
    {
        switch (state)
        {
            case 'R':
                return "Running";
            case 'S':
                return "Sleeping";
            case 'D':
                return "Disk sleep";
            case 'Z':
                return "Zombie";
            default:
                return "Unknown";
        }
    }

    static long getLong(const string& status, const char* key)
    {
        long result = 0;
        size_t pos = status.find(key);
        if (pos != string::npos)
        {
            result = atoi(status.c_str() + pos + strlen(key));
        }
        return result;
    }

    static long getBootTime()
    {
        string stat;
        readFile("/proc/stat", 65536, &stat);
        return getLong(stat, "btime ");
    }

    // ------------------ data member -------------------------------

    struct CpuTime
    {
        int user_time;
        int sys_time;

        double cpuUsage(double kPeriod, double kClockTicksPerSecond) const
        {
            return (user_time + sys_time) / (kClockTicksPerSecond * kPeriod);
        }
    };

    static const int                 kPeriod_ = 2.0;

    const int                        kClockTicksPerSecond_;
    const int                        kb_per_page_;
    const long                       kBootTime_;
    const pid_t                      pid_;
    HttpServer                       server_;
    const string                     procname_;
    const string                     hostname_;
    const string                     cmdline_;
    int                              ticks_;
    StatData                         last_stat_data_;
    boost::circular_buffer<CpuTime>  cpu_usage_;
    Plot                             cpu_chart_;
    Plot                             ram_chart_;
    Buffer                           response_;
};

const int Procmon::kPeriod_;

// define out for __attribute__
int Procmon::appendReponse(const char* fmt, ...)
{
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    int ret = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    response_.append(buf);
    return ret;
}

bool processExists(pid_t pid)
{
    char filename[256];
    snprintf(filename, sizeof(filename), "/proc/%d/stat", pid);
    return ::access(filename, R_OK) == 0;
}

int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s <port> <pid> [name]\n", argv[0]);
        return 1;
    }
    int pid = atoi(argv[2]);
    if (!processExists(pid))
    {
        fprintf(stderr, "Process %d doesn't exist.\n", pid);
        return 2;
    }
    uint16_t port = static_cast<uint16_t>(atoi(argv[1]));
    EventLoop loop;
    Procmon procmon(&loop, pid, port, argc > 3 ? argv[3] : "");
    procmon.start();
    loop.loop();
    return 0;
}
