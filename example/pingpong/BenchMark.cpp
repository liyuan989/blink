#include "EventLoop.h"
#include "Channel.h"
#include "Thread.h"
#include "Log.h"

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/bind.hpp>

#include <sys/socket.h>
#include <sys/resource.h>
#include <unistd.h>

#include <vector>
#include <stdio.h>

using namespace blink;

std::vector<int>            g_pipes;
int                         g_numPipes;
int                         g_numActive;
int                         g_numWrites;
EventLoop*                  g_loop;
boost::ptr_vector<Channel>  g_channels;
int                         g_reads;
int                         g_writes;
int                         g_fired;

void readCallback(Timestamp receive_time, int fd, int index)
{
    char ch;
    g_reads += static_cast<int>(::recv(fd, &ch, sizeof(ch), 0));
    if (g_writes > 0)
    {
        int w_index = index + 1;
        if (w_index >= g_numPipes)
        {
            w_index -= g_numPipes;
        }
        ::send(g_pipes[2 * w_index + 1], "m", 1, 0);
        --g_writes;
        ++g_fired;
    }
    if (g_fired == g_reads)
    {
        g_loop->quit();
    }
}

std::pair<int, int> runOnce()
{
    Timestamp before_init(Timestamp::now());
    for (int i = 0; i < g_numPipes; ++i)
    {
        Channel& channel = g_channels[i];
        channel.setReadCallback(boost::bind(readCallback, _1, channel.fd(), i));
        channel.enableReading();
    }
    int space = g_numPipes / g_numActive;
    space *= 2;
    for (int i = 0; i < g_numActive; ++i)
    {
        send(g_pipes[i * space + 1], "m", 1, 0);
    }
    g_fired = g_numActive;
    g_reads = 0;
    g_writes = g_numWrites;
    Timestamp before_loop(Timestamp::now());
    g_loop->loop();
    Timestamp end(Timestamp::now());
    int iter_time = static_cast<int>(end.microSecondsSinceEpoch() - before_init.microSecondsSinceEpoch());
    int loop_time = static_cast<int>(end.microSecondsSinceEpoch() - before_loop.microSecondsSinceEpoch());
    return std::make_pair(iter_time, loop_time);
}

int main(int argc, char* argv[])
{
    g_numPipes = 100;
    g_numActive = 1;
    g_numWrites = 100;
    int x;
    while ((x = getopt(argc, argv, "n:a:w")) != -1)
    {
        switch(x)
        {
            case 'n':
                g_numPipes = atoi(optarg);
                break;
            case 'a':
                g_numActive = atoi(optarg);
                break;
            case 'w':
                g_numWrites = atoi(optarg);
                break;
            default:
                fprintf(stderr, "Invalid argument \"%c\"\n", x);
                return 1;
        }
    }
    struct rlimit rl;
    rl.rlim_cur = g_numPipes * 2 + 50;
    rl.rlim_max = g_numPipes * 2 + 50;
    if (::setrlimit(RLIMIT_NOFILE, &rl) == -1)
    {
        perror("setrlimit");
        return 1;
    }
    g_pipes.resize(2 * g_numPipes);
    for (int i = 0; i < g_numPipes; ++i)
    {
        if (::socketpair(AF_UNIX, SOCK_STREAM, 0, &g_pipes[i * 2]) == -1)
        {
            perror("pipe");
            return 1;
        }
    }
    EventLoop loop;
    g_loop = &loop;
    for (int i = 0; i < g_numPipes; ++i)
    {
        Channel* channel = new Channel(&loop, g_pipes[i * 2]);
        g_channels.push_back(channel);
    }
    for (int i = 0; i < 25; ++i)
    {
        std::pair<int, int> t = runOnce();
        printf("%-8d %-8d\n", t.first, t.second);
    }
    for (boost::ptr_vector<Channel>::iterator it = g_channels.begin();
         it != g_channels.end(); ++it)
    {
        it->disableAll();
        it->remove();
    }
    return 0;
}
