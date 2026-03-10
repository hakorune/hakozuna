#pragma once

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS 1
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/uio.h>

#define DISABLE_UNIX_SOCKET 1
#define ENDIAN_LITTLE 1
#define HAVE_HTONLL 1
#define IOV_MAX 1024
#define PACKAGE "memcached"
#define VERSION "1.6.40"
#define getpid _getpid
#define strdup _strdup
#define strtok_r strtok_s

#ifndef SIGHUP
#define SIGHUP SIGTERM
#endif

#ifndef SIGUSR1
#define SIGUSR1 SIGTERM
#endif

/* Windows native bring-up keeps these feature boxes closed at first. */
#undef ENABLE_SASL
#undef EXTSTORE
#undef PROXY
#undef TLS

/* Keep optional Unix/Linux probes off during the first native compile box. */
#undef HAVE_EVENTFD
#undef HAVE_GETOPT_LONG
#undef HAVE_PTHREAD_SETNAME_NP
