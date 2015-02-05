#include "EventLoop.h"
#include "TcpServer.h"

#include <map>

using namespace blink;

typedef std::map<string, string> UserMap;
UserMap users;

string getUser(const string& user)
{
    string result("No such user");
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
        string user(buf->peek(), crlf);
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
