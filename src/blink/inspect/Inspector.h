#ifndef __BLINK_INSPECT_INSPECTOR_H__
#define __BLINK_INSPECT_INSPECTOR_H__

#include <blink/http/HttpServer.h>
#include <blink/http/HttpRequest.h>
#include <blink/MutexLock.h>
#include <blink/Nocopyable.h>

#include <boost/function.hpp>
#include <boost/scoped_ptr.hpp>

#include <map>

namespace blink
{

class ProcessInspector;
class PerformanceInspector;
class SystemInspector;

// An internal inspector of the running process, usually a singleton.
// Better to run a separated thread, as some method may block for seconds.
class Inspector : Nocopyable
{
public:
    typedef std::vector<string> ArgList;
    typedef boost::function<string (HttpRequest::Method, const ArgList&)> Callback;

    Inspector(EventLoop* loop, const InetAddress& http_addr, const string& name);
    ~Inspector();

    void add(const string& module,
             const string& command,
             const Callback& cb,
             const string& help);
    void remove(const string& module, const string& command);

private:
    void start();
    void onRequest(const HttpRequest& request, HttpResponse* response);

    typedef std::map<string, Callback> CommandList;
    typedef std::map<string, string> HelpList;

    HttpServer                               server_;
    boost::scoped_ptr<ProcessInspector>      process_inspector_;
    boost::scoped_ptr<PerformanceInspector>  performance_inspector_;
    boost::scoped_ptr<SystemInspector>       system_inspector_;
    MutexLock                                mutex_;
    std::map<string, CommandList>            modules_;
    std::map<string, HelpList>               helps_;
};

}  // namespace blink

#endif
