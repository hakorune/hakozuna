# Windows B-path Design (HookBox)

Goal: provide a Windows-native hook path that routes malloc/free family to hz3,
with Box Theory boundaries, fail-fast behavior, and ENV on/off toggles.

Status: link-mode HookBox implemented (see `hakozuna/hz3/win/hz3_hook_link.c`,
`hakozuna/hz3/win/hz3_hook_link_cpp.cc`, `hakozuna/hz3/win/build_win_link.ps1`).
Inject-mode (IAT patch) implemented (see `hakozuna/hz3/win/hz3_hook_iat.c`,
`hakozuna/hz3/win/hz3_injector.c`, `hakozuna/hz3/win/build_win_inject.ps1`).

## Constraints

- Windows has no LD_PRELOAD, so we must use link-time interpose or runtime injection.
- Keep hot path clean: the hook boundary must be one place.
- Fail-fast on unsupported or partially hooked states.

## Box layout (boundary = one place)

### Box 1: HookBox (boundary 1 place)
Responsibility: replace allocator entry points and route to hz3.
Boundary API:
- `hz3_hook_init_once()` (one-shot init, fail-fast)
- `hz3_hook_malloc/free/calloc/realloc/posix_memalign/aligned_alloc`
- `hz3_hook_new/delete` (C++ boundary)

### Box 2: ResolverBox
Responsibility: resolve the "next" allocator when fallback is allowed.
- For link mode, this is usually "CRT" or "original" function pointer.
- For inject mode, resolve via IAT or detours.

### Box 2.5: ModuleScanBox
Responsibility: optional module enumeration for IAT patching beyond the main EXE.
- Enumerate all loaded modules and patch their IAT when enabled.
- Must run outside DllMain (worker thread) to avoid loader lock.
- Fail-fast only on main module (malloc/free), DLLs are stats only.

### Box 2.6: SyncBox (injector handshake)
Responsibility: ensure IAT patch completes before the target starts executing.
- Injector creates a named event and sets `HZ3_HOOK_IAT_READY`.
- DLL signals the event after patching is complete (worker thread).
- Injector waits before resuming the main thread; timeout is fail-fast.

### Box 3: SwitchBox (ENV)
Responsibility: allow quick rollback.
Suggested ENV:
- `HZ3_HOOK_ENABLE=1/0`
- `HZ3_HOOK_MODE=link|inject`
- `HZ3_HOOK_FAILFAST=1` (abort on hook failure)
- `HZ3_HOOK_LOG_SHOT=1` (one-shot log at init)
- `HZ3_HOOK_IAT_ALL_MODULES=1` (patch all DLLs)

### Box 4: FailFastBox
Responsibility: abort on:
- hook failure
- unknown entry point
- mismatched CRT version

### Box 5: ExportBox (DLL surface)
Responsibility: export standard allocator symbols.
Exports:
- C: `malloc`, `free`, `calloc`, `realloc`, `posix_memalign`, `aligned_alloc`, `malloc_usable_size`
- C++: `operator new`, `operator delete`, `operator new[]`, `operator delete[]`

## Two implementation routes

### 1) Link mode (shortest path, deterministic)
Target: bench EXE or known test program.

Approach:
- Build `hz3_win.lib` (already done in A-path).
- Add a small "shim obj" that defines the allocator symbols and forwards to hz3.
- Link the bench EXE against the shim first so the symbols resolve to hz3.

Notes:
- MSVC/clang-cl may need `/FORCE:multiple` or `/alternatename` depending on CRT.
- For controlled benches, direct `hz3_malloc/free` calls are acceptable for sanity.

Fail-fast:
- If shim cannot resolve expected symbols, abort immediately.

### 2) Inject mode (full route)
Target: arbitrary Windows EXEs.

Options:
- Manual IAT patch (no external deps) - implemented.
- Detours (Microsoft Research) for import patching (future).
- MinHook for function detouring (future).

Notes:
- Keep the "HookBox boundary" in one file (no scattered hooks).
- Hook must be enabled only if `HZ3_HOOK_ENABLE=1`.
- If any required symbol is missing, abort (fail-fast).

## Minimal verification (B-path)

1) Build DLL or shim library.
2) Run `bench_minimal` via B-path.
3) Check:
   - same output as A-path (ops/s within margin)
   - `HZ3_WIN_MMAN` stats are low (single digits/teens)

## Rollback plan

- `HZ3_HOOK_ENABLE=0` returns to baseline allocator immediately.
- Keep A-path build as reference for comparisons.

## Next steps

- Expand inject mode to patch additional modules or C++ new/delete if needed.
- Keep link mode as deterministic baseline for A/B checks.
