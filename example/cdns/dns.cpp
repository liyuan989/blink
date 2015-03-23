#include <example/cdns/Resolver.h>

#include <blink/EventLoop.h>

#include <boost/bind.hpp>

#include <stdio.h>

using namespace blink;

EventLoop* g_loop = NULL;
int g_count = 0;
int g_total = 0;

void quit()
{
    g_loop->quit();
}

void resolveCallback(const string& hostname, const InetAddress& addr)
{
    printf("resolveCallback: %s -> %s\n", hostname.c_str(), addr.toIp().c_str());
    if (++g_count == g_total)
    {
        quit();
    }
}

void resolve(Resolver* resolver, const string& hostname)
{
    resolver->resolve(hostname, boost::bind(resolveCallback, hostname, _1));
}

int main(int argc, char* argv[])
{
    EventLoop loop;
    loop.runAfter(10, quit);
    g_loop = &loop;
    Resolver resolver(&loop, argc == 1 ? Resolver::kDnsOnly : Resolver::kDnsAndHostFile);
    if (argc == 1)
    {
        g_total = 3;
        resolve(&resolver, "liyuanlife.com");
        resolve(&resolver, "github.com");
        resolve(&resolver, "stackoverflow.com");
    }
    else
    {
        g_total = argc - 1;
        for (int i = 1; i < argc; ++i)
        {
            resolve(&resolver, argv[i]);
        }
    }
    loop.loop();
    return 0;
}
