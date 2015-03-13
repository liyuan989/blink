#include <blink/InetAddress.h>

#include <sys/socket.h>

#include <sstream>
#include <string.h>
#include <stdio.h>

void dnsParse(const blink::string& hostname)
{
    struct sockaddr_in sock_addr;
    memset(&sock_addr, 0, sizeof(sock_addr));
    blink::InetAddress addr(sock_addr);
    if (blink::InetAddress::resolve(hostname, &addr))
    {
        printf("%s parse succeed, IpAddr: %s\n", hostname.c_str(), addr.toIp().c_str());
    }
    else
    {
        printf("%s parse failed\n", hostname.c_str());
    }
}

void testIpPort()
{
    for (int i = 0; i < 255; ++i)
    {
        std::ostringstream os;
        os << "192.168." << i << ".255";
        blink::InetAddress addr(os.str().c_str(), static_cast<uint16_t>(10 * i));
        printf("Ip:   %s\n", addr.toIp().c_str());
    }
    for (int i = 0; i < 255; ++i)
    {
        blink::InetAddress addr("192.168.1.1", static_cast<uint16_t>(10 * i));
        printf("Port:   %hu\n", addr.toPort());
    }
    for (int i = 0; i < 255; ++i)
    {
        std::ostringstream os;
        os << "192.168.1." << i;
        blink::InetAddress addr(os.str().c_str(), static_cast<uint16_t>(10 * i));
        printf("Ip: Port   %s\n", addr.toIpPort().c_str());
    }
}

void testResolve()
{
    dnsParse("www.baidu.com");
    dnsParse("www.google.com");
    dnsParse("facebook.com");
    dnsParse("stackoverflow.com");
    dnsParse("www.163.com");
    dnsParse("www.zhihu.com");
    dnsParse("twiter.com");
    dnsParse("twitch.com");
    dnsParse("youtube.com");
    dnsParse("cnblog.com");
    dnsParse("www.csdn.com");
    dnsParse("www.1201dsad2sda12sad901.com.dsa");
    dnsParse("dsadqw12e.cn.net.oki");
    dnsParse("23123asdasd12313asdasd");
}

int main(int argc, char const *argv[])
{
    testIpPort();
    testResolve();
    return 0;
}
