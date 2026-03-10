#pragma once

#include <stddef.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <errno.h>

static inline int hz_memcached_uio_wsa_to_errno(int wsa_error);

struct iovec {
    void* iov_base;
    size_t iov_len;
};

struct msghdr {
    void* msg_name;
    int msg_namelen;
    struct iovec* msg_iov;
    int msg_iovlen;
    void* msg_control;
    int msg_controllen;
    int msg_flags;
};

static inline int writev(SOCKET s, const struct iovec* iov, int iovcnt) {
    if (iov == NULL || iovcnt <= 0 || iovcnt > 64) {
        WSASetLastError(WSAEINVAL);
        return -1;
    }

    WSABUF buffers[64];
    for (int i = 0; i < iovcnt; ++i) {
        buffers[i].buf = (CHAR*)iov[i].iov_base;
        buffers[i].len = (ULONG)iov[i].iov_len;
    }

    DWORD sent = 0;
    if (WSASend(s, buffers, (DWORD)iovcnt, &sent, 0, NULL, NULL) == SOCKET_ERROR) {
        errno = hz_memcached_uio_wsa_to_errno(WSAGetLastError());
        return -1;
    }
    return (int)sent;
}
static inline int hz_memcached_uio_wsa_to_errno(int wsa_error) {
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
