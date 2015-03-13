#include <example/memcached/server/MemcacheServer.h>

#include <blink/inspect/Inspector.h>
#include <blink/EventLoopThread.h>
#include <blink/EventLoop.h>

#include <boost/program_options.hpp>

#include <iostream>

namespace po = boost::program_options;
using namespace blink;

bool parseCommandLine(int argc, char* argv[], MemcacheServer::Options* options)
{
    options->tcp_port = 11211;
    options->gperf_port = 11212;
    options->threads = 4;

    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "Help")
        ("port,p", po::value<uint16_t>(&options->tcp_port), "TCP port")
        ("udpport,U", po::value<uint16_t>(&options->udp_port), "UDP port")
        ("gperf,g", po::value<uint16_t>(&options->gperf_port), "port for gperftools")
        ("threads,t", po::value<int>(&options->threads), "Number of worker threads")
    ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);
    if (vm.count("help"))
    {
        std::cout << "blink-memcached 1.0\n" << desc;
        return false;
    }
    return true;
}

int main(int argc, char *argv[])
{
    EventLoop loop;
    EventLoopThread inspect_thread;
    MemcacheServer::Options options;
    if (parseCommandLine(argc, argv, &options))
    {
        // FIXME: how to destruct it safely?
        new Inspector(inspect_thread.startLoop(), InetAddress(options.gperf_port), "memcache-debug");
        MemcacheServer server(&loop, options);
        server.setThreadNumber(options.threads);
        server.start();
        loop.loop();
    }
    return 0;
}
