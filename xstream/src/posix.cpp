#include <xstream/config.h>
#include <xstream/posix.h>
#include <xstream/except/posix.h>
#include <iosfwd>

#include <string>
#include <ctime>

//for read write, etc
#ifdef _MSC_VER
#include <unistd_win32.h>
#else
#include <unistd.h>
#endif

//for errno
#include <errno.h>

//for strerror and strerror_r
#include <cstring>

#include "debug.h"

#ifdef _MSC_VER
typedef SSIZE_T ssize_t;
#endif

namespace xstream {
namespace posix{

    date_format::date_format(const std::string& f):format(f){};

    std::string date_format::now() const {
        LOG("posix::date_format::now");
        ::time_t t;
        ::time(&t);
        
        struct std::tm tm;

        //XXX check error and throw exception
        ::localtime_r(&t,&tm);

        //first size of the allocated buffer
        //if needed it will be reallocated
        size_t len=512;
        std::string ret;
        char *buf=(char*)malloc(1);

        do {
            free(buf);
            LOG("\tlen=" << len);
            buf = (char*)malloc(len);
            size_t cret=std::strftime(buf,len,format.c_str(),&tm);

            if (0 != cret && cret <= len) {
                buf[len - 1] = '\0'; //just in case
                ret = buf;
            }
            len *= 2;
        } while (0 == ret.size());

        LOG("\tdate=" << ret);
        return ret;
    }

    void check_return(int code, const std::string& call) {
        LOG("posix::check_return " << call << " => " << code);
        if (-1 == code) {
            char errmsg[512];
            ::strerror_r(errno, errmsg, 512);
            const std::string desc(errmsg);
            LOG("\tthrowing " << errno << " => " << desc);
            throw general_error(call, errno, desc);
        }
    }

#if HAVE_FD
    fd::fd(int f, bool c)
    : fdn(f), dest_close(c)
    {
        LOG("posix::fd (" << f << "," << c << ")");
    }
    
    std::streamsize fd::read(char* buf, std::streamsize len)
    {
        LOG("posix::fd::read " << len);
        
        ssize_t count;
        
        do {
#ifndef _MSC_VER
            count = ::read(fdn, buf, len);
#else
            count = ::_read(fdn, buf, (int)len);
#endif
        } while (-1 == count && EINTR == errno);

        check_return((int)count, "read");

        return count;
    }
    
    std::string fd::read(std::streamsize len)
    {
        char *buf = (char*)malloc(len);
        ssize_t count = read(buf, len);
        std::string sbuf(buf, buf + count);
        free(buf);
        return sbuf;
    }
    
    std::streamsize fd::write(const char* buf, std::streamsize len)
    {
        LOG("posix::fd::write " << len);
        
        ssize_t count;
        
        do {
#ifndef _MSC_VER
            count = ::write(fdn, buf, len);
#else
            count = ::_write(fdn, buf, (int)len);
#endif
        } while (-1 == count && EINTR == errno);

        check_return((int)count, "write");

        return count;
    }
    
    void fd::sync()
    {
        LOG("posix::fd::sync");
        int cret = fsync(fdn);
        check_return(cret, "fsync");
    }

    fd::~fd()
    {
        LOG("posix::fd::~fd");
        if (dest_close) {
            LOG("\tclosing");
#ifndef _MSC_VER
            int cret = ::close(fdn);
#else
            int cret = ::_close(fdn);
#endif
            check_return(cret, "close");
        }
    }
#endif

}//namespace posix
}//namespace xstream
