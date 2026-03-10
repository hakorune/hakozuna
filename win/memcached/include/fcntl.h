#pragma once

#if !defined(_WIN32)
#error "fcntl shim is Windows-only"
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdarg.h>
#include <io.h>

#ifndef _O_BINARY
#define _O_BINARY 0x8000
#endif

#ifndef O_NONBLOCK
#define O_NONBLOCK 0x0004
#endif

#ifndef F_GETFL
#define F_GETFL 3
#endif

#ifndef F_SETFL
#define F_SETFL 4
#endif

static inline int fcntl(int fd, int cmd, ...) {
    va_list args;
    va_start(args, cmd);

    int rc = 0;
    switch (cmd) {
        case F_GETFL:
            rc = 0;
            break;
        case F_SETFL: {
            long flags = va_arg(args, long);
            u_long nonblocking = ((flags & O_NONBLOCK) != 0) ? 1UL : 0UL;
            rc = ioctlsocket((SOCKET)fd, FIONBIO, &nonblocking);
            break;
        }
        default:
            rc = -1;
            break;
    }

    va_end(args);
    return rc;
}
