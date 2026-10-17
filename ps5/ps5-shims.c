/* C library functions the payload SDK's libc does not provide, for a PS5 title. */
#include <stddef.h>
#include <stdint.h>
#include <time.h>

/* The C locale is the only one in a title: one byte per character. */
int ___mb_cur_max(void) { return 1; }

/* Advice only: ignoring it is always allowed. */
int posix_madvise(void *addr, size_t len, int advice) {
    (void)addr; (void)len; (void)advice;
    return 0;
}

/* Days since 1970-01-01 for a proleptic Gregorian date (Howard Hinnant's days_from_civil). */
static int64_t days_from_civil(int64_t y, unsigned m, unsigned d) {
    y -= m <= 2;
    const int64_t era = (y >= 0 ? y : y - 399) / 400;
    const unsigned yoe = (unsigned)(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + (int64_t)doe - 719468;
}

/* mktime for UTC: struct tm fields as given, out-of-range months folded into years. */
time_t timegm(struct tm *tm) {
    int64_t year = (int64_t)tm->tm_year + 1900;
    int64_t month = tm->tm_mon;
    year += month / 12;
    month %= 12;
    if (month < 0) {
        month += 12;
        year -= 1;
    }
    int64_t days = days_from_civil(year, (unsigned)month + 1, 1) + (tm->tm_mday - 1);
    return (time_t)(days * 86400 + (int64_t)tm->tm_hour * 3600 + (int64_t)tm->tm_min * 60 + tm->tm_sec);
}

/* Perfect Dark's libraries reach for these; the title's system modules have none of them. */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

struct passwd;
struct group;
struct mmsghdr;
struct timespec;

/* fs.c: a scratch file for an archive member. In the title folder, gone once closed. */
FILE *tmpfile(void) {
    char path[] = "/app0/.tmpfileXXXXXX";
    int fd = mkstemp(path);
    if (fd < 0) return NULL;
    unlink(path);
    FILE *f = fdopen(fd, "w+b");
    if (!f) close(fd);
    return f;
}

/* curl's socketpair fallback. */
int pipe2(int fds[2], int flags) {
    if (pipe(fds) != 0) return -1;
    for (int i = 0; i < 2; i++) {
        if (flags & O_NONBLOCK) fcntl(fds[i], F_SETFL, fcntl(fds[i], F_GETFL) | O_NONBLOCK);
        if (flags & O_CLOEXEC) fcntl(fds[i], F_SETFD, FD_CLOEXEC);
    }
    return 0;
}

/* curl: a scope id by interface name, for IPv6 link-local URLs nobody types here. */
unsigned int if_nametoindex(const char *name) { (void)name; errno = ENXIO; return 0; }

/* OpenSSL's DTLS batching: it falls back to one datagram at a time on ENOSYS. */
#include <sys/socket.h>
ssize_t sendmmsg(int s, struct mmsghdr *__restrict v, size_t n, int flags) {
    (void)s; (void)v; (void)n; (void)flags; errno = ENOSYS; return -1;
}
ssize_t recvmmsg(int s, struct mmsghdr *__restrict v, size_t n, int flags, const struct timespec *__restrict t) {
    (void)s; (void)v; (void)n; (void)flags; (void)t; errno = ENOSYS; return -1;
}

/* unrar: owners by name, only for archives extracted with -ow, which nothing asks for. */
struct passwd *getpwnam(const char *name) { (void)name; return NULL; }
struct group *getgrnam(const char *name) { (void)name; return NULL; }

/* OpenSSL's async jobs (ASYNC_start_job) switch stacks with these. Nothing in the title
 * starts one: an engine would have to ask for it, and none is loaded. */
int _setjmp(void *env) { (void)env; return 0; }
void _longjmp(void *env, int val) { (void)env; (void)val; abort(); }

/* libzstd (PacBrew, under libcurl) names its tracing hooks as weak imports, which the title
 * converter refuses: defined here as no tracing. A zero context means "do not trace". */
unsigned long long ZSTD_trace_compress_begin(const void *cctx) { (void)cctx; return 0; }
void ZSTD_trace_compress_end(unsigned long long ctx, const void *trace) { (void)ctx; (void)trace; }
unsigned long long ZSTD_trace_decompress_begin(const void *dctx) { (void)dctx; return 0; }
void ZSTD_trace_decompress_end(unsigned long long ctx, const void *trace) { (void)ctx; (void)trace; }

/* Names a title can import but that only libScePosixForWebKit's and libkernel_sys's stubs
 * define: those imports are null at run time (PS5_Vulkan tools/radv-link.sh), so they are
 * defined here. Found by tools/check-imports.sh. */
#include <ctype.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

int mkstemps(char *template, int suffixlen);
int mkstemp(char *template) { return mkstemps(template, 0); }

char *strcasestr(const char *haystack, const char *needle) {
    size_t n = strlen(needle);
    for (; *haystack; haystack++) {
        if (strncasecmp(haystack, needle, n) == 0) return (char *)haystack;
    }
    return n == 0 ? (char *)haystack : NULL;
}

/* *, ? and [...] with ranges and !/^, as unrar and curl's globbing ask; flags beyond
 * FNM_CASEFOLD (0x10 on FreeBSD) are not needed by them. */
static int fnmatch_at(const char *p, const char *s, int fold) {
    for (; *p; p++, s++) {
        if (*p == '*') {
            while (*p == '*') p++;
            if (!*p) return 0;
            for (; *s; s++) if (fnmatch_at(p, s, fold) == 0) return 0;
            return 1;
        }
        if (!*s) return 1;
        if (*p == '?') continue;
        if (*p == '[') {
            const char *q = p + 1;
            int neg = (*q == '!' || *q == '^'), hit = 0;
            if (neg) q++;
            for (; *q && (*q != ']' || q == p + 1 + neg); q++) {
                unsigned char lo = (unsigned char)*q, hi = lo, c = (unsigned char)*s;
                if (q[1] == '-' && q[2] && q[2] != ']') { hi = (unsigned char)q[2]; q += 2; }
                if (fold) { c = (unsigned char)tolower(c); lo = (unsigned char)tolower(lo); hi = (unsigned char)tolower(hi); }
                if (c >= lo && c <= hi) hit = 1;
            }
            if (!*q || hit == neg) return 1;
            p = q;
            continue;
        }
        if (fold ? tolower((unsigned char)*p) != tolower((unsigned char)*s) : *p != *s) return 1;
    }
    return *s ? 1 : 0;
}
int fnmatch(const char *pattern, const char *string, int flags) {
    return fnmatch_at(pattern, string, (flags & 0x10) != 0);
}

const char *gai_strerror(int code) { (void)code; return "name resolution failed"; }
struct hostent *gethostbyname(const char *name) { (void)name; return NULL; }

/* Numbers only, which is all ENet, curl and OpenSSL ask of it here. */
int getnameinfo(const struct sockaddr *sa, socklen_t salen, char *host, size_t hostlen,
                char *serv, size_t servlen, int flags) {
    (void)salen; (void)flags;
    unsigned port = 0;
    if (sa->sa_family == AF_INET) {
        const struct sockaddr_in *in = (const struct sockaddr_in *)sa;
        if (host && hostlen && !inet_ntop(AF_INET, &in->sin_addr, host, hostlen)) return EAI_FAIL;
        port = ntohs(in->sin_port);
    } else if (sa->sa_family == AF_INET6) {
        const struct sockaddr_in6 *in6 = (const struct sockaddr_in6 *)sa;
        if (host && hostlen && !inet_ntop(AF_INET6, &in6->sin6_addr, host, hostlen)) return EAI_FAIL;
        port = ntohs(in6->sin6_port);
    } else {
        return EAI_FAMILY;
    }
    if (serv && servlen) snprintf(serv, servlen, "%u", port);
    return 0;
}

int isatty(int fd) { (void)fd; errno = ENOTTY; return 0; }
int lchown(const char *path, uid_t owner, gid_t group) { (void)path; (void)owner; (void)group; return 0; }
int link(const char *from, const char *to) { (void)from; (void)to; errno = EPERM; return -1; }
int symlink(const char *from, const char *to) { (void)from; (void)to; errno = EPERM; return -1; }
ssize_t readlink(const char *path, char *buf, size_t size) { (void)path; (void)buf; (void)size; errno = EINVAL; return -1; }
int lutimes(const char *path, const struct timeval *times) { (void)path; (void)times; return 0; }
long pathconf(const char *path, int name) { (void)path; (void)name; return 255; }
static mode_t ps5_umask_value = 022;
mode_t umask(mode_t mask) { mode_t old = ps5_umask_value; ps5_umask_value = mask & 0777; return old; }
