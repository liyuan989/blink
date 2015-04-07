#ifndef __BLINK_SOCKETBASE_H__
#define __BLINK_SOCKETBASE_H__

#include <netinet/in.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>

#include <stdint.h>

namespace blink
{

namespace sockets
{

uint32_t hton_long(uint32_t hostlong);
uint16_t hton_short(uint16_t hostshort);
uint32_t ntoh_long(uint32_t netlong);
uint16_t ntoh_short(uint16_t netshort);

void toIpPort(char* buf, size_t size, const struct sockaddr_in& addr);
void toIp(char* buf, size_t size, const struct sockaddr_in& addr);
void fromIpPort(const char* ip, uint16_t port, struct sockaddr_in* addr);

//  struct hostent
//  {
//  	char*   h_name;              /* official name of host */
//  	char**  h_aliases;           /* alias list */
//  	int     h_addrtype;          /* host address type */
//  	int     h_length;            /* length of address */
//  	char**  h_addr_list;         /* list of addresses */
//  };
//
//  #define h_addr h_addr_list[0]    /* for backward compatibility */

struct hostent* gethostbyname(const char* name);  // thread unsafe
struct hostent* gethostbyaddr(const char* addr, int len, int type);  // thread unsafe

int gethostbyname_r(const char* name, struct hostent* ret, char* buf,
                    size_t buflen, struct hostent** result, int* h_errnop);  // thread safe

int gethostbyaddr_r(const void* addr, socklen_t len, int type, struct hostent* ret,
                    char* buf, size_t buflen, struct hostent** result, int* h_errnop);  // thread safe

//  struct addrinfo
//  {
//      int               ai_flags;       /* AI_PASSIVE, AI_CANONNAME */
//      int               ai_family;      /* AF_xxx */
//      int               ai_socktype;    /* SOCK_xxx */
//      int               ai_protocol;    /* 0 or IPPROTO_xxx for IPv4 and IPv6 */
//      socklen_t         ai_addrlen;     /* length of ai_addr */
//      char*             ai_canonname;   /* ptr to canonical name for host */
//      struct sockaddr*  ai_addr;        /* ptr to socket address structure */
//      struct addrinfo*  ai_next;        /* ptr to next structure in licked list */
//  };

int getaddrinfo(const char* hostname, const char* service,
                const struct addrinfo* hints, struct addrinfo** result);

int getnameinfo(const struct sockaddr* addr, socklen_t addrlen, char* host,
                socklen_t hostlen, char* serv, socklen_t servlen, int flags);

//  struct sockaddr
//  {
//  　　unsigned short sa_family;    /* address family, AF_xxx */
//  　　char sa_data[14];            /* 14 bytes of protocol address */
//  };
//
//  struct sockaddr_in
//  {
//  	short int sin_family;        /* Address family */
//  	unsigned short int sin_port; /* Port number */
//  	struct in_addr sin_addr;     /* Internet address */
//  	unsigned char sin_zero[8];   /* Same size as struct sockaddr */
//  };
//
//  struct in_addr
//  {
//      unsigned int s_addr;         /* 32 bits */
//  };

struct sockaddr* sockaddr_cast(struct sockaddr_in* addr);
const struct sockaddr* sockaddr_cast(const struct sockaddr_in* addr);
struct sockaddr_in* sockaddr_in_cast(struct sockaddr* addr);
const struct sockaddr_in* sockaddr_in_cast(const struct sockaddr* addr);

int socket(int domain, int type, int protocol);
int connect(int sockfd, const struct sockaddr_in& server_addr);
void bindOrDie(int sockfd, const struct sockaddr_in& addr);
void listenOrDie(int sockfd);
int accept(int sockfd, struct sockaddr_in* addr);
ssize_t read(int fd, void* buf, size_t n);
ssize_t write(int fd, const void* buf, size_t n);
int close(int fd);
int shutdown(int sockfd, int howto);

//  struct iovec
//  {
//      void*   iov_base;   /* Starting address */
//      size_t  iov_len;    /* Length in bytes */
//  };

ssize_t readv(int fd, const struct iovec* iov, int iovcnt);
ssize_t writev(int fd, const struct iovec* iov, int iovcnt);

int createNonblockingOrDie();    // create tcp ipv4 socket
bool setNonBlockAndCloseOnExec(int sockfd);  // fcntl
int getSocketError(int sockfd);
bool isSelfConnect(int sockfd);

// Implement by getsockname
struct sockaddr_in getLocalAddr(int sockfd);

// Implement by getpeername
struct sockaddr_in getPeerAddr(int sockfd);

int openClientFd(char* hostname, int port);
int openListenFd(int port);

}  // namespace sockets

}  // namespace blink

#endif
