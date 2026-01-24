# Windows Build (A-path: static + minimal bench)

This is the first-step Windows bring-up for `hz3`. It builds a static library and a minimal bench
that calls `hz3_malloc/free` directly (no LD_PRELOAD).

## Prerequisites

- LLVM `clang-cl` in PATH
- Visual Studio Build Tools (for `lib.exe`) or LLVM `llvm-lib`

## Build

From the repo root:

```powershell
.\hakozuna\hz3\win\build_win_min.ps1
```

If PowerShell execution policy blocks the script:

```powershell
powershell -ExecutionPolicy Bypass -File .\hakozuna\hz3\win\build_win_min.ps1
```

Minimal config (forward-only fallback; fastest for sanity checks):

```powershell
.\hakozuna\hz3\win\build_win_min.ps1 -Minimal
```

Link-mode hook (B-path: link the hook shim into the bench):

```powershell
.\hakozuna\hz3\win\build_win_link.ps1
```

Inject-mode hook (B-path: IAT patch DLL + injector):

```powershell
.\hakozuna\hz3\win\build_win_inject.ps1
```

Outputs:
- `hakozuna\hz3\out_win\hz3_win.lib`
- `hakozuna\hz3\out_win\bench_minimal.exe`

Minimal outputs:
- `hakozuna\hz3\out_win_min\hz3_win.lib`
- `hakozuna\hz3\out_win_min\bench_minimal.exe`

Link-mode outputs:
- `hakozuna\hz3\out_win_link\hz3_win.lib`
- `hakozuna\hz3\out_win_link\bench_minimal_link.exe`

Inject-mode outputs:
- `hakozuna\hz3\out_win_inject\hz3_hook_iat.dll`
- `hakozuna\hz3\out_win_inject\hz3_injector.exe`
- `hakozuna\hz3\out_win_inject\bench_minimal_inject.exe`

## Run (minimal bench)

```powershell
.\hakozuna\hz3\out_win\bench_minimal.exe 4 1000000 16 1024
```

Args: `[threads] [iters_per_thread] [min_size] [max_size]`

## Notes

- This is **A-path only** (static link, minimal bench).
- Windows shims live in `hakozuna\hz3\win\include` (pthread/mmap/dlfcn/unistd/etc).
- `madvise(MADV_DONTNEED)` is mapped to `VirtualFree(..., MEM_DECOMMIT)` (approximate).
- LD_PRELOAD equivalents (DLL injection / malloc hooks) are **B-path** and not covered here.
- Inject bench is built with `/MD` and `-fno-builtin-malloc/-fno-builtin-realloc` to ensure IAT entries; if `realloc` shows `0` hits, add a realloc call to the bench or verify flags.
- Optional: enable Windows mmap stats (atexit one-shot)

```powershell
.\hakozuna\hz3\win\build_win_min.ps1 -MmanStats
```

Link-mode + mmap stats:

```powershell
.\hakozuna\hz3\win\build_win_link.ps1 -MmanStats
```

Inject-mode + mmap stats:

```powershell
.\hakozuna\hz3\win\build_win_inject.ps1 -MmanStats
```

Inject-mode run (IAT patch via injector):

```powershell
.\hakozuna\hz3\out_win_inject\hz3_injector.exe `
  .\hakozuna\hz3\out_win_inject\hz3_hook_iat.dll `
  .\hakozuna\hz3\out_win_inject\bench_minimal_inject.exe 4 1000000 16 1024
```

Hook ENV (link mode):
- `HZ3_HOOK_ENABLE=1/0`
- `HZ3_HOOK_FAILFAST=1`
- `HZ3_HOOK_LOG_SHOT=1`

Hook ENV (inject mode):
- `HZ3_HOOK_ENABLE=1/0`
- `HZ3_HOOK_FAILFAST=1`
- `HZ3_HOOK_LOG_SHOT=1`
- `HZ3_HOOK_IAT_ALL_MODULES=1` (patch all loaded DLLs; default is main module only)
- `HZ3_HOOK_IAT_READY` (injector-only: named event for patch-ready sync)

## Bench compare (CRT/hz3/mimalloc/tcmalloc)

Requires vcpkg (mimalloc + gperftools):

```powershell
vcpkg install mimalloc:x64-windows gperftools:x64-windows
```

Build all bench variants:

```powershell
.\hakozuna\hz3\win\build_win_bench_compare.ps1
```

Outputs:
- `hakozuna\hz3\out_win_bench\bench_mixed_ws_crt.exe`
- `hakozuna\hz3\out_win_bench\bench_mixed_ws_hz3.exe`
- `hakozuna\hz3\out_win_bench\bench_mixed_ws_mimalloc.exe`
- `hakozuna\hz3\out_win_bench\bench_mixed_ws_tcmalloc.exe`

Note: mimalloc needs `mimalloc-redirect.dll` next to the exe (script copies it).
