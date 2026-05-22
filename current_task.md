# Current Task: HZ5 Ubuntu Standalone Exact Lane

## Goal

Build an Ubuntu/Linux development lane for HZ5 that can be moved to native
Ubuntu later for paper-facing measurement.

The target lane is standalone and fallback-free:

- do not merge to `main`
- do not depend on HZ3 fallback
- do not use process-wide `LD_PRELOAD` as the primary HZ5 measurement path
- call the HZ5 API directly from a dedicated benchmark binary
- route only supported exact HZ5 cases, starting with `64K align=8192`

## Branch

Use:

```bash
codex/hz5-linux-p43-port
```

Current reference commit:

```bash
931c63b Add HZ5 Linux standalone exact lane
```

Parent branch:

```bash
codex/hz3-rss-largepath-research
```

## Phase 1: WSL2 Development

Status: initial implementation complete.

Purpose:

- make Linux compilation work
- add a standalone HZ5 build entrypoint
- add a direct HZ5 benchmark entrypoint
- keep commands reproducible for later native Ubuntu rerun

The WSL2 numbers are not paper results.

## Phase 2: Native Ubuntu Measurement

Status: initial P25 measurement complete; Linux P43 port in progress.

Purpose:

- rerun the same commands on native Ubuntu
- use `RUNS=10` median
- compare against glibc, mimalloc, tcmalloc, hz3, and hz4 where the workload is
  semantically comparable

Native Ubuntu first-pass result:

- `64K align=8192` local-only measured the HZ5 P25 lowpage64 standalone speed
  lane, not the Windows P43i/P45 lane.
- HZ5 P25 was above glibc/HZ3/mimalloc but below HZ4/tcmalloc on the local-only
  microbench.
- Probe confirmed `source=5` (`HZ5_WRAPPER_SOURCE_P25_HZ4LOWPAGE`) and
  fail-closed unsupported routes.
- P43 segment-slot source was not active on Linux before this branch.

Raw result folders:

```bash
private/raw-results/linux/hz5_ubuntu_groupA_20260523_012604
private/raw-results/linux/hz5_ubuntu_groupB_20260523_012655
private/raw-results/linux/hz5_ubuntu_groupC_20260523_012859
```

## Phase 3: Linux P43 Candidate Port

Status: initial candidate/control implemented.

Goal:

- port the P43 segment-slot source layer to Linux without weakening or renaming
  the existing Windows P43i/P45 lane
- preserve Windows `VirtualAlloc` / `VirtualFree` behavior
- add Linux OS backend behavior under explicit Linux build flags
- keep P25 bridge / PreparedBridge / FastLookup semantics separable
- keep unsupported routes fail-closed
- do not enable decommit or runtime release in the first Linux port

Implemented:

- `hakozuna-hz5/lowpage/hz5_lowpage64_p43_segment.c`
  - keeps Windows backend on `VirtualAlloc` / `VirtualFree`
  - adds Linux backend using `mmap` / `munmap`
  - adds Linux pthread mutex path for P43 segment lock
  - keeps Linux commit as a no-op for the first candidate
  - keeps Linux decommit disabled for the first candidate
- `hakozuna-hz5/lowpage/hz5_lowpage64.c`
  - allows `HZ5_LOWPAGE64_P43_SEGMENT_SLOTS` to call the P43 source on Linux
- `linux/build_linux_hz5_standalone.sh`
  - `--linux-p43`
  - `--linux-p43-unsafe-no-lookup`
  - `--linux-p43-trust-fast-lookup`
  - `--linux-p43-trust-wrapper-source`
  - `--linux-p43-no-prepared-bridge`
- `linux/run_linux_hz5_p43_ab.sh`
  - builds/runs six A/B lanes:
    - `p25`
    - `p43`
    - `p43-trustfast`
    - `p43-trustwrap`
    - `p43-nolookup`
    - `p43-source`

Verification:

```bash
./linux/build_linux_hz5_standalone.sh --arch x86_64 \
  --linux-p43 --out-dir hakozuna-hz5/out/linux/x86_64-p43

./hakozuna-hz5/out/linux/x86_64-p43/bench_hz5_standalone_aligned64k \
  1 1000 65536 8192
```

