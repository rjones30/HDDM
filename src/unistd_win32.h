// this is a place holder

#include <io.h>
#include <time.h>
#include <winsock.h>

#ifdef _WIN32
#ifndef _GETPID_DEF
#define _GETPID_DEF 1
#include <process.h>
#define getpid _getpid
#endif
#ifndef _UNLINK_DEF
#define _UNLINK_DEF 1
#define unlink _unlink
#endif
#endif

#ifndef _LOCALTIME_R
#define _LOCALTIME_R 1
inline struct tm *localtime_r (const time_t *timer, struct tm *tp) {
    localtime_s(tp, timer);
    return tp;
}
#endif

#ifndef _STRERROR_R
#define _STRERROR_R 1
inline int strerror_r(int errnum, char *strerrbuf, size_t buflen) {
    char errbuf[9999];
    strncpy_s(errbuf, 9999, strerrbuf, buflen);
    strerror_s(errbuf, errnum);
    return errnum;
}
#endif

#ifndef _FSYNC
#define _FSYNC 1
int fsync(int fd);
#endif
