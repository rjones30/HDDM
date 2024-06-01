#ifndef XROOTDSTREAM_T
#define XROOTDSTREAM_T 1

#include <iostream>
#include <thread>
#include <deque>

#include <XrdCl/XrdClFile.hh>
#include <XrdCl/XrdClFileSystem.hh>

class xrootdStreambuf : public std::streambuf {
 public:
    xrootdStreambuf(const std::string& url, size_t buffersize=10000000);
    virtual ~xrootdStreambuf();
    virtual int underflow();
    void setg(char *gbeg, char *gcurr, char *gend);

    std::string url_;
    std::streamoff buffersize_;
    int readahead_;
    int lookback_;
    int buffer_index_;
    int verbose_;
    XrdCl::FileSystem *xrdfs_;
    XrdCl::File *xrdfile_;

 protected:
    virtual int advance();

 private:
    class stream_block {
     private:
        stream_block();
     public:
        stream_block(std::streampos offset, std::streamoff size) {
           offset_ = offset;
           buf_ = new char[size];
           size_ = size;
           bytes_read_ = 0;
           reader_ = 0;
        }
        virtual ~stream_block() { delete [] buf_; }
        static void background_fill(stream_block* block, 
			            XrdCl::File *xrdfile,
                                    const std::string url);

        std::streampos offset_;
        char *buf_;
        XrdCl::XRootDStatus resp_;
        std::streamoff size_;
        unsigned int bytes_read_;
        std::thread *reader_;
    };
    std::deque<stream_block*> buffer_;
};

class xrootdIstream : public std::istream {
public:
    xrootdIstream(const std::string& url) : std::istream(&buf_), buf_(url) {}

private:
    xrootdStreambuf buf_;
};

#endif
