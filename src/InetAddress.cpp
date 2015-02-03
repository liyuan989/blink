#include "InetAddress.h"
#include "Endian.h"
#include "Log.h"

#include <boost/static_assert.hpp>

#include <string.h>
#include <assert.h>
#include <stdio.h>

namespace blink
{

BOOST_STATIC_ASSERT(sizeof(InetAddress) == sizeof(struct sockaddr_in));

InetAddress::InetAddress(uint16_t port, bool loop_back_only)
{
    memset(&addr_, 0, sizeof(addr_));
    addr_.sin_family = AF_INET;
    in_addr_t ip = loop_back_only ? INADDR_LOOPBACK : INADDR_ANY;     // in_addr_t equal to uint32_t
    addr_.sin_addr.s_addr = sockets::hostToNetwork32(ip);
    addr_.sin_port = sockets::hostToNetwork16(port);
}

InetAddress::InetAddress(std::string ip, uint16_t port)
{
    memset(&addr_, 0, sizeof(addr_));
    sockets::fromIpPort(ip.c_str(), port, &addr_);
}

std::string InetAddress::toIp() const
{
    char buf[32];
    sockets::toIp(buf, sizeof(buf), addr_);
    return buf;
}

std::string InetAddress::toIpPort() const
{
    char buf[32];
    sockets::toIpPort(buf, sizeof(buf), addr_);
    return buf;
}

uint16_t InetAddress::toPort() const
{
    return sockets::networkToHost16(addr_.sin_port);
}

//  struct hostent
//  {
//      char*   h_name;              /* official name of host */
//      char**  h_aliases;           /* alias list */
//      int     h_addrtype;          /* host address type */
//      int     h_length;            /* length of address */
//      char**  h_addr_list;         /* list of addresses */
//  };
//
//  #define h_addr h_addr_list[0]    /* for backward compatibility */

static __thread char t_resolveBuffer[1024 * 64];

bool InetAddress::resolve(std::string hostname, InetAddress* result)
{
    assert(result != NULL);
    struct hostent hent;
    memset(&hent, 0, sizeof(hent));
    struct hostent* he = NULL;
    int herrno = 0;

    int val = sockets::gethostbyname_r(hostname.c_str(), &hent, t_resolveBuffer,
                                       sizeof(t_resolveBuffer), &he, &herrno);
    if (val == 0 && he != NULL)
    {
        assert(he->h_addrtype == AF_INET && he->h_length == sizeof(uint32_t));
        result->addr_.sin_addr = *reinterpret_cast<struct in_addr*>(he->h_addr);
        return true;
    }
    else
    {
        if (val)
        {
            LOG_ERROR << "InetAddress::resolve";
        }
        return false;
    }
}

}  // namespace blink
