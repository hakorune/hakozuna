# HZ8 Windows Bring-Up

This note defines the first Windows port lane for HZ8.  The goal is to bring
up a clean Win64 direct-API allocator build without weakening the current Linux
v1.1 default evidence line.

## Positioning

```text
Linux:
  current default / evidence platform
  HZ8 v1.1 remains frozen except hard safety fixes

Windows:
  bring-up platform
  direct API first
  no public parity or default claim yet
```

The first Windows box must not reopen Linux small-v0, MediumRun-v1.1, lazy128,
owner queue, or preload ABI policy decisions.

## First Box

Name:

```text
PlatformAbstraction-Win64-L1
```

Scope:

```text
- Win64 buildable source tree
- direct API only:
    h8_malloc
    h8_free
    h8_calloc
    h8_realloc
    h8_usable_size
- smoke correctness for MISS / VALID / INVALID
- small local alloc/free
- small remote-safe free
- medium local correctness
- adoption / orphan-span optimization is deferred;
  first Win64 smoke may run with regular adoption disabled by default
```

Non-goals:

```text
- no CRT replacement claim
- no Detours/AppInit/global override surface
- no preload-equivalent interpose layer
- no Windows-specific throughput tuning
- no Linux parity benchmark claim
```

## Platform Surfaces To Abstract

The current Linux tree uses these platform-specific mechanisms directly:

```text
- mmap / munmap
- madvise
- pthread_once
- pthread_key_create / getspecific / setspecific
- pthread_mutex
- sched_yield
- clock_gettime(CLOCK_MONOTONIC)
- dlsym(RTLD_NEXT)
```

The Windows bring-up lane should replace those direct calls with a small
platform layer and provide Win64 backends for:

```text
- reserve / commit / release
- purge-or-noop hook
- once
- thread-local destructor key
- mutex
- yield
- monotonic time
```

## Windows Mapping

Initial mapping target:

```text
reserve / commit / release:
  VirtualAlloc / VirtualFree

purge hook:
  MEM_RESET or noop

once:
  InitOnceExecuteOnce or a minimal fallback

thread key:
  FlsAlloc/FlsGetValue/FlsSetValue preferred
  TlsAlloc fallback only if destructor behavior is solved separately

mutex:
  SRWLOCK or CRITICAL_SECTION

yield:
  SwitchToThread or Sleep(0)

time:
  QueryPerformanceCounter
```

The first box does not need a preload-equivalent.  `dlsym(RTLD_NEXT)` should be
kept behind a Linux-only boundary.

## Safety Contract

Windows bring-up must preserve the same ownership contract:

```text
- owned VALID pointer frees through HZ8
- owned-looking invalid pointer does not fall to platform free
- interior pointer stays INVALID
- remote-safe free is correct even if not yet throughput-optimized
- owner exit remains the hard drain point
```

## Bring-Up Gates

The first box passes only if:

```text
- Win64 build succeeds
- direct API smoke passes
- remote-safe smoke passes
- INVALID / double-free checks stay fail-closed
- no owned-looking invalid falls to platform free
```

Throughput and RSS comparison come later.  The first milestone is correctness
and architectural cleanliness, not benchmark parity.

## Current Bring-Up Hook

Win64 direct-API smoke is now wired through:

```text
scripts/build_hz8_win64_smoke.ps1
```

Current expectation:

```text
- builds h8_smoke_win.exe with clang-cl
- runs direct-API smoke on Win64
- keeps regular adoption disabled by default for this first box
```
