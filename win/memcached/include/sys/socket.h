#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <sys/uio.h>
#include <errno.h>

#ifndef SHUT_RD
#define SHUT_RD SD_RECEIVE
#endif
#ifndef SHUT_WR
#define SHUT_WR SD_SEND
#endif
#ifndef SHUT_RDWR
#define SHUT_RDWR SD_BOTH
#endif

#ifndef MSG_DONTWAIT
#define MSG_DONTWAIT 0
#endif

static inline int hz_memcached_socket_wsa_to_errno(int wsa_error);

static inline int hz_setsockopt(SOCKET s, int level, int optname, const void* optval, int optlen) {
    return setsockopt(s, level, optname, (const char*)optval, optlen);
}

static inline int hz_getsockopt(SOCKET s, int level, int optname, void* optval, int* optlen) {
    return getsockopt(s, level, optname, (char*)optval, optlen);
}

static inline int sendmsg(SOCKET s, const struct msghdr* msg, int flags) {
    if (msg == NULL || msg->msg_iov == NULL || msg->msg_iovlen <= 0 || msg->msg_iovlen > 64) {
        WSASetLastError(WSAEINVAL);
        return -1;
    }

    WSABUF buffers[64];
    for (int i = 0; i < msg->msg_iovlen; ++i) {
        buffers[i].buf = (CHAR*)msg->msg_iov[i].iov_base;
        buffers[i].len = (ULONG)msg->msg_iov[i].iov_len;
    }

    DWORD sent = 0;
    int rc = WSASend(s, buffers, (DWORD)msg->msg_iovlen, &sent, (DWORD)flags, NULL, NULL);
    if (rc == SOCKET_ERROR) {
        errno = hz_memcached_socket_wsa_to_errno(WSAGetLastError());
        return -1;
    }
    return (int)sent;
}

#define setsockopt hz_setsockopt
#define getsockopt hz_getsockopt
static inline int hz_memcached_socket_wsa_to_errno(int wsa_error) {
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
