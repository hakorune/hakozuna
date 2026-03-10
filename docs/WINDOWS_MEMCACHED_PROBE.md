# Windows Memcached MSVC Probe

This document is the SSOT for the compile-probe box before editing the native
Windows memcached shim.

## Goal

Fail fast and collect the first MSVC compile errors without widening the port.

Use this probe to identify the first missing Windows boundary for a specific
translation unit.

## Public Script

- [win/probe_win_memcached_msvc.ps1](/C:/git/hakozuna-win/win/probe_win_memcached_msvc.ps1)

## Recommended Order

1. Prepare `libevent` with [win/prepare_win_memcached_libevent.ps1](/C:/git/hakozuna-win/win/prepare_win_memcached_libevent.ps1)
2. Probe `memcached.c`
3. Probe `thread.c`
4. Create the smallest possible shim only after the first failure is clear

## Current Result

As of 2026-03-09:

- [thread.c](/C:/git/hakozuna-win/private/bench-assets/windows/memcached/source/memcached-1.6.40/thread.c) compile-probes successfully with one Windows `getsockopt()` pointer-type warning:
  - [private/raw-results/windows/memcached/compile_probe/20260309_212938_thread_clang_cl.log](/C:/git/hakozuna-win/private/raw-results/windows/memcached/compile_probe/20260309_212938_thread_clang_cl.log)
- [memcached.c](/C:/git/hakozuna-win/private/bench-assets/windows/memcached/source/memcached-1.6.40/memcached.c) now fails later in the bring-up path, at the Unix CLI/signal box:
  - `SIGHUP`
  - `SIGUSR1`
  - `getopt`
  - `optarg`
  - current log: [private/raw-results/windows/memcached/compile_probe/20260309_213007_memcached_clang_cl.log](/C:/git/hakozuna-win/private/raw-results/windows/memcached/compile_probe/20260309_213007_memcached_clang_cl.log)
- the Windows-native minimal-main wrapper also compile-probes successfully:
  - [private/raw-results/windows/memcached/compile_probe/20260309_220542_memcached_win_min_main_clang_cl.log](/C:/git/hakozuna-win/private/raw-results/windows/memcached/compile_probe/20260309_220542_memcached_win_min_main_clang_cl.log)

## Commands

Probe the main server translation unit:

```powershell
powershell -ExecutionPolicy Bypass -File .\win\probe_win_memcached_msvc.ps1 -TranslationUnit memcached.c
```

Probe the worker-thread translation unit:

```powershell
powershell -ExecutionPolicy Bypass -File .\win\probe_win_memcached_msvc.ps1 -TranslationUnit thread.c
```

## Output Policy

Keep probe scripts public.

Keep probe logs private under:

- `private\raw-results\windows\memcached\compile_probe`
