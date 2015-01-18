#ifndef __BLINK_INETADDRESS_H__
#define __BLINK_INETADDRESS_H__

#include "Copyable.h"
#include "SocketBase.h"

#include <string>
#include <stdint.h>

namespace blink
{

//  struct sockaddr
//  {
//  　　unsigned short sa_family;    /* address family, AF_xxx */
//  　　char sa_data[14];            /* 14 bytes of protocol address */
//  };
//
//  struct sockaddr_in
//  {
//      short int sin_family;        /* Address family */
//      unsigned short int sin_port; /* Port number */
//      struct in_addr sin_addr;     /* Internet address */
//      unsigned char sin_zero[8];   /* Same size as struct sockaddr */
//  };
//
//  struct in_addr
//  {
//      unsigned int s_addr;         /* 32 bits */
//  };

class InetAddress : Copyable
{
public:
    explicit InetAddress(const struct sockaddr_in& addr)
        : addr_(addr)
    {
    }

    explicit InetAddress(uint16_t port = 0, bool loop_back_only = false);
    InetAddress(std::string ip, uint16_t port);

    std::string toIp() const;
    std::string toIpPort() const;
    uint16_t toPort() const;

    const struct sockaddr_in getSockAddrInet() const
    {
        return addr_;
    }

    void setSockAddrInet(const struct sockaddr_in& addr)
    {
        addr_ = addr;
    }

    uint32_t ipNetEndian() const
    {
        return addr_.sin_addr.s_addr;
    }

    uint16_t portNetEndian() const
    {
        return addr_.sin_port;
    }

    static bool resolve(std::string hostname, InetAddress* result);

private:
    struct sockaddr_in  addr_;
};

}  // namespace

#endif
