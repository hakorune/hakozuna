# Windows Memcached Shim

This document is the SSOT for the native Windows platform-shim box for
`memcached`.

## Goal

Keep Windows compatibility concentrated in one place before touching the
upstream server sources broadly.

## Current First-Failure Sequence

Using [win/probe_win_memcached_msvc.ps1](/C:/git/hakozuna-win/win/probe_win_memcached_msvc.ps1):

1. first fatal was `sys/socket.h`
2. after adding a minimal socket compatibility include box, the next fatal was `pthread.h`
3. after adding `pthread.h`, `unistd.h`, `sys/mman.h`, `netinet/tcp.h`, `arpa/inet.h`, and related POSIX shims:
   - [thread.c](/C:/git/hakozuna-win/private/bench-assets/windows/memcached/source/memcached-1.6.40/thread.c) now compile-probes successfully
   - [memcached.c](/C:/git/hakozuna-win/private/bench-assets/windows/memcached/source/memcached-1.6.40/memcached.c) is now blocked at the CLI / daemonization boundary: `SIGHUP`, `SIGUSR1`, `getopt`, `optarg`

Current probe references:

- [private/raw-results/windows/memcached/compile_probe/20260309_212938_memcached_clang_cl.log](/C:/git/hakozuna-win/private/raw-results/windows/memcached/compile_probe/20260309_212938_memcached_clang_cl.log)
- [private/raw-results/windows/memcached/compile_probe/20260309_213007_memcached_clang_cl.log](/C:/git/hakozuna-win/private/raw-results/windows/memcached/compile_probe/20260309_213007_memcached_clang_cl.log)
- [private/raw-results/windows/memcached/compile_probe/20260309_212938_thread_clang_cl.log](/C:/git/hakozuna-win/private/raw-results/windows/memcached/compile_probe/20260309_212938_thread_clang_cl.log)

## Current Compatibility Include Box

The current compatibility headers live in:

- [win/memcached/include/sys/socket.h](/C:/git/hakozuna-win/win/memcached/include/sys/socket.h)
- [win/memcached/include/netinet/in.h](/C:/git/hakozuna-win/win/memcached/include/netinet/in.h)
- [win/memcached/include/netinet/tcp.h](/C:/git/hakozuna-win/win/memcached/include/netinet/tcp.h)
- [win/memcached/include/arpa/inet.h](/C:/git/hakozuna-win/win/memcached/include/arpa/inet.h)
- [win/memcached/include/netdb.h](/C:/git/hakozuna-win/win/memcached/include/netdb.h)
- [win/memcached/include/sys/time.h](/C:/git/hakozuna-win/win/memcached/include/sys/time.h)
- [win/memcached/include/sys/mman.h](/C:/git/hakozuna-win/win/memcached/include/sys/mman.h)
- [win/memcached/include/fcntl.h](/C:/git/hakozuna-win/win/memcached/include/fcntl.h)
- [win/memcached/include/sysexits.h](/C:/git/hakozuna-win/win/memcached/include/sysexits.h)
- [win/memcached/include/config.h](/C:/git/hakozuna-win/win/memcached/include/config.h)
- [win/memcached/include/unistd.h](/C:/git/hakozuna-win/win/memcached/include/unistd.h)

These are intentionally small and only exist to move the compile probe to the
next real boundary.

## pthread Surface Seen In Source

The recovered `memcached 1.6.40` tree currently uses at least these pthread APIs:

- `pthread_attr_init`
- `pthread_cond_destroy`
- `pthread_cond_init`
- `pthread_cond_signal`
- `pthread_cond_timedwait`
- `pthread_cond_wait`
- `pthread_create`
- `pthread_equal`
- `pthread_getspecific`
- `pthread_join`
- `pthread_key_create`
- `pthread_mutex_destroy`
- `pthread_mutex_init`
- `pthread_mutex_lock`
- `pthread_mutex_trylock`
- `pthread_mutex_unlock`
- `pthread_once`
- `pthread_self`
- `pthread_setspecific`

And these pthread types/macros:

- `pthread_t`
- `pthread_mutex_t`
- `pthread_cond_t`
- `pthread_key_t`
- `pthread_once_t`
- `PTHREAD_MUTEX_INITIALIZER`
- `PTHREAD_COND_INITIALIZER`
- `PTHREAD_ONCE_INIT`

## Next Native Shim Box

The next implementation box should be:

1. keep [thread.c](/C:/git/hakozuna-win/private/bench-assets/windows/memcached/source/memcached-1.6.40/thread.c) green under the compile probe
2. decide how to handle the `memcached.c` CLI lane on Windows:
   - shim `getopt` and signal constants
   - or cut a Windows-native `main` / option-parse path for first boot
3. keep daemon-only Unix behavior boxed away from the network / worker-thread port
4. stop again at the next first-fatal boundary instead of widening the port
