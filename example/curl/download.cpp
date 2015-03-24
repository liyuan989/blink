// concurrent downloading one file from http

#include <example/curl/Curl.h>

#include <blink/EventLoop.h>
#include <blink/Nocopyable.h>
#include <blink/Log.h>

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/bind.hpp>

#include <sstream>
#include <stdio.h>

using namespace blink;

typedef boost::shared_ptr<FILE> FilePtr;

template<int N>
bool startWith(const string& str, const char (&prefix)[N])
{
    return str.size() >= N - 1 && std::equal(prefix, prefix + N - 1, str.begin());
}

class Piece : Nocopyable
{
public:
    Piece(const RequestPtr& request,
          const FilePtr& out,
          const string& range,
          const boost::function<void ()>& done)
        : request_(request),
          out_(out),
          range_(range),
          done_callback_(done)
    {
        LOG_INFO << "range: " << range;
        request_->setRange(range);
        request_->setDataCallback(boost::bind(&Piece::onData, this, _1, _2));
        request_->setDoneCallback(boost::bind(&Piece::onDone, this, _1, _2));
    }

private:
    void onData(const char* data, int len)
    {
        ::fwrite(data, 1, len, boost::get_pointer(out_));
    }

    void onDone(Request* request, int code)
    {
        LOG_INFO << "[" << range_ << "] is done";
        request_.reset();
        out_.reset();
        done_callback_();
    }

    RequestPtr                request_;
    FilePtr                   out_;
    string                    range_;
    boost::function<void ()>  done_callback_;
};

class Downloader : blink::Nocopyable
{
public:
    Downloader(EventLoop* loop, const string& url)
        : loop_(loop),
          curl_(loop),
          url_(url),
          request_(curl_.getUrl(url)),
          found_(false),
          accept_range_(false),
          length_(0),
          pieces_(kConcurrent),
          concurrent_(0)
    {
        request_->setHeaderCallback(boost::bind(&Downloader::onHeader, this, _1, _2));
        request_->setDoneCallback(boost::bind(&Downloader::onHeaderDone, this, _1, _2));
        request_->headerOnly();
    }

private:
    void onHeader(const char* data, int len)
    {
        string line(data, len);
        if (startWith(line, "HTTP/1.1 200") || startWith(line, "HTTP/1.0 200"))
        {
            found_ = true;

        }
        if (line == "Accept-Ranges: bytes\r\n")
        {
            accept_range_ = true;
            LOG_DEBUG << "Accept-Ranges";
        }
        else if (startWith(line, "Content-Length:"))
        {
            length_ = atol(line.c_str() + strlen("Content-Length:"));
            LOG_INFO << "Content-Length: " << length_;
        }
    }

    void onHeaderDone(Request* request, int code)
    {
        LOG_DEBUG << code;
        if (accept_range_ && length_ >= kConcurrent * 4096)
        {
            LOG_INFO << "Downloading with " << kConcurrent << " connections";
            concurrent_ = kConcurrent;
            concurrentDownload();
        }
        else if (found_)
        {
            LOG_WARN << "Single connection download";
            FILE* fp = ::fopen("output", "wb");
            if (fp)
            {
                FilePtr(fp, ::fclose).swap(out_);
                request_.reset();
                request2_ = curl_.getUrl(url_);
                request2_->setDataCallback(boost::bind(&Downloader::onData, this, _1, _2));
                request2_->setDoneCallback(boost::bind(&Downloader::onDownloadDone, this));
                concurrent_ = 1;
            }
            else
            {
                LOG_ERROR << "Can not create output file";
                loop_->quit();
            }
        }
        else
        {
            LOG_ERROR << "File not found";
            loop_->quit();
        }
    }

    void concurrentDownload()
    {
        const int64_t piece_len = length_ / kConcurrent;
        for (int i = 0; i < kConcurrent; ++i)
        {
            char buf[256];
            snprintf(buf, sizeof(buf), "output-%05d-of-%05d", i, kConcurrent);
            FILE* fp = ::fopen(buf, "wb");
            if (fp)
            {
                FilePtr out(fp, ::fclose);
                RequestPtr request = curl_.getUrl(url_);
                std::ostringstream range;
                if (i < kConcurrent - 1)
                {
                    range << i * piece_len << "-" << (i + 1) * piece_len - 1;
                }
                else
                {
                    range << i * piece_len << "-" << length_ - 1;
                }
                pieces_.push_back(new Piece(
                    request, out, range.str().c_str(), boost::bind(&Downloader::onDownloadDone, this)));
            }
            else
            {
                LOG_ERROR << "Can not create output file: " << buf;
                loop_->quit();
            }
        }
    }

    void onData(const char* data, int len)
    {
        ::fwrite(data, 1, len, boost::get_pointer(out_));
    }

    void onDownloadDone()
    {
        if (--concurrent_ <= 0)
        {
            loop_->quit();
        }
    }

    EventLoop*                loop_;
    Curl                      curl_;
    string                    url_;
    RequestPtr                request_;
    RequestPtr                request2_;
    bool                      found_;
    bool                      accept_range_;
    int64_t                   length_;
    FilePtr                   out_;
    boost::ptr_vector<Piece>  pieces_;
    int                       concurrent_;

    static const int          kConcurrent = 4;
};

const int Downloader::kConcurrent;

int main(int argc, char* argv[])
{
    EventLoop loop;
    Curl::initialize();
    string url = argc > 1 ? argv[1] : "http://images.cnblogs.com/cnblogs_com/liyuan989/658888/t_o_rss-round-red.png";
    Downloader downloader(&loop, url);
    loop.loop();
    return 0;
}
