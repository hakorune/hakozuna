# Windows Memcached libevent

This document is the SSOT for the first native MSVC dependency box for the
Windows `memcached` port.

## Goal

Provide a Windows-native `libevent` dependency for the future MSVC build of
`memcached.exe`.

For the first box:

- use `vcpkg`
- require thread support
- keep OpenSSL out of scope

## Public Script

- [win/prepare_win_memcached_libevent.ps1](/C:/git/hakozuna-win/win/prepare_win_memcached_libevent.ps1)

## Recommended Package

Use:

- `libevent[thread]:x64-windows`

This is the smallest useful Windows package for the first native `memcached`
bring-up because worker-thread support needs `event2/thread.h` and
`evthread_use_windows_threads()`.

## Commands

Check whether the dependency is ready:

```powershell
powershell -ExecutionPolicy Bypass -File .\win\prepare_win_memcached_libevent.ps1
```

Install it through `vcpkg` if missing:

```powershell
powershell -ExecutionPolicy Bypass -File .\win\prepare_win_memcached_libevent.ps1 -InstallIfMissing
```

## Expected Outputs

Under `C:\vcpkg\installed\x64-windows`:

- headers:
  - `include\event.h`
  - `include\event2\event.h`
  - `include\event2\thread.h`
- import libraries:
  - `lib\event.lib`
  - `lib\event_core.lib`
  - `lib\event_extra.lib`

At runtime, the matching DLLs may also exist under:

  - `bin\event.dll`
- `bin\event_core.dll`
- `bin\event_extra.dll`

## Why This Box Exists

The native `memcached` port should not begin by hand-copying old paper
snapshots or mixing Linux-built artifacts into the Windows lane.

The first dependency box should be:

- reproducible
- scriptable from the repo root
- separable from the later platform shim and server build boxes

## Current Status

- the private paper archive still contains a reusable `libevent` source snapshot
- the preferred native Windows dependency path is now `vcpkg` for the first bring-up box
- OpenSSL is intentionally deferred until after the first native `memcached.exe` boot
