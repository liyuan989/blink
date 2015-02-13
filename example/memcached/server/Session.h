#ifndef __EXAMPLE_MEMCACHED_SERVER_SESSION_H__
#define __EXAMPLE_MEMCACHED_SERVER_SESSION_H__

#include "Item.h"

#include <Nocopyable.h>
#include <StringPiece.h>
#include <TcpConnection.h>
#include <Log.h>

#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/tokenizer.hpp>

class MemcacheServer;

class Session : blink::Nocopyable,
                public boost::enable_shared_from_this<Session>
{
public:
    Session(MemcacheServer* owner, const blink::TcpConnectionPtr& connection)
        : owner_(owner),
          connection_(connection),
          state_(kNewCommand),
          protocol_(kAscii),
          no_reply_(false),
          policy_(Item::kInvalid),
          bytes_to_discard_(0),
          needle_(Item::makeItem(kLongestKey, 0, 0, 2, 0)),
          bytes_read_(0),
          requests_processed_(0)
    {
        connection_->setMessageCallback(boost::bind(&Session::onMessage, this, _1, _2, _3));
    }

    ~Session()
    {
        LOG_INFO << "requests processed: " << requests_processed_
                 << " input buffer size: " << connection_->inputBuffer()->bufferCapacity()
                 << " output buffer size: " << connection_->outputBuffer()->bufferCapacity();
    }

private:
    enum State
    {
        kNewCommand,
        kReceiveValue,
        kDiscardValue,
    };

    enum Protocol
    {
        kAscii,
        kBinary,
        kAuto,
    };

    struct Reader;

    struct SpaceSeparator
    {
        void reset()
        {
        }

        template<typename InputIterator, typename Token>
        bool operator()(InputIterator& next, InputIterator end, Token& tok);
    };

    typedef boost::tokenizer<SpaceSeparator, const char*, blink::StringPiece> Tokenizer;

    void onMessage(const blink::TcpConnectionPtr& connection,
                   blink::Buffer* buf,
                   blink::Timestamp receive_time);
    void onWriteComplete(const blink::TcpConnectionPtr& connection);
    void receiveValue(blink::Buffer* buf);
    void discardValue(blink::Buffer* buf);
    // TODO: highWaterMark

    // return true if finished a request
    bool processRequest(blink::StringPiece request);
    void resetRequest();
    void reply(blink::StringPiece message);
    bool doUpdate(Tokenizer::iterator& begin, Tokenizer::iterator& end);
    bool doDelete(Tokenizer::iterator& begin, Tokenizer::iterator& end);

    MemcacheServer*          owner_;
    blink::TcpConnectionPtr  connection_;
    State                    state_;
    Protocol                 protocol_;
    blink::string            command_;           // current request
    bool                     no_reply_;
    Item::UpdatePolicy       policy_;
    ItemPtr                  current_item_;
    size_t                   bytes_to_discard_;
    ItemPtr                  needle_;            // cached
    blink::Buffer            output_buffer_;
    size_t                   bytes_read_;
    size_t                   requests_processed_;

    static blink::string     kLongestKey;
};

typedef boost::shared_ptr<Session> SessionPtr;

#endif
