#include "Session.h"
#include "MemcacheServer.h"

#ifndef HAVE_TCMALLOC
#include <gperftools/malloc_extension.h>
#endif

using namespace blink;

static bool isBinaryProtocal(uint8_t first_byte)
{
    return first_byte == 0x80;
}

const int kLongestKeySize = 250;
string Session::kLongestKey(kLongestKeySize, 'x');

template<typename InputIterator, typename Token>
bool Session::SpaceSeparator::operator()(InputIterator& next, InputIterator end, Token& tok)
{
    while (next != end && *next == ' ')
    {
        ++next;
    }
    if (next == end)
    {
        tok.clear();
        return false;
    }
    InputIterator start(next);
    const char* sp = static_cast<const char*>(memchr(start, ' ', end - start));
    if (sp)
    {
        tok.set(start, static_cast<int>(sp - start));
        next = sp;
    }
    else
    {
        tok.set(start, static_cast<int>(end - next));
        next = end;
    }
    return true;
}

struct Session::Reader
{
    Reader(Tokenizer::iterator& begin, Tokenizer::iterator end)
        : first_(begin), last_(end)
    {
    }

    template<typename T>
    bool read(T* val)
    {
        if (first_ == last_)
        {
            return false;
        }
        char* end = NULL;
        uint64_t x = strtoul((*first_).data(), &end, 10);
        if (end == (*first).end())
        {
            *val = static_cast<T>(x);
            ++first;
            return true;
        }
        return false;
    }

private:
    Tokenizer::iterator  first_;
    Tokenizer::iterator  last_;
};

void Session::onMessage(const TcpConnectionPtr& connection,
                        Buffer* buf,
                        Timestamp receive_time)
{
    const size_t initial_readable = buf->readableSize();
    while (buf->readableSize() > 0)
    {
        if (state_ == kNewCommand)
        {
            if (protocol_ == kAuto)
            {
                assert(bytes_read_ == 0);
                protocol_ = isBinaryProtocal(buf->peek()[0]) ? kBinary : kAscii;
            }
            assert(protocol_ == kAscii || protocol_ == kBinary);
            if (protocol_ == kBinary)
            {
                // FIXME
            }
            else // protocol_ == kAscii
            {
                const char* crlf = buf->findCRLF();
                if (crlf)
                {
                    int len = static_cast<int>(crlf - buf->peek());
                    StringPiece request(buf->peek(), len);
                    if (processRequest(request))
                    {
                        resetRequest();
                    }
                    buf->resetUntil(crlf + 2);
                }
                else
                {
                    if (buf->readableSize() > 1024)
                    {
                        // FIXME: check for 'get' and 'gets'
                        connection_->shutdown();
                        // buf->resetAll() ???
                    }
                    break;
                }
            }
        }
        else if (state_ == kReceiveValue)
        {
            receiveValue(buf);
        }
        else if (state_ == kDiscardValue)
        {
            receiveValue(buf);
        }
        else
        {
            assert(false);
        }
    }
    bytes_read_ += initial_readable - buf->readableSize();
}
