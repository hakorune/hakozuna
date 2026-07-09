# HZ11 Windows Bring-up L0

Status: active Windows bring-up scaffold.

HZ11 is currently a Linux-first, speed-first research line. The Linux evidence
is centered on the `fine128` opt-in lane:

```text
recommended Linux opt-in lane:
  libhz11_span_transfer_thread_exit_cap_batch32_fine128.so

real-app posture:
  redis build: faster than tcmalloc with lower RSS
  espresso: near-parity wall with much lower RSS
  redis server / rocksdb: near-parity or small tradeoff
  sqlite3: slower with lower RSS
  git repack: large-buffer weakness
```

Windows support should start with a small, honest bring-up ladder. Do not try
to port the Linux `LD_PRELOAD` shim, span-transfer lane, or fine128 macro claim
in one step.

## L0 Scope

```text
Goal:
  Build and run the HZ11 standalone token/front-cache smoke on Windows.

Build shape:
  HZ11_CLASSIFY_SPAN=0
  no LD_PRELOAD shim
  no span arena
  no transfer cache
  no pthread thread-exit destructor
  no fine128 claim

Why:
  This checks the public entry, token table, thread-local cache, realloc path,
  large passthrough, and stats contract without mixing in POSIX-only backing.
```

## Non-goals

```text
Not in L0:
  fine128 Windows parity
  Windows DLL interposition
  malloc replacement
  remote/mixed performance claim
  FLS thread-exit current-span salvage
  VirtualAlloc-backed span arena
```

## Portability Work

```text
1. Thread-local macro:
   Replace direct `_Thread_local` spelling in HZ11 public state with
   `HZ11_THREAD_LOCAL`.

2. System allocation fallback:
   On Windows standalone builds, `hz11_sys_*` calls CRT malloc/free/realloc/
   calloc directly and reports `_msize` for usable size.

3. Build script:
   `scripts/build_hz11_win64_smoke.ps1` builds only the standalone token lane:
     hz11_size_class.c
     hz11_sys_alloc.c
     hz11_thread_cache.c
     hz11_public_entry.c
     tests/hz11_thread_cache_smoke.c
```

## Next After L0

```text
L1:
  Add a Windows platform layer for locks/TLS/once.

L2:
  Add VirtualAlloc-backed span arena and classify-span smoke.

L3:
  Add a Windows DLL/provider route. This is not LD_PRELOAD parity; it needs a
  Windows-specific API/interposition story.

L4:
  Measure Windows synthetic lanes before attempting real-app claims.
```

## Acceptance

```text
accept:
  build_hz11_win64_smoke.ps1 succeeds with clang-cl
  hz11_thread_cache_smoke_win.exe exits 0
  no changes to Linux default/fine128 build flags

no-go:
  L0 requires pthread, mmap, dlsym, or LD_PRELOAD behavior
  L0 claims fine128 performance parity
  Windows code changes alter Linux evidence lanes
```
