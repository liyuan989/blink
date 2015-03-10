#include "Session.h"
#include "MemcacheServer.h"

#ifdef HAVE_TCMALLOC
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
        if (end == (*first_).end())
        {
            *val = static_cast<T>(x);
            ++first_;
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
            discardValue(buf);
        }
        else
        {
            assert(false);
        }
    }
    bytes_read_ += initial_readable - buf->readableSize();
}

void Session::receiveValue(Buffer* buf)
{
    assert(current_item_.get());
    assert(state_ == kReceiveValue);
    // if (protocol_ == kBinary)

    const size_t avail = std::min(buf->readableSize(), current_item_->neededBytes());
    assert(current_item_.unique());
    current_item_->append(buf->peek(), avail);
    buf->reset(avail);
    if (current_item_->neededBytes() == 0)
    {
        if (current_item_->endWithCRLF())
        {
            bool exists = false;
            if (owner_->storeItem(current_item_, policy_, &exists))
            {
                reply("STORED\r\n");
            }
            else
            {
                if (policy_ == Item::kCas)
                {
                    if (exists)
                    {
                        reply("EXISTS\r\n");
                    }
                    else
                    {
                        reply("NOT_FOUND\r\n");
                    }
                }
                else
                {
                    reply("NOT_STORED\r\n");
                }
            }
        }
        else
        {
            reply("CLIENT_ERROR bad data chunk\r\n");
        }
        resetRequest();
        state_ = kNewCommand;
    }
}

void Session::discardValue(Buffer* buf)
{
    assert(!current_item_);
    assert(state_ == kDiscardValue);
    if (buf->readableSize() < bytes_to_discard_)
    {
        bytes_to_discard_ -= buf->readableSize();
        buf->resetAll();
    }
    else
    {
        buf->reset(bytes_to_discard_);
        bytes_to_discard_ = 0;
        resetRequest();
        state_ = kNewCommand;
    }
}

bool Session::processRequest(StringPiece request)
{
    assert(command_.empty());
    assert(!no_reply_);
    assert(policy_ == Item::kInvalid);
    assert(!current_item_);
    assert(bytes_to_discard_ == 0);
    ++requests_processed_;

    // check "no_reply_" at end of request line.
    if (request.size() >= 8)
    {
        StringPiece end(request.end() - 8, 8);
        if (end == " noreply")
        {
            no_reply_ = true;
            request.removeSuffix(8);
        }
    }

    SpaceSeparator sep;
    Tokenizer tok(request.begin(), request.end(), sep);
    Tokenizer::iterator beg = tok.begin();
    if (beg == tok.end())
    {
        reply("ERROR\r\n");
        return true;
    }
    (*beg).copyToString(&command_);
    ++beg;
    if (command_ == "set" || command_ == "add" || command_ == "replace"
        || command_ == "append" || command_ == "prepend" || command_ == "cas")
    {
        // this normally returns false
        return doUpdate(beg, tok.end());
    }
    else if (command_ == "get" || command_ == "gets")
    {
        bool cas = command_ == "gets";

        // FIXME: send multiple chunks with write complete callback.
        while (beg != tok.end())
        {
            StringPiece key = *beg;
            bool good = key.size() <= kLongestKeySize;
            if (!good)
            {
                reply("CLIENT_ERROR bad command line format\r\n");
                return true;
            }
            needle_->resetKey(key);
            ConstItemPtr item = owner_->getItem(needle_);
            ++beg;
            if (item)
            {
                item->output(&output_buffer_, cas);
            }
        }
        output_buffer_.append("END\r\n");
        if (connection_->outputBuffer()->writeableSize() > 65536 + output_buffer_.readableSize())
        {
            LOG_DEBUG << "shrink output buffer from " << connection_->outputBuffer()->bufferCapacity();
            connection_->outputBuffer()->shrink(65536 + output_buffer_.readableSize());
        }
        connection_->send(&output_buffer_);
    }
    else if (command_ == "delete")
    {
        doDelete(beg, tok.end());
    }
    else if (command_ == "version")
    {
#ifdef HAVE_TCMALLOC
        reply("VERSION 0.01 blink with tcmalloc\r\n");
#else
        reply("VERSION 0.01 blink\r\n");
#endif
    }
#ifdef HAVE_TCMALLOC
    else if (command_ == "memstat")
    {
        char buf[1024 * 64];
        MallocExtension::instance()->GetStats(buf, sizeof(buf));
        reply(buf);
    }
#endif
    else if (command_ == "quit")
    {
        connection_->shutdown();
    }
    else if (command_ == "shutdown")
    {
        // "ERROR: shutdown not enabled"
        connection_->shutdown();
        owner_->stop();
    }
    else
    {
        reply("ERROR\r\n");
        LOG_INFO << "Unknown command: " << command_;
    }
    return true;
}

