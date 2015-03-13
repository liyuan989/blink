#ifndef __BLINK_INSPECT_PROCESSINSPECTOR_H__
#define __BLINK_INSPECT_PROCESSINSPECTOR_H__

#include <blink/inspect/Inspector.h>

namespace blink
{

class ProcessInspector : Nocopyable
{
public:
    void registerCommands(Inspector* inspector);

    static string overview(HttpRequest::Method, const Inspector::ArgList&);
    static string pid(HttpRequest::Method, const Inspector::ArgList&);
    static string proStatus(HttpRequest::Method, const Inspector::ArgList&);
    static string openedFiles(HttpRequest::Method, const Inspector::ArgList&);
    static string threads(HttpRequest::Method, const Inspector::ArgList&);

    static string username_;
};

string uptime(Timestamp now, Timestamp start, bool show_microseconds);
int stringPrintf(string* out, const char* fmt, ...) __attribute__((format(printf, 2, 3)));
long getLong(const string& proc_status, const char* key);

}  // namespace blink

#endif
