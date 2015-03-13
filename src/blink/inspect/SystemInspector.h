#ifndef __BLINK_INSPECT_SYSTEMINSPECTOR_H__
#define __BLINK_INSPECT_SYSTEMINSPECTOR_H__

#include <blink/inspect/Inspector.h>

namespace blink
{

class SystemInspector : Nocopyable
{
public:
    void registerCommands(Inspector* inspector);

    static string overview(HttpRequest::Method, const Inspector::ArgList&);
    static string loadavg(HttpRequest::Method, const Inspector::ArgList&);
    static string version(HttpRequest::Method, const Inspector::ArgList&);
    static string cpuinfo(HttpRequest::Method, const Inspector::ArgList&);
    static string meminfo(HttpRequest::Method, const Inspector::ArgList&);
    static string stat(HttpRequest::Method, const Inspector::ArgList);
};

}  // namespace blink

#endif