void Session::resetRequest()
{
    command_.clear();
    no_reply_ = false;
    policy_ = Item::kInvalid;
    current_item_.reset();
    bytes_to_discard_ = 0;
}

void Session::reply(StringPiece message)
{
    if (!no_reply_)
    {
        connection_->send(message.data(), message.size());
    }
}

bool Session::doUpdate(Tokenizer::iterator& begin, Tokenizer::iterator end)
{
    if (command_ == "set")
    {
        policy_ = Item::kSet;
    }
    else if (command_ == "add")
    {
        policy_ = Item::kAdd;
    }
    else if (command_ == "replace")
    {
        policy_ = Item::kReplace;
    }
    else if (command_ == "append")
    {
        policy_ = Item::kAppend;
    }
    else if (command_ == "prepend")
    {
        policy_ = Item::kPrepend;
    }
    else if (command_ == "cas")
    {
        policy_ = Item::kCas;
    }
    else
    {
        assert(false);
    }

    // FIXME: check (begin != end)
    StringPiece key = *begin;
    ++begin;
    bool good = key.size() <= kLongestKeySize;
    uint32_t flags = 0;
    time_t exptime = 1;
    int bytes = -1;
    uint64_t cas = 0;

    Reader reader(begin, end);
    good = good && reader.read(&flags) && reader.read(&exptime) && reader.read(&bytes);

    int rel_exptime = static_cast<int>(exptime);
    if (exptime > 60 * 60 * 24 * 30)
    {
        rel_exptime = static_cast<int>(exptime - owner_->startTime());
        if (rel_exptime < 1)
        {
            rel_exptime = 1;
        }
    }
    else
    {
        //rel_exptime = exptime + current_item_;
    }

    if (good && policy_ == Item::kCas)
    {
        good = reader.read(&cas);
    }
    if (!good)
    {
        reply("CLIENT_ERROR bad command line format\r\n");
        return true;
    }

    if (bytes > 1024 * 1024)
    {
        reply("SERVER_ERROR object too large for cache\r\n");
        needle_->resetKey(key);
        owner_->deleteItem(needle_);
        bytes_to_discard_ = bytes + 2;
        state_ = kDiscardValue;
        return false;
    }
    else
    {
        current_item_ = Item::makeItem(key, flags, rel_exptime, bytes + 2, cas);
        state_ = kReceiveValue;
        return false;
    }
}

void Session::doDelete(Tokenizer::iterator& begin, Tokenizer::iterator end)
{
    assert(command_ == "delete");
    // FIXME: check (begin != end)
    StringPiece key = *begin;
    bool good = key.size() <= kLongestKeySize;
    ++begin;
    if (!good)
    {
        reply("CLIENT_ERROR bad command line format\r\n");
    }
    else if (begin != end && *begin != "0")  // old protocol
    {
        reply("CLIENT_ERROR bad command line format. Usage: delete <key> [noreply]\r\n");
    }
    else
    {
        needle_->resetKey(key);
        if (owner_->deleteItem(needle_))
        {
            reply("DELETED\r\n");
        }
        else
        {
            reply("NOT_FOUND\r\n");
        }
    }
}
