#include "PubSubClient.h"

#include "EventLoop.h"
#include "ProcessInfo.h"

#include <vector>
#include <stdio.h>

using namespace blink;

EventLoop* g_loop = NULL;
std::vector<std::string> g_topics;

void subscription(const std::string& topic, const std::string& content, Timestamp)
{
    printf("%s: %s\n", topic.c_str(), content.c_str());
}

void connection(PubSubClient* client)
{
    if (client->connected())
    {
        for (std::vector<std::string>::iterator it = g_topics.begin();
             it != g_topics.end(); ++it)
        {
            client->subscribe(*it, subscription);
        }
    }
    else
    {
        g_loop->quit();
    }
}

int main(int argc, char const *argv[])
{
    if (argc < 3)
    {
        printf("Usage: %s <ip>:<port> <topic> [topic ...]>\n", argv[0]);
        return 1;
    }
    std::string hostport = argv[1];
    size_t colon = hostport.find(':');
    if (colon != std::string::npos)
    {
        std::string ip = hostport.substr(0, colon);
        uint16_t port = static_cast<uint16_t>(atoi(hostport.c_str() + colon + 1));
        for (int i = 2; i < argc; ++i)
        {
            g_topics.push_back(argv[i]);
        }
        EventLoop loop;
        g_loop = &loop;
        std::string name = username() + "@" + hostName();
        name += ":" + pidString();
        PubSubClient client(g_loop, InetAddress(ip, port), name);
        client.setConnectionCallback(connection);
        client.start();
        loop.loop();
    }
    else
    {
        printf("Usage : %s <ip>:<port> <topic> [topic ...]>\n", argv[0]);
    }
    return 0;
}
