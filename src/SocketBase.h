#ifndef __BLINK_SOCKETBASE_H__
#define __BLINK_SOCKETBASE_H__

#include <netinet/in.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <netdb.h>

#include <stdint.h>

namespace blink
{

namespace sockets
{

unsigned long hton_long(unsigned long hostlong);
unsigned short hton_short(unsigned short hostshort);
unsigned long ntoh_long(unsigned long netlong);
unsigned short ntoh_short(unsigned short netshort);

int64_t hton64(int64_t host64);
int32_t hton32(int32_t host32);
int16_t hton16(int16_t host16);

int64_t ntoh64(int64_t net64);
int32_t ntoh32(int32_t net32);
int16_t ntoh16(int16_t net16);

int inet_pton(int family, const char* str, struct in_addr* addr);
const char* inet_ntop(int family, const struct in_addr* addr, char* str, size_t len);

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

int gethostbyname_r(const char* name, struct hostent* ret, char* buf, size_t buflen,
                    struct hostent** result, int* h_errnop);  // thread safe

int gethostbyaddr_r(const void* addr, socklen_t len, int type, struct hostent* ret, char* buf, size_t buflen,
                    struct hostent** result, int* h_errnop);  // thread safe

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

int getaddrinfo(const char* hostname, const char* service, const struct addrinfo* hints, struct addrinfo** result);
int getnameinfo(const struct sockaddr* addr, socklen_t addrlen, char* host, socklen_t hostlen,
	            char* serv, socklen_t servlen, int flags);

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
const struct sockaddr* sockaddr_cast(const struct socket_in* addr);
struct sockaddr_in* sockaddr_in_cast(struct sockaddr* addr);
const struct sockaddr_in* sockaddr_in_cast(const struct sockaddr* addr);

int socket(int domain, int type, int protocol);
int connect(int sockfd, struct sockaddr* server_addr, int addrlen);
int bind(int sockfd, struct sockaddr* my_addr, int addrlen);
int listen(int sockfd, int backlog);
int accept(int sockfd, struct sockaddr* addr, int* addrlen);
int accept4(int sockfd, struct sockaddr* addr, int* addrlen, int flags);
int read(int fd, void* buf, unsigned int n);
int write(int fd, const void* buf, unsigned int n);
int close(int fd);
int shutdown(int sockfd, int howto);

//  struct iovec
//  {
//      void*   iov_base;   /* Starting address */
//      size_t  iov_len;    /* Length in bytes */
//  };

ssize_t readv(int fd, const struct iovec* iov, int iovcnt);
ssize_t writev(int fd, const struct iovec* iov, int iovcnt);

int createNonblocking();    // create tcp socket
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
