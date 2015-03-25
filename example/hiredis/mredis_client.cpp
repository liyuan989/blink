#include <example/hiredis/Hiredis.h>

#include <blink/EventLoop.h>
#include <blink/Log.h>

#include <string>

using namespace blink;

string toString(long long value)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "%lld", value);
    return buf;
}

string redisReplyToString(const redisReply* reply)
{
    static const char* const types[] = {"",
        "REDIS_REPLY_STRING", "REDIS_REPLY_ARRAY",
        "REDIS_REPLY_INTEGER", "REDIS_REPLY_NIL",
        "REDIS_REPLY_STATUS", "REDIS_REPLY_ERROR"};
    string str;
    if (!reply)
    {
        return str;
    }

    str += (types[reply->type] + string("(") + toString(reply->type) + ") ");
    str += "{ ";
    if (reply->type == REDIS_REPLY_STRING ||
        reply->type == REDIS_REPLY_STATUS ||
        reply->type == REDIS_REPLY_ERROR)
    {
        str += '"' + string(reply->str, reply->len) + '"';
    }
    else if (reply->type == REDIS_REPLY_INTEGER)
    {
        str += toString(reply->integer);
    }
    else if (reply->type == REDIS_REPLY_ARRAY)
    {
        str += (toString(reply->elements) + " ");
        for (size_t i = 0; i < reply->elements; ++i)
        {
            str += " " + redisReplyToString(reply->element[i]);
        }
    }
    str += " }";
    return str;
}

void connectCallback(Hiredis* c, int status)
{
    if (status != REDIS_OK)
    {
        LOG_ERROR << "connectCallback error: " << c->errstr();
    }
    else
    {
        LOG_INFO << "Connected...";
    }
}

void disconnectCallback(Hiredis* c, int status)
{
    if (status != REDIS_OK)
    {
        LOG_ERROR << "disconnectCallback: " << c->errstr();
    }
    else
    {
        LOG_INFO << "Disconnected...";
    }
}

void timeCallback(Hiredis* c, redisReply* reply)
{
    LOG_INFO << "time " << redisReplyToString(reply);
}

void echoCallback(Hiredis* c, redisReply* reply, string* echo)
{
    LOG_INFO << *echo << " " << redisReplyToString(reply);
    c->disconnect();
}

void dbsizeCallback(Hiredis* c, redisReply* reply)
{
    LOG_INFO << "dbsize " << redisReplyToString(reply);
}

void selectCallback(Hiredis* c, redisReply* reply, uint16_t* index)
{
    LOG_INFO << "select " << *index << " " << redisReplyToString(reply);
}

void authCallback(Hiredis* c, redisReply* reply, string* password)
{
    LOG_INFO << "auth " << *password << " " << redisReplyToString(reply);
}

void echo(Hiredis* c, string* s)
{
    c->command(boost::bind(echoCallback, _1, _2, s), "echo %s", s->c_str());
}

int main(int argc, char* argv[])
{
    Log::setLogLevel(Log::DEBUG);
    EventLoop loop;
    InetAddress server_addr("127.0.0.1", 6379);
    Hiredis hiredis(&loop, server_addr);
    hiredis.setConnectCallback(connectCallback);
    hiredis.setDisconnectCallback(disconnectCallback);
    hiredis.connect();

    hiredis.ping();
    loop.runEvery(1.0, boost::bind(&Hiredis::ping, &hiredis));

    hiredis.command(timeCallback, "time");

    string hi = "hi";
    hiredis.command(boost::bind(echoCallback, _1, _2, &hi), "echo %s", hi.c_str());
    loop.runEvery(2.0, boost::bind(echo, &hiredis, &hi));

    hiredis.command(dbsizeCallback, "dbsize");

    uint16_t index = 8;
    hiredis.command(boost::bind(selectCallback, _1, _2, &index), "select %d", index);

    string password = "password";
    hiredis.command(boost::bind(authCallback, _1, _2, &password), "auth %s", password.c_str());

    loop.loop();
    return 0;
}
