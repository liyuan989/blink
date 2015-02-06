#include "PubSubClient.h"

#include "EventLoop.h"
#include "EventLoopThread.h"
#include "ProcessInfo.h"

#include <iostream>
#include <stdio.h>

using namespace blink;

EventLoop* g_loop = NULL;
string g_topic;
string g_content;

void connection(PubSubClient* client)
{
    if (client->connected())
    {
        client->publish(g_topic, g_content);
        client->stop();
    }
    else
    {
        g_loop->quit();
    }
}

int main(int argc, char const *argv[])
{
    if (argc != 4)
    {
        printf("Usage: %s <ip>:<port> <topic> <content>\n"
               "Read contents from stdin:\n"
               "%s <ip>:<port> <topic> -\n", argv[0], argv[0]);
        return 1;
    }
    string hostport = argv[1];
    size_t colon = hostport.find(':');
    if (colon != string::npos)
    {
        string ip = hostport.substr(0, colon);
        uint16_t port = static_cast<uint16_t>(atoi(hostport.c_str() + colon + 1));
        g_topic = argv[2];
        g_content = argv[3];
        string name = username() + "@" + hostName();
        name += ":" + pidString();

        if (g_content == "-")
        {
            EventLoopThread loop_thread;
            g_loop = loop_thread.startLoop();
            PubSubClient client(g_loop, InetAddress(ip, port), name);
            client.start();
            string line;
            while (std::getline(std::cin, line))
            {
                client.publish(g_topic, line);
            }
            client.stop();
            sleepMicroseconds(1000 * 1000);
        }
        else
        {
            EventLoop loop;
            g_loop = &loop;
            PubSubClient client(g_loop, InetAddress(ip, port), name);
            client.setConnectionCallback(connection);
            client.start();
            loop.loop();
        }
    }
    else
    {
        printf("Usage: %s <ip>:<port> <topic> <content>\n", argv[0]);
    }
    return 0;
}
