#include <example/hub/Codec.h>

using namespace blink;

ParseResult parseMessage(Buffer* buf,
                         string* command,
                         string* topic,
                         string* content)
{
    ParseResult result = kError;
    const char* crlf = buf->findCRLF();
    if (crlf)
    {
        const char* space = std::find(buf->peek(), crlf, ' ');
        if (space != crlf)
        {
            command->assign(buf->peek(), space);
            topic->assign(space + 1, crlf);
            if (*command == "pub")
            {
                const char* start = crlf + 2;
                crlf = buf->findCRLF(start);
                if (crlf)
                {
                    content->assign(start, crlf);
                    buf->resetUntil(crlf + 2);
                    result = kSuccess;
                }
                else
                {
                    result = kContinue;
                }
            }
            else
            {
                buf->resetUntil(crlf + 2);
                result = kSuccess;
            }
        }
        else
        {
            result = kError;
        }
    }
    else
    {
        result = kContinue;
    }
    return result;
}
