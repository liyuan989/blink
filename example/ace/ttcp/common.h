#ifndef __EXAMPLE_ACE_TTCP_COMMON_H__
#define __EXAMPLE_ACE_TTCP_COMMON_H__

#include <string>
#include <stdint.h>

struct Options
{
    uint16_t     port;
    int          length;
    int          number;
    bool         transmit;
    bool         receive;
    bool         nodelay;
    std::string  host;

    Options()
        : port(0), length(0), number(0),
          transmit(false), receive(false), nodelay(false)
    {
    }
};

struct SessionMessage
{
    int32_t  number;
    int32_t  length;
} __attribute__ ((__packed__));

struct PayloadMessage
{
    int32_t  length;
    char     data[0];
};

bool parseCommandLine(int argc, char* argv[], Options* opt);
struct sockaddr_in resolveOrDie(const char* host, uint16_t port);
void transmit(const Options& opt);
void receive(const Options& opt);

#endif
