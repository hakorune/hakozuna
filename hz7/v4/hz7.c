#include "hz7.h"

#include <stdint.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sched.h>
#include <sys/mman.h>
#include <unistd.h>
#ifndef MAP_ANONYMOUS
#ifdef MAP_ANON
#define MAP_ANONYMOUS MAP_ANON
#elif defined(__linux__)
#define MAP_ANONYMOUS 0x20
#endif
#endif
#endif

#include "hz7_state.inc"

#include "../common/hz7_common.inc"
