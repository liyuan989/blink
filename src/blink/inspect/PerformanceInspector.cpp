#include <blink/inspect/PerformanceInspector.h>
#include <blink/ProcessInfo.h>
#include <blink/LogStream.h>
#include <blink/FileTool.h>
#include <blink/CurrentThread.h>

#ifdef HAVE_TCMALLOC

#include <gperftools/malloc_extension.h>
#include <gperftools/profiler.h>

namespace blink
{

void PerformanceInspector::registerCommands(Inspector* inspector)
{
    inspector->add("pprof", "heap", PerformanceInspector::heap, "get heap information");
    inspector->add("pprof", "growth", PerformanceInspector::growth, "get heap growth information");
    inspector->add("pprof", "profile", PerformanceInspector::profile,
                   "get cpu profiling information, CAUTION: blocking thread for 30 seconds!");
    inspector->add("pprof", "cmdline", PerformanceInspector::cmdline, "get command cmdline");
    inspector->add("pprof", "memstats", PerformanceInspector::memstats, "get memory stats");
    inspector->add("pprof", "memhistogram", PerformanceInspector::memhistogram, "get memory histogram");
    inspector->add("pprof", "releasefreememory", PerformanceInspector::releaseFreeMemory, "release free memory");
}

string PerformanceInspector::heap(HttpRequest::Method, const Inspector::ArgList&)
{
    std::string result;
    MallocExtension::instance()->GetHeapSample(&result);
    return string(result.data(), result.size());
}

string PerformanceInspector::growth(HttpRequest::Method, const Inspector::ArgList&)
{
    std::string result;
    MallocExtension::instance()->GetHeapGrowthStacks(&result);
    return string(result.data(), result.size());
}

string PerformanceInspector::profile(HttpRequest::Method, const Inspector::ArgList&)
{
    string filename = "/tmp/" + procName();
    filename += ".";
    filename += pidString();
    filename += ".";
    filename += Timestamp::now().toString();
    filename += ".profile";

    string profile;
    if (ProfilerStart(filename.c_str()))
    {
        // FIXME: async
        sleepMicroseconds(30 * 1000 * 1000);
        ProfilerStop();
        readFile(filename, 1024 * 1024, &profile);
        ::unlink(filename.c_str());
    }
    return profile;
}

string PerformanceInspector::cmdline(HttpRequest::Method, const Inspector::ArgList&)
{
    return "";
}

string PerformanceInspector::memstats(HttpRequest::Method, const Inspector::ArgList&)
{
    char buf[1024 * 64];
    MallocExtension::instance()->GetStats(buf, sizeof(buf));
    return buf;
}

string PerformanceInspector::memhistogram(HttpRequest::Method, const Inspector::ArgList&)
{
    int blocks = 0;
    size_t total = 0;
    int histogram[kMallocHistogramSize] = {0,};
    MallocExtension::instance()->MallocMemoryStats(&blocks, &total, histogram);
    LogStream s;
    s << "blocks " << blocks << "\ntotal " << total << "\n";
    for (int i = 0; i < kMallocHistogramSize; ++i)
    {
        s << i << " " << histogram[i] << "\n";
    }
    return s.buffer().toString();
}

string PerformanceInspector::releaseFreeMemory(HttpRequest::Method, const Inspector::ArgList&)
{
    char buf[256];
    snprintf(buf, sizeof(256), "memory release rate: %f\nAll free memory released.\n",
             MallocExtension::instance()->GetMemoryReleaseRate());
    MallocExtension::instance()->ReleaseFreeMemory();
    return buf;
}

}  // namespace blink

#endif
