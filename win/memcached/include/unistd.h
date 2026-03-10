#pragma once

#if !defined(_WIN32)
#error "unistd shim is Windows-only"
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <io.h>
#include <process.h>

typedef intptr_t ssize_t;
typedef int pid_t;
typedef unsigned int useconds_t;

#ifndef _SC_PAGESIZE
#define _SC_PAGESIZE 30
#endif

#ifndef F_OK
#define F_OK 0
#endif

#ifndef PATH_MAX
#define PATH_MAX MAX_PATH
#endif

static inline long sysconf(int name) {
    if (name != _SC_PAGESIZE) {
        return -1;
    }
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    return (long)info.dwPageSize;
}

static inline int usleep(useconds_t usec) {
    Sleep((DWORD)((usec + 999) / 1000));
    return 0;
}

static inline unsigned int sleep(unsigned int seconds) {
    Sleep((DWORD)(seconds * 1000U));
    return 0;
}

static inline void srandom(unsigned int seed) {
    srand(seed);
}

static inline long random(void) {
    return (long)rand();
}

static inline int getuid(void) {
    return 0;
}

static inline int kill(pid_t pid, int sig) {
    (void)sig;
    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, (DWORD)pid);
    if (process == NULL) {
        return -1;
    }
    CloseHandle(process);
    return 0;
}

static inline int hz_memcached_wsa_to_errno(int wsa_error) {
    switch (wsa_error) {
        case WSAEWOULDBLOCK:
        case WSAENOBUFS:
            return EAGAIN;
        case WSAEINTR:
            return EINTR;
        case WSAEINVAL:
            return EINVAL;
        case WSAEALREADY:
        case WSAEINPROGRESS:
            return EALREADY;
        case WSAEMSGSIZE:
            return EMSGSIZE;
        case WSAECONNRESET:
            return ECONNRESET;
        case WSAENOTCONN:
            return ENOTCONN;
        case WSAECONNABORTED:
            return ECONNABORTED;
        case WSAESHUTDOWN:
            return EPIPE;
        case WSAETIMEDOUT:
            return ETIMEDOUT;
        default:
            return EIO;
    }
}

static inline int hz_memcached_is_socket_fd(int fd) {
    int socket_type = 0;
    int socket_len = (int)sizeof(socket_type);
    return getsockopt((SOCKET)(uintptr_t)fd, SOL_SOCKET, SO_TYPE,
        (char *)&socket_type, &socket_len) == 0;
}

static inline int hz_memcached_close(int fd) {
    if (hz_memcached_is_socket_fd(fd)) {
        if (closesocket((SOCKET)(uintptr_t)fd) == 0) {
            return 0;
        }
        errno = hz_memcached_wsa_to_errno(WSAGetLastError());
        return -1;
    }
    return _close(fd);
}

static inline ssize_t hz_memcached_read(int fd, void *buf, size_t count) {
    if (hz_memcached_is_socket_fd(fd)) {
        int rc = recv((SOCKET)(uintptr_t)fd, (char *)buf, (int)count, 0);
        if (rc == SOCKET_ERROR) {
            errno = hz_memcached_wsa_to_errno(WSAGetLastError());
            return -1;
        }
        return (ssize_t)rc;
    }
    return (ssize_t)_read(fd, buf, (unsigned int)count);
}

static inline ssize_t hz_memcached_write(int fd, const void *buf, size_t count) {
    if (hz_memcached_is_socket_fd(fd)) {
        int rc = send((SOCKET)(uintptr_t)fd, (const char *)buf, (int)count, 0);
        if (rc == SOCKET_ERROR) {
            errno = hz_memcached_wsa_to_errno(WSAGetLastError());
            return -1;
        }
        return (ssize_t)rc;
    }
    return (ssize_t)_write(fd, buf, (unsigned int)count);
}

#define access _access
#define close  hz_memcached_close
#define dup    _dup
#define pipe(fds) _pipe((fds), 4096, _O_BINARY)
#define read   hz_memcached_read
#define write  hz_memcached_write