Attribution probe result:

```text
request size=65536 align=8192
source=5 requested=65536 raw_bytes=131072
p43_lookup_user=1 p43_lookup_raw=1
```

Guard behavior:

- `64K align=4096`: fail-closed
- `65537 align=16`: fail-closed

Current A/B result:

```bash
./linux/run_linux_hz5_p43_ab.sh --arch x86_64 --runs 5
```

Summary:

```text
lane           runs  median_ops_s  median_ru_maxrss_kb
p25            5     6.53783e+07   2048
p43            5     5.42509e+07   2048
p43-nolookup   5     6.11691e+07   2048
p43-source     5     6.49505e+07   2048
p43-trustfast  5     5.46213e+07   2048
p43-trustwrap  5     6.62676e+07   2048
```

Interpretation:

- Linux P43 source itself is not the main local-only bottleneck.
- `p43-trustfast` does not recover, so the atomic mask loads inside the fast
  lookup hit are not the main bottleneck.
- `p43-trustwrap` recovers to P25-or-better, so the current loss is primarily
  from doing P43 ownership lookup before decoding the HZ5 wrapper.
- `p43-source` is P25-level in the short run, so the Linux segment source is
  plausible, but it is not yet a final Linux P43i result.
- `p43-trustwrap` is still a candidate/control lane because stale HZ5 wrapper
  double-free detection is weaker than the full P43 lookup lane.

Latest raw result folder:

```bash
private/raw-results/linux/hz5_p43_ab_20260523_015959
```

### Decoded Raw Lookup Follow-Up

Added three safer decoded-wrapper-first control lanes:

- `p43-rawlookup`
  - decodes the HZ5 wrapper first
  - validates `wrapped->raw` through normal P43 lookup
  - fail-closed when raw lookup is not active
- `p43-rawfast`
  - validates `wrapped->raw` with P43 fast table only
  - still checks slot base equality and active mask state
  - avoids global scan/lock fallback
- `p43-rawalloc`
  - validates `wrapped->raw` with P43 fast table and `allocated_mask`
  - keeps double-free/stale allocated-bit detection
  - intentionally skips committed/cold/pending mask loads for the first Linux
    no-decommit candidate/control

Commands:

```bash
./linux/build_linux_hz5_standalone.sh --linux-p43-decoded-raw-lookup
./linux/build_linux_hz5_standalone.sh --linux-p43-decoded-raw-fastlookup
./linux/build_linux_hz5_standalone.sh --linux-p43-decoded-raw-allocated
./linux/run_linux_hz5_p43_ab.sh --runs 5 --iters 1000000
```

Bench harness note:

- `bench_hz5_standalone_aligned64k` no longer calls `abort()` when
  `hz5_aligned_alloc()` returns `NULL`.
- Unsupported exact-only routes now exit with status `5`, so guard rows can
  record fail-closed behavior without producing a core dump.

Guard smoke after the harness change:

```text
65536:8192 status=0 allocator=hz5-standalone ...
65536:4096 status=5 hz5_aligned_alloc failed iter=0 size=65536 align=4096
65537:16   status=5 hz5_aligned_alloc failed iter=0 size=65537 align=16
```

Latest summary:

```text
lane           runs  median_ops_s  median_ru_maxrss_kb
p25            5     6.58166e+07   2048
p43            5     5.31063e+07   2048
p43-nolookup   5     5.725e+07     2048
p43-rawalloc   5     5.41864e+07   2048
p43-rawfast    5     5.36164e+07   2048
p43-rawlookup  5     4.99126e+07   2048
p43-source     5     6.71181e+07   2048
p43-trustfast  5     5.616e+07     2048
p43-trustwrap  5     6.31005e+07   2048
```

Latest raw result folder:

```bash
private/raw-results/linux/hz5_p43_ab_20260523_025431
```

Interpretation:

- Decoded raw validation does not recover local-only throughput.
- Avoiding scan/lock (`p43-rawfast`) is not enough.
- Reducing active validation to allocated bit only (`p43-rawalloc`) is still not
  enough.
- The only P25-level decoded-wrapper-first result is `p43-trustwrap`, which is
  not acceptable as a final lane because it trusts the wrapper source and skips
  stale/double-free validation.
