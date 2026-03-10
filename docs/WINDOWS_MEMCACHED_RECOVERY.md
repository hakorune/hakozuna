# Windows Memcached Recovery

This document is the SSOT for the next Windows app-bench lane after real Redis.

Use it to keep the public recovery scripts separate from the private assets needed
for `memcached` and `memtier_benchmark`.

## Goal

Recover a Windows-native `memcached + memtier_benchmark` lane without committing:

- third-party source drops
- vendor binaries
- raw benchmark logs

## Current Findings

- The paper-facing Linux lane used:
  - `memcached 1.6.40`
  - `memtier_benchmark 2.2.1`
  - `memcached -p 11211 -U 0 -t 4 -m 1024 -l 127.0.0.1`
  - `memtier_benchmark --protocol=memcache_text --threads=4 --clients=50 --ratio=1:1 --data-size=64 --key-maximum=100000 --hide-histogram --test-time=20 --run-count=3`
- The private tree contains:
  - paper logs for `memcached` and `memcached2`
  - `memcached_summary.csv`
  - `libevent-2.1.12-stable.tar.gz`
  - extracted `libevent` directories under the paper archive
- The canonical private recovery path now contains upstream source snapshots for:
  - `memcached 1.6.40`
  - `memtier_benchmark 2.2.1`
- Those recovered trees do not currently include `.sln`, `.vcxproj`, or `CMakeLists.txt` files.

Useful references:

- [private/hakmem/private/paper/RESULTS_20260118.md](/C:/git/hakozuna-win/private/hakmem/private/paper/RESULTS_20260118.md)
- [private/hakmem/docs/paper/ACE-Alloc/paper_full_20260120/summary.md](/C:/git/hakozuna-win/private/hakmem/docs/paper/ACE-Alloc/paper_full_20260120/summary.md)
- [private/hakmem/docs/paper/ACE-Alloc/paper_full_20260120/memcached_summary.csv](/C:/git/hakozuna-win/private/hakmem/docs/paper/ACE-Alloc/paper_full_20260120/memcached_summary.csv)

## Canonical Private Layout

Use these private paths as the current SSOT:

- `private/bench-assets/windows/memcached/source`
- `private/bench-assets/windows/memcached/client`
- `private/bench-assets/windows/memcached/deps`
- `private/raw-results/windows/memcached`

Purpose:

- `source`: `memcached` source tree or prepared Windows port tree
- `client`: `memtier_benchmark` source tree or built client binaries
- `deps`: `libevent`, OpenSSL, zlib, or other third-party inputs
- `raw-results`: local logs, dumps, and benchmark outputs

## Public Script

The current repo-level recovery entrypoint is:

- [win/prepare_win_memcached_private_layout.ps1](/C:/git/hakozuna-win/win/prepare_win_memcached_private_layout.ps1)

## Recommended Order

1. Prepare the canonical private layout
2. Copy any recoverable dependency seeds such as `libevent`
3. Keep upstream source snapshots in `private/bench-assets/windows/memcached/{source,client}`
4. Decide the Windows build box: native MSVC, MSYS2/autotools, or private prebuilt lane
5. Add public build and runner scripts only after the build box is chosen

## Commands

Prepare the private layout:

```powershell
powershell -ExecutionPolicy Bypass -File .\win\prepare_win_memcached_private_layout.ps1
```

Prepare the private layout and copy the paper `libevent` seed when available:

```powershell
powershell -ExecutionPolicy Bypass -File .\win\prepare_win_memcached_private_layout.ps1 -CopyPaperLibevent
```

## Missing Inputs

The following are still missing for a Windows-native lane:

- a chosen Windows build path for `memcached`
- a chosen Windows build path for `memtier_benchmark`
- a decision on whether `libevent` should be reused from the paper snapshot or rebuilt cleanly for Windows

## Output Policy

Keep these public:

- recovery scripts
- summary docs
- small config files

Keep these private:

- source drops and extracted vendor trees
- built third-party binaries
- raw logs and benchmark artifacts

## Current Status

- The Linux paper lane is documented and reproducible in terms of workload settings.
- The Windows private recovery layout is now defined.
- `libevent` seed material exists in the private paper archive.
- Upstream `memcached 1.6.40` and `memtier_benchmark 2.2.1` source snapshots have been recovered into the canonical private layout.
- The current blocker is no longer missing source, but choosing and validating a Windows build box for those sources.

