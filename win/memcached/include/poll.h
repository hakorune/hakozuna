#pragma once

#if !defined(_WIN32)
#error "poll shim is Windows-only"
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>

typedef WSAPOLLFD pollfd;
typedef ULONG nfds_t;

#ifndef POLLIN
#define POLLIN POLLRDNORM
#endif

#ifndef POLLOUT
#define POLLOUT POLLWRNORM
#endif

#ifndef poll
#define poll WSAPoll
#endif