- Current Linux P43 segment source remains plausible as a source layer
  (`p43-source`), but the P43 ownership/validation model is too expensive for
  the local-only exact microbench.

Next attack:

- Do not promote `p43-trustwrap` as final.
- Keep `p43-rawalloc` only as a control while decommit/runtime release remains
  disabled.
- Measure producer/consumer remote-free and RSS plateau before spending more on
  local-only ownership lookup tuning.
- If local-only must match P25, the next real design should store a compact
  source/slot token in the wrapper at allocation time instead of reconstructing
  ownership from the user/raw pointer on every free.

### Linux P43 Token Candidate

Implemented first Ubuntu-only token controls:

- `--linux-p43-token`
  - macro: `BENCHLAB_HZ5_P43_WRAPPER_TOKEN=1`
  - wrapper header grows only in this candidate build
  - stores `p43_segment_token`, `p43_slot_index`, `p43_token_cookie`
  - free validates token and calls direct P43 prepared slot release
  - keeps allocated-bit double-free/stale guard, but bypasses the P25 bridge
    release topology
- `--linux-p43-token-bridge`
  - macros:
    - `BENCHLAB_HZ5_P43_WRAPPER_TOKEN=1`
    - `BENCHLAB_HZ5_P43_WRAPPER_TOKEN_BRIDGE=1`
  - validates the token cookie, then releases through the existing P25 bridge
  - preserves the faster bridge release topology but does not clear P43
    allocated state on free

Important constraint:

- Windows builds do not define `BENCHLAB_HZ5_P43_WRAPPER_TOKEN`, so the normal
  wrapper layout and P43i/P45 behavior are unchanged.

Guard smoke:

```text
65536:8192 status=0 allocator=hz5-standalone ...
65536:4096 status=5 hz5_aligned_alloc failed iter=0 size=65536 align=4096
65537:16   status=5 hz5_aligned_alloc failed iter=0 size=65537 align=16
```

Latest A/B:

```text
lane             runs  median_ops_s  median_ru_maxrss_kb
p25              5     6.75262e+07   2048
p43              5     5.33562e+07   2048
p43-nolookup     5     5.82627e+07   2048
p43-rawalloc     5     5.40514e+07   2048
p43-rawfast      5     5.4359e+07    2048
p43-rawlookup    5     5.37109e+07   2048
p43-source       5     6.7193e+07    2048
p43-token        5     4.48786e+07   1920
p43-tokenbridge  5     5.39215e+07   2048
p43-trustfast    5     5.43095e+07   2048
p43-trustwrap    5     6.40049e+07   2048
```

Latest raw result folder:

```bash
private/raw-results/linux/hz5_p43_ab_20260523_032018
```

Interpretation:

- `p43-token` is slower because direct P43 descriptor release changes the
  release topology and leaves the P25 bridge fast path.
- `p43-tokenbridge` is still slow because token generation currently performs an
  allocation-side raw lookup after `hz5_lowpage64_acquire()`.
- The design lesson is sharper now: a viable Linux token lane must have the P43
  source return `{raw, segment, slot}` together. Post-hoc token construction by
  looking up `raw` loses the benefit.
- `p43-trustwrap` remains the speed target/control, not a safe final lane.

Next token design:

- add a P43 source acquire API that fills `Hz5Lowpage64FreeCtx` or a compact
  `{segment, slot}` token at the moment the slot is allocated/reused
- store that token in the wrapper without a second lookup
- benchmark both:
  - token + P25 bridge release, for speed parity with `trustwrap`
  - token + direct P43 release, for stricter stale/double-free guard

### Source-Direct Token Follow-Up

Implemented source-direct token acquisition:

- `hz5_lowpage64_p43_alloc_slot_prepared(raw_bytes, ctx)`
  - returns `raw`
  - fills `ctx.segment_token`, `ctx.slot_index`, `ctx.slot_base` at the same
    point the P43 source chooses the slot
- `hz5_lowpage64_acquire_p43_token(raw_bytes, ctx)`
  - skips post-hoc raw lookup for token builds
- token builds now store the wrapper token from the source-provided ctx

Latest A/B after removing allocation-side token lookup:

