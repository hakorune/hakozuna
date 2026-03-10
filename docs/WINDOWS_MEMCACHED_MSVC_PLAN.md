# Windows Memcached Native MSVC Plan

This document is the SSOT for the proper native Windows path for the
`memcached + memtier_benchmark` lane.

The goal is a real MSVC-driven Windows lane, not an MSYS2 compatibility lane.

## Decision

Current direction: prefer a proper native MSVC port.

Do this in small boxes:

1. native `memcached.exe` first
2. native benchmark client second
3. allocator comparison only after both are stable

## Why This Order

- `memcached` is the server under test and the harder portability boundary.
- `memtier_benchmark` adds its own portability work: C++, pthreads, libevent, OpenSSL, autotools.
- If we port both at once, we will not know whether failures belong to the server, the client, or the build environment.

## Smallest Viable First Target

First target:

- `memcached.exe`
- TCP only
- ASCII protocol only is acceptable for first bring-up
- no TLS
- no SASL
- no proxy
- no extstore
- no unix socket

Keep the first native Windows box as small as possible.

## Current Source Facts

- `memcached 1.6.40` source is present in private:
  - [private/bench-assets/windows/memcached/source/memcached-1.6.40](/C:/git/hakozuna-win/private/bench-assets/windows/memcached/source/memcached-1.6.40)
- `memtier_benchmark 2.2.1` source is present in private:
  - [private/bench-assets/windows/memcached/client/memtier_benchmark-2.2.1](/C:/git/hakozuna-win/private/bench-assets/windows/memcached/client/memtier_benchmark-2.2.1)
- Neither tree currently ships a native Windows build file in this snapshot:
  - no `.sln`
  - no `.vcxproj`
  - no `CMakeLists.txt`

## Main Native Port Boxes

### Box 1: Dependency Box

Prepare Windows-native third-party inputs:

- libevent for Windows
- optional OpenSSL later, but not for first bring-up

For the first box, `libevent` is mandatory and OpenSSL stays out.

### Box 2: Header/Platform Shim Box

Memcached currently assumes:

- `pthread.h`
- `unistd.h`
- POSIX sockets headers
- `sys/time.h`
- `sys/uio.h`
- direct `event.h`

The first compile probe has already narrowed this box to:

- socket compatibility first
- `pthread.h` second

The native port needs one concentrated platform boundary for:

- threads and mutexes
- condition variables
- wakeup pipes or equivalent
- socket typedefs and startup
- `iovec` / scatter-gather compatibility
- time helpers

Do not scatter `_WIN32` edits everywhere if a small shim can absorb them.

### Box 3: Minimal Server Build Box

Create a native MSVC project for:

- `memcached.c`
- core support `.c` files needed for TCP + ASCII + worker threads
- Windows libevent and Winsock linkage

Disable features aggressively until first boot:

- `TLS`
- `ENABLE_SASL`
- `EXTSTORE`
- `PROXY`
- unix socket paths

Current checkpoint inside Box 3:

- worker-thread / libevent notification code is now compile-probing on Windows
- the next concentrated blocker is the Unix command-line / daemonization path in [memcached.c](/C:/git/hakozuna-win/private/bench-assets/windows/memcached/source/memcached-1.6.40/memcached.c):
  - `SIGHUP`
  - `SIGUSR1`
  - `getopt`
  - `optarg`

Treat this as its own box instead of mixing it into the network/thread port.

That box now has a first Windows-native answer:

- [win/memcached/memcached_win_min_main.c](/C:/git/hakozuna-win/win/memcached/memcached_win_min_main.c)
- [docs/WINDOWS_MEMCACHED_MIN_MAIN.md](/C:/git/hakozuna-win/docs/WINDOWS_MEMCACHED_MIN_MAIN.md)

Current status:

- the wrapper compile-probes successfully
- the next blocker has moved from CLI parsing to first link-stage build

### Box 4: First Boot Box

Success criteria:

- `memcached.exe -p 11211 -U 0 -t 4 -m 1024 -l 127.0.0.1`
- process starts
- accepts a basic ASCII `set/get`
- survives a short local smoke test

### Box 5: Native Client Box

Only after Box 4 passes:

- decide whether to port full `memtier_benchmark` to MSVC
- or build a smaller native memcache-text benchmark client first as a bring-up tool

For correctness, a tiny native memcache-text client is acceptable before the full paper-grade client.

### Box 6: Paper Lane Box

After both native server and client are stable:

- restore the paper-like workload
- record summary only
- keep raw logs private

## Recommendation

Recommended immediate next implementation box:

1. make a native Windows `libevent` dependency box
2. add a tiny local platform shim for `memcached`
3. build the smallest native `memcached.exe`

This is the best path if the goal is a real Windows lane that can be maintained later.
