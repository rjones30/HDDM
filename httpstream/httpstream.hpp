#ifndef HTTPSTREAM_T
#define HTTPSTREAM_T 1

#include <cpr/cpr.h>
#include <iostream>
#include <thread>
#include <deque>

class httpStreambuf : public std::streambuf {
 public:
    httpStreambuf(const std::string& url, size_t buffersize=10000000);
    virtual ~httpStreambuf();
    virtual int underflow();
    void setg(char *gbeg, char *gcurr, char *gend);

    std::string url_;
    std::streamoff buffersize_;
    int readahead_;
    int lookback_;
    int buffer_index_;
    int verbose_;

 protected:
    virtual int advance();

 private:
    class stream_block {
     private:
        stream_block();
     public:
        stream_block(std::streampos offset, std::streamoff size) {
           offset_ = offset;
	   size_ = size;
           reader_ = 0;
        }
        ~stream_block() {
           if (reader_ != 0)
              delete reader_;
        }
        static void background_fill(stream_block* block, 
                                    const std::string url);

        std::streamoff offset_;
	std::streamoff size_;
        cpr::Response resp_;
        std::thread *reader_;
    };
    std::deque<stream_block*> buffer_;
};

class httpIstream : public std::istream {
public:
    httpIstream(const std::string& url) : std::istream(&buf_), buf_(url) {}

private:
    httpStreambuf buf_;
};

#endif