```text
lane             runs  median_ops_s  median_ru_maxrss_kb
p25              5     6.38231e+07   2048
p43              5     5.31559e+07   2048
p43-nolookup     5     6.07449e+07   2048
p43-rawalloc     5     5.32593e+07   2048
p43-rawfast      5     5.42002e+07   2048
p43-rawlookup    5     5.38802e+07   2048
p43-source       5     6.54146e+07   2048
p43-token        5     4.50776e+07   1920
p43-tokenbridge  5     4.27586e+07   6144
p43-trustfast    5     5.43984e+07   2048
p43-trustwrap    5     6.41794e+07   2048
```

Latest raw result folder:

```bash
private/raw-results/linux/hz5_p43_ab_20260523_032253
```

Updated interpretation:

- Removing post-hoc token lookup did not rescue `p43-token`.
- The local-only regression is now clearly in the direct P43 release/reuse
  topology, not only in token construction.
- `p43-tokenbridge` became worse after source-direct acquisition because it no
  longer benefits from the P25 bridge acquire/reuse path while still releasing
  into that bridge shape.
- The only fast local-only variants remain:
  - `p43-source`: source-only control
  - `p43-trustwrap`: P25 bridge topology with wrapper-source trust
- For Ubuntu, the next viable design is likely not "direct P43 token release" but
  a bridge-compatible token scheme that stores/reuses token metadata in the P25
  bridge nodes, or a separate remote-free/RSS-focused lane where direct P43
  release has a reason to exist.

### Trace Lane Instrumentation

Added an explicit trace-only observation lane:

- macro: `BENCHLAB_HZ5_TRACE_LANE=1`
- build option: `./linux/build_linux_hz5_standalone.sh --trace-lane`
- trace builds force `BENCHLAB_HZ5_SPEED_LANE=0`
- `BENCHLAB_HZ5_TRACE_LANE && BENCHLAB_HZ5_SPEED_LANE` is a compile error
- normal speed builds keep `BENCHLAB_HZ5_TRACE_LANE=0`, so real benchmark lanes
  do not include trace counters or trace output

Trace output is one atexit line on stderr:

```text
[HZ5_TRACE] key=value ...
```

Counters currently include:

- `alloc_p25_bridge`
- `alloc_p43_source_tls`
- `alloc_p43_source_committed`
- `alloc_p43_source_release_buffer`
- `alloc_p43_source_cold`
- `alloc_p43_source_free_slot`
- `alloc_p43_source_new_segment`
- `alloc_p43_token`
- `free_p25_bridge`
- `free_p43_lookup_prepared`
- `free_p43_token_direct`
- `free_p43_token_bridge`
- `free_trustwrap`
- `free_rawlookup`
- `free_fallback_or_invalid`
- `wrapper_decode_ok`
- `wrapper_decode_miss`
- `wrapper_token_valid`
- `wrapper_token_invalid`

Smoke examples:

```bash
./linux/build_linux_hz5_standalone.sh --trace-lane \
  --out-dir hakozuna-hz5/out/linux/test-trace-p25
./hakozuna-hz5/out/linux/test-trace-p25/bench_hz5_standalone_aligned64k \
  1 1000 65536 8192
```

```text
[HZ5_TRACE] alloc_p25_bridge=1000 free_p25_bridge=1000 wrapper_decode_ok=1000
```

```bash
./linux/build_linux_hz5_standalone.sh --linux-p43-trust-wrapper-source \
  --trace-lane --out-dir hakozuna-hz5/out/linux/test-trace-trustwrap
./hakozuna-hz5/out/linux/test-trace-trustwrap/bench_hz5_standalone_aligned64k \
  1 1000 65536 8192
```

```text
[HZ5_TRACE] alloc_p25_bridge=1000 alloc_p43_source_free_slot=7 \
alloc_p43_source_new_segment=1 free_trustwrap=1000 wrapper_decode_ok=1000
```

```bash
./linux/build_linux_hz5_standalone.sh --linux-p43-token --trace-lane \
  --out-dir hakozuna-hz5/out/linux/test-trace-token
./hakozuna-hz5/out/linux/test-trace-token/bench_hz5_standalone_aligned64k \
  1 1000 65536 8192
```

