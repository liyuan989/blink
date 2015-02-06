#ifndef __BLINK_INSPECT_PERFORMANCEINSPECTOR_H__
#define __BLINK_INSPECT_PERFORMANCEINSPECTOR_H__

#include "inspect/Inspector.h"

namespace blink
{

class PerformanceInspector : Nocopyable
{
public:
    void registerCommands(Inspector* inspector);

    static string heap(HttpRequest::Method, const Inspector::ArgList&);
    static string growth(HttpRequest::Method, const Inspector::ArgList&);
    static string profile(HttpRequest::Method, const Inspector::ArgList&);
    static string cmdline(HttpRequest::Method, const Inspector::ArgList&);
    static string memstats(HttpRequest::Method, const Inspector::ArgList&);
    static string memhistogram(HttpRequest::Method, const Inspector::ArgList&);
    static string symbol(HttpRequest::Method, const Inspector::ArgList&);
};

}  // namespace blink

#endif
