/* Relative paths for a PS5 title.
 *
 * A title has no working directory: the kernel answers a relative path with EINVAL, so
 * std::filesystem::exists("portable.txt") threw at start-up. The working directory is kept
 * here instead, starting at the title folder, and the path functions the game, libc++ and the
 * libraries use are wrapped (link-title.sh: --wrap) to resolve a relative path against it.
 * getcwd, lstat and access are not wrapped: system modules import those names (link-title.sh).
 * Absolute paths pass through unchanged. Same approach as stk-ps5's ps5-posix.c. */
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static char g_cwd[PATH_MAX] = "/app0";

/* Copies path into out, resolving "." and ".." by text alone. A relative path is taken
 * against the working directory kept above. */
static int normalise_path(const char *path, char *out, size_t size) {
    size_t len = 0;
    if (path[0] != '/') {
        len = strlen(g_cwd);
        if (len + 1 > size) {
            errno = ENAMETOOLONG;
            return -1;
        }
        memcpy(out, g_cwd, len);
        if (len == 1) len = 0; /* "/" itself: the parts add their own slash */
    }
    out[len] = '\0';

    const char *p = path;
    while (*p) {
        while (*p == '/') p++;
        const char *end = p;
        while (*end && *end != '/') end++;
        size_t part = (size_t)(end - p);

        if (part == 0 || (part == 1 && p[0] == '.')) {
            /* nothing to add */
        } else if (part == 2 && p[0] == '.' && p[1] == '.') {
            while (len > 0 && out[len - 1] != '/') len--;
            if (len > 0) len--; /* the slash before it */
            out[len] = '\0';
        } else {
            if (len + 1 + part + 1 > size) {
                errno = ENAMETOOLONG;
                return -1;
            }
            out[len++] = '/';
            memcpy(out + len, p, part);
            len += part;
            out[len] = '\0';
        }
        p = end;
    }
    if (len == 0) {
        out[0] = '/';
        out[1] = '\0';
    }
    return 0;
}

/* The path to hand to the kernel: path itself when absolute, else resolved into buf.
 * NULL (errno set) when the path is too long; an empty path is passed on for the call to reject. */
static const char *resolve(const char *path, char *buf) {
    if (path == NULL || path[0] == '\0' || path[0] == '/') {
        return path;
    }
    return normalise_path(path, buf, PATH_MAX) == 0 ? buf : NULL;
}

#define RESOLVE(name, path)            \
    char name##_buf[PATH_MAX];         \
    const char *name = resolve(path, name##_buf); \
    if (name == NULL) return -1

int __real_stat(const char *path, struct stat *info);

int __wrap_chdir(const char *path) {
    if (path == NULL || path[0] == '\0') {
        errno = ENOENT;
        return -1;
    }
    char target[PATH_MAX];
    if (normalise_path(path, target, sizeof(target)) != 0) return -1;
    struct stat info;
    if (__real_stat(target, &info) != 0) return -1;
    if (!S_ISDIR(info.st_mode)) {
        errno = ENOTDIR;
        return -1;
    }
    strcpy(g_cwd, target);
    return 0;
}

int __wrap_stat(const char *path, struct stat *info) {
    RESOLVE(p, path);
    return __real_stat(p, info);
}

int __real_open(const char *path, int flags, ...);
int __wrap_open(const char *path, int flags, ...) {
    mode_t mode = 0;
    if (flags & O_CREAT) {
        va_list args;
        va_start(args, flags);
        mode = (mode_t)va_arg(args, int);
        va_end(args);
    }
    RESOLVE(p, path);
    return __real_open(p, flags, mode);
}

FILE *__real_fopen(const char *path, const char *mode);
FILE *__wrap_fopen(const char *path, const char *mode) {
    char buf[PATH_MAX];
    const char *p = resolve(path, buf);
    return p == NULL ? NULL : __real_fopen(p, mode);
}

int __real_mkdir(const char *path, mode_t mode);
int __wrap_mkdir(const char *path, mode_t mode) {
    RESOLVE(p, path);
    return __real_mkdir(p, mode);
}

int __real_rmdir(const char *path);
int __wrap_rmdir(const char *path) {
    RESOLVE(p, path);
    return __real_rmdir(p);
}

int __real_unlink(const char *path);
int __wrap_unlink(const char *path) {
    RESOLVE(p, path);
    return __real_unlink(p);
}

int __real_remove(const char *path);
int __wrap_remove(const char *path) {
    RESOLVE(p, path);
    return __real_remove(p);
}

int __real_rename(const char *from, const char *to);
int __wrap_rename(const char *from, const char *to) {
    RESOLVE(f, from);
    RESOLVE(t, to);
    return __real_rename(f, t);
}

int __real_truncate(const char *path, off_t length);
int __wrap_truncate(const char *path, off_t length) {
    RESOLVE(p, path);
    return __real_truncate(p, length);
}

int __real_chmod(const char *path, mode_t mode);
int __wrap_chmod(const char *path, mode_t mode) {
    RESOLVE(p, path);
    return __real_chmod(p, mode);
}

DIR *__real_opendir(const char *path);
DIR *__wrap_opendir(const char *path) {
    char buf[PATH_MAX];
    const char *p = resolve(path, buf);
    return p == NULL ? NULL : __real_opendir(p);
}

/* realpath needs readlink on every component, which a title cannot use. Paths on the console
 * have no symbolic links the game follows, so the path is normalised as text and checked. */
char *__wrap_realpath(const char *path, char *resolved) {
    if (path == NULL) {
        errno = EINVAL;
        return NULL;
    }
    char buf[PATH_MAX];
    if (normalise_path(path, buf, sizeof(buf)) != 0) return NULL;
    struct stat info;
    if (__real_stat(buf, &info) != 0) return NULL;
    if (resolved == NULL) {
        return strdup(buf);
    }
    strcpy(resolved, buf);
    return resolved;
}