```text
[HZ5_TRACE] alloc_p25_bridge=1000 alloc_p43_source_free_slot=999 \
alloc_p43_source_new_segment=1 alloc_p43_token=1000 \
free_p43_token_direct=1000 wrapper_decode_ok=1000 wrapper_token_valid=1000
```

Earlier next attack, now superseded by the decoded raw lookup results:

- test producer/consumer remote-free before deciding whether local-only
  regression matters
- add RSS plateau harness after speed/control is stable
- keep `madvise` / decommit / runtime release for a later separate lane

## Initial Workloads

Start with:

- exact `64K align=8192` allocation/free
- exact `4K/8K align=8192` smoke
- mixed small/mid/large only after unsupported routes are explicitly defined
- RSS plateau after the standalone exact lane is stable

## Implementation Notes

- HZ5 lowpage64 has Windows-specific pieces; port only the minimum needed for
  the Linux exact lane first.
- P43 segment-slot paths still contain Windows APIs and should remain out of
  the first standalone lane unless explicitly ported.
- Unsupported routes should fail closed in standalone mode instead of falling
  back to HZ3 or CRT.

## Implemented In This Pass

- `linux/build_linux_hz5_standalone.sh`
  - builds `hakozuna-hz5/out/linux/<arch>/libhakozuna_hz5_standalone.so`
  - builds `hakozuna-hz5/out/linux/<arch>/bench_hz5_standalone_aligned64k`
  - builds the generic comparison binary `bench/out/linux/<arch>/bench_aligned64k`
- `linux/run_linux_hz5_standalone_compare.sh`
  - compares HZ5 standalone against glibc, mimalloc, and tcmalloc
  - defaults to exact `4096:8192,8192:8192,65536:8192`
  - accepts `--cases size:align,...` for explicit exact-route sweeps
  - accepts `--size N --align N` for a single-case compatibility path
  - accepts `--allocators` for optional `hz3` / `hz4` LD_PRELOAD comparison
  - records per-run logs under `private/raw-results/linux/`
  - writes `results.tsv` with case, allocator, run, and log path
- `bench/bench_hz5_standalone_aligned64k.c`
  - calls `hz5_aligned_alloc()` / `hz5_free()` directly
- `BENCHLAB_HZ5_STANDALONE_EXACT_ONLY`
  - makes unsupported HZ5 routes return `NULL`
  - prevents fallback to HZ3 or CRT allocation paths in standalone mode

## Verified On WSL2 Only

```bash
./linux/build_linux_hz5_standalone.sh --arch x86_64
./hakozuna-hz5/out/linux/x86_64/bench_hz5_standalone_aligned64k 1 1000 65536 8192
./hakozuna-hz5/out/linux/x86_64/bench_hz5_standalone_aligned64k 1 10000 4096 8192
./hakozuna-hz5/out/linux/x86_64/bench_hz5_standalone_aligned64k 1 10000 8192 8192
./linux/run_linux_hz5_standalone_compare.sh --arch x86_64 --runs 1 \
  --threads 1 --iters 1000 \
  --skip-build --skip-prepare-allocators
```

Native Ubuntu first pass:

```bash
./linux/build_linux_hz5_standalone.sh --arch x86_64
./linux/run_linux_hz5_standalone_compare.sh --arch x86_64 --runs 10 \
  --threads 1 --iters 1000000 \
  --cases 4096:8192,8192:8192,65536:8192
```

Optional HZ3/HZ4 inclusion after building those lanes:

```bash
./linux/build_linux_release_lane.sh
./linux/run_linux_hz5_standalone_compare.sh --arch x86_64 --runs 10 \
  --threads 1 --iters 1000000 \
  --cases 4096:8192,8192:8192,65536:8192 \
  --allocators hz5,system,hz3,hz4,mimalloc,tcmalloc
```

Fail-closed checks:

```bash
./hakozuna-hz5/out/linux/x86_64/bench_hz5_standalone_aligned64k 1 1 65537 8192
./hakozuna-hz5/out/linux/x86_64/bench_hz5_standalone_aligned64k 1 1 1024 16
```

Both reject unsupported routes instead of falling back.
