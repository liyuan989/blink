#include "EventLoop.h"
#include "TcpServer.h"

#include <map>

using namespace blink;

typedef std::map<std::string, std::string> UserMap;
UserMap users;

std::string getUser(const std::string& user)
{
    std::string result("No such user");
    UserMap::iterator it = users.find(user);
    if (it != users.end())
    {
        result = it->second;
    }
    return result;
}

void onMessage(const TcpConnectionPtr& connection,
               Buffer* buf,
               Timestamp receive_time)
{
    const char* crlf = buf->findCRLF();
    if (crlf)
    {
        std::string user(buf->peek(), crlf);
        connection->send(getUser(user) + "\r\n");
        buf->resetUntil(crlf + 2);
        connection->shutdown();
    }
}

int main(int argc, char const *argv[])
{
    users["john"] = "hey boy!";
    EventLoop loop;
    TcpServer server(&loop, InetAddress(9600), "FingerServer");
    server.setMessageCallback(onMessage);
    server.start();
    loop.loop();
    return 0;
}
