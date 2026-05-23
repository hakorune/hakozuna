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

## Paper Benchmark Source

The previous paper benchmark tree is available at:

```text
/mnt/workdisk/public_share/hakmem
```

This maps to the user's `Y:\hakmem` path. Use it as the default paper-main
source for fresh runs. The archived copy at
`/mnt/workdisk/public_share/hakozuna_paper/hakmem` has matching MT matrix
runner and benchmark source, but it is not a git checkout.

Current policy:

- do not copy the whole `hakmem` tree into this branch
- run paper-main through `linux/run_paper_allocator_suite.sh --tier paper-main`
  when a general LD_PRELOAD allocator comparison is needed
- if a self-contained paper-compat suite is needed, import only the paper result
  docs, `scripts/run_mt_lane_remote_matrix.sh`,
  `scripts/lib/ssot_preload_guard.sh`,
  `hakozuna/bench/bench_random_mixed_mt_remote.c`, and
  `hakozuna/scripts/run_realworld_4pack.sh`
- keep these paper-main workloads separate from HZ5 standalone exact
  `64K/a8192` appendix benchmarks

## Current Development Focus: Linux Local2P v2

Status: object-node, route-cookie, reuse-state-only, slim-check, and
fast-cookie measured; offset-cookie A/B was rejected; free-first dispatch A/B
was measured and kept as a mixed-speed candidate; remote-batch was measured and
kept as the current remote-free candidate; remote-batch cap A/B measured.

Goal:

- close the remaining Linux local-only gap to HZ4/tcmalloc without touching the
  Windows P43i/P45 lanes
- keep P25 bridge, P43 token/source, and Linux Local2P roles separated
- keep invalid/double-free behavior fail-closed

Current references:

```text
local exact speed:      hz5-local2p-linkflags
public API shape:       hz5-local2p-tlsfast
remote-free candidate:  hz5-local2p-remotebatch
mixed-prelude watch:    hz5-local2p-slot1
```

Design:

- keep exact route only: `size=65536`, `align=8192`
- keep current Local2P wrapper/cookie/state safety for the first A/B
- change the Local2P recycle node from `raw_ptr` to the aligned `user_ptr`
- store freelist `next` in freed user memory, tcmalloc-style
- keep `raw` in the wrapper header for validation and OS release
- skip invariant wrapper-prefix rewrites on cached object-node reuse
- for `owner == current`, replace the locked `atomic_exchange` free-state
  transition with atomic load/store
- keep remote free on locked atomic transition
- keep local and remote recycle policies separate after shared decode/cookie/state
  validation
- use the Local2P cookie as the direct route guard instead of also checking the
  generic wrapper cookie
- on TLS reuse, update only `local2p_state=ACTIVE`; do not rewrite owner,
  generation, or Local2P cookie
- direct exact API was tested and rejected: it was slower than reusefast
- remove redundant source/requested/raw_bytes checks after direct Local2P decode
- keep corrupted-cookie guard but simplify Local2P cookie to
  raw/aligned/process-secret in the fast-cookie candidate
- free-first dispatch is available as a mixed-speed candidate, but it is not the
  local-speed reference
- `hz5-local2p-freefirst-fastcookie` is now the explicit measurement label for
  the same fast-cookie + free-first compound lane
- `hz5-local2p-tlsfast` is the public-API-shape local/mixed reference:
  owner-local TLS reuse restores only `local2p_state=ACTIVE` and returns the
  cached aligned user pointer without re-reading raw/bounds/header init paths
- `hz5-local2p-exactapi` is the local-only exact API candidate: the aligned64k
  benchmark calls `hz5_local2p_alloc_64k_a8192()` and
  `hz5_local2p_free_64k_a8192()` to bypass generic alloc/free route dispatch
- `hz5-local2p-slot1` is the single-slot TLS candidate: exactapi plus
  `TLS_CAP=1` head-only push/pop, avoiding the local `count` and `next`
  maintenance in the owner-local cache
- `hz5-local2p-linkflags` is the local exact speed reference: exactapi plus
  `-fno-semantic-interposition`, `-fno-plt`, `-fno-stack-protector`,
  x86 `-fcf-protection=none`, and `-Wl,-Bsymbolic-functions`
- remote-batch is the current remote-free candidate: batch remote frees in the
  freeing thread before owner-inbox handoff
- cleanup rule: commonize decode/cookie/state validation helpers, but keep
  local TLS recycle and remote handoff policy separate
- cleanup done so far: split Local2P free path into validate/recycle-local/
  recycle-remote helpers without changing lane selectors
- remote-batch cap is selectable; cap16 remains the balanced default, cap32 is
  the remote-only candidate

Measurement policy:

- compare each new Local2P candidate against the previous candidate, `hz5-p25`,
  `hz4`, and `tcmalloc`
- focus first on local and mixed `64K/a8192` ops/s and instruction count
- keep remote/RSS rows in the same run so local-speed wins do not get mistaken
  for remote/RSS wins
- do not merge remote inbox/batch policy into the local-speed reference unless
  local, remote, mixed, and RSS all support the change

Latest confirmed measurement:

```text
private/raw-results/linux/local2p_linkflags_runs30

local median:
  hz5-local2p-linkflags  253.2M ops/s
  tcmalloc               252.6M ops/s
  exactapi               223.1M ops/s
  tlsfast                213.2M ops/s
  hz4                    129.0M ops/s

mixed final median:
  hz5-local2p-linkflags  282.6M ops/s
  tcmalloc               267.6M ops/s
  exactapi               220.9M ops/s
  tlsfast                220.6M ops/s
  hz4                    136.0M ops/s

remote pairs/s median:
  hz5-local2p-remotebatch 14.79M
  hz5-p25                 12.30M
  hz4                     11.80M
  tlsfast                  8.05M
  hz5-local2p-linkflags    7.49M
  tcmalloc                 2.34M
```

Interpretation:

- linkflags is tcmalloc-class for local exact `64K/a8192` and wins this
  mixed-prelude final row
- linkflags is a speed-lane build policy, not a production/default build policy
- remote-free still belongs to `remotebatch`; do not use linkflags as a remote
  result

Route-cookie measurement:

```text
private/raw-results/linux/local2p_routecookie_runs10

local median:
  hz5-local2p-routecookie  173.3M ops/s
  hz5-local2p-faststate    160.9M ops/s
  hz5-local2p-object       150.8M ops/s
  hz4                      130.5M ops/s
  tcmalloc                 250.3M ops/s

mixed final median:
  hz5-local2p-routecookie  175.7M ops/s
  hz5-local2p-faststate    161.6M ops/s
  hz5-local2p-object       154.4M ops/s
  hz4                      136.7M ops/s
  tcmalloc                 269.9M ops/s

remote pairs/s median:
  hz5-local2p-routecookie    8.23M
  hz5-local2p-faststate      8.11M
  hz5-local2p-object         8.35M
  hz5-p25                   12.30M
  hz4                       10.64M
  tcmalloc                   2.33M
```

Interpretation:

- skipping the generic wrapper-cookie check in the direct Local2P route is a
  real speed win
- Local2P cookie remains the route guard, so mutated Local2P cookie safety smoke
  still fails closed
- remaining tcmalloc gap is now likely header shape, API call path, generation /
  cookie recompute, and RSS/source policy

Reuse-state-only measurement:

```text
private/raw-results/linux/local2p_reusefast_runs10

local median:
  hz5-local2p-reusefast    187.0M ops/s
  hz5-local2p-routecookie  176.1M ops/s
  hz4                      133.1M ops/s
  tcmalloc                 256.3M ops/s

mixed final median:
  hz5-local2p-reusefast    193.6M ops/s
  hz5-local2p-routecookie  177.5M ops/s
  hz4                      137.3M ops/s
  tcmalloc                 270.0M ops/s

remote pairs/s median:
  hz5-local2p-reusefast      8.36M
  hz5-local2p-routecookie    8.22M
  hz5-p25                   12.48M
  hz4                       10.93M
  tcmalloc                   2.38M
```

Interpretation:

- avoiding owner/generation/cookie rewrites on owner-local TLS reuse is a real
  speed win
- 10M perf smoke dropped to about 1.80B instructions
- remaining tcmalloc gap is mostly direct header/API shape and RSS/source policy

Direct exact API note:

- `hz5-linux-local2p-direct-exact-api` was tested as an API/route-dispatch A/B.
- It was slower than `reusefast` in RUNS=10, so the public exact API change was
  not kept.
- Measurement folder: `private/raw-results/linux/local2p_directapi_runs10`

Slim-check measurement:

```text
private/raw-results/linux/local2p_slimcheck_runs10

local median:
  hz5-local2p-slimcheck  198.4M ops/s
  hz5-local2p-reusefast  186.9M ops/s
  hz4                    132.3M ops/s
  tcmalloc               257.8M ops/s

mixed final median:
  hz5-local2p-slimcheck  204.4M ops/s
  hz5-local2p-reusefast  191.6M ops/s
  hz4                    137.2M ops/s
  tcmalloc               269.2M ops/s

remote pairs/s median:
  hz5-local2p-slimcheck    7.97M
  hz5-local2p-reusefast    8.29M
  hz5-p25                 12.51M
  hz4                     11.17M
  tcmalloc                 2.39M
```

Interpretation:

- slim-check is a real local/mixed speed win
- it slightly hurts remote, so it should remain a local-speed candidate
- remaining local gap to tcmalloc is now about 60M ops/s rather than 100M+

Fast-cookie measurement:

```text
private/raw-results/linux/local2p_fastcookie_runs10

local median:
  hz5-local2p-fastcookie  206.2M ops/s
  hz5-local2p-slimcheck   197.1M ops/s
  hz5-p25                  66.2M ops/s
  hz4                     129.8M ops/s
  tcmalloc                250.0M ops/s

mixed final median:
  hz5-local2p-fastcookie  215.3M ops/s
  hz5-local2p-slimcheck   204.3M ops/s
  hz5-p25                  57.3M ops/s
  hz4                     137.1M ops/s
  tcmalloc                265.5M ops/s

remote pairs/s median:
  hz5-local2p-fastcookie    8.13M
  hz5-local2p-slimcheck     8.22M
  hz5-p25                  12.52M
  hz4                      12.14M
  tcmalloc                  2.36M

rss median ops/s:
  hz5-local2p-fastcookie   49.4K ops/s, final RSS 1.6MB
  hz5-local2p-slimcheck    49.4K ops/s, final RSS 1.5MB
  hz5-p25                  61.5K ops/s, final RSS 21.0MB
  hz4                     328.6K ops/s, final RSS 75.5MB
  tcmalloc                366.4K ops/s, final RSS 73.2MB
```

Interpretation:

- fast-cookie is a clean local/mixed speed win while preserving mutated-cookie
  fail-closed safety smoke
- remote remains a separate problem; fast-cookie does not improve the
  producer/consumer remote-free lane
- local gap to tcmalloc is now about 44M ops/s on this focused benchmark

Rejected A/B:

- `hz5-linux-local2p-offset-cookie` replaced the per-free cookie mix with a
  fixed route cookie plus `raw == aligned - 8192` validation.
- Safety smoke passed, but RUNS=10 regressed versus fast-cookie:
  `204.6M` vs `206.3M` local ops/s and `208.2M` vs `213.4M` mixed ops/s.
- Code was not kept; measurement folder:
  `private/raw-results/linux/local2p_offsetcookie_runs10`

Free-first measurement:

```text
private/raw-results/linux/local2p_freefirst_runs10

local median:
  hz5-local2p-fastcookie  210.3M ops/s
  hz5-local2p-freefirst   210.0M ops/s
  hz5-p25                  63.5M ops/s
  hz4                     119.8M ops/s
  tcmalloc                249.5M ops/s

mixed final median:
  hz5-local2p-freefirst   217.6M ops/s
  hz5-local2p-fastcookie  210.0M ops/s
  hz5-p25                  58.0M ops/s
  hz4                     136.6M ops/s
  tcmalloc                270.5M ops/s

remote pairs/s median:
  hz5-local2p-freefirst     7.95M
  hz5-local2p-fastcookie    8.17M
  hz5-p25                  12.21M
  hz4                      11.29M
  tcmalloc                  2.35M
```

Interpretation:

- free-first does not replace fast-cookie for pure local throughput
- it is useful as a mixed-prelude candidate because it improved mixed final
  throughput in the same RUNS=10 set
- remote remains separate and should not inherit local/mixed dispatch choices

Remote-batch measurement:

```text
private/raw-results/linux/local2p_remotebatch_runs10

remote pairs/s median:
  hz5-local2p-remotebatch  13.78M
  hz5-p25                  12.08M
  hz4                      11.88M
  hz5-local2p-inbox         9.99M
  hz5-local2p-fastcookie    8.23M
  tcmalloc                  2.39M

local median:
  hz5-local2p-remotebatch  207.7M ops/s
  hz5-local2p-fastcookie   206.8M ops/s
  hz4                      129.5M ops/s
  tcmalloc                 254.5M ops/s

mixed final median:
  hz5-local2p-fastcookie   213.8M ops/s
  hz5-local2p-remotebatch  206.9M ops/s
  hz4                      135.4M ops/s
  tcmalloc                 269.9M ops/s
```

Interpretation:

- remote-batch is the first Local2P remote candidate in this branch that beats
  both P25 and HZ4 on producer/consumer remote-free
- local throughput stayed near fast-cookie, but mixed was lower, so it should
  remain a remote lane rather than replacing the local/mixed reference
- next remote attack should tune batch cap / flush policy, not local header
  dispatch

Cleanup smoke:

```text
private/raw-results/linux/local2p_cleanup_smoke_runs3

local median:
  hz5-local2p-fastcookie   204.3M ops/s
  hz5-local2p-remotebatch  202.1M ops/s
  hz5-p25                   66.7M ops/s
  hz4                      127.9M ops/s
  tcmalloc                 255.8M ops/s

remote pairs/s median:
  hz5-local2p-remotebatch  15.65M
  hz5-p25                  11.88M
  hz4                      12.47M
  hz5-local2p-fastcookie    8.39M
  tcmalloc                  2.44M
```

Interpretation:

- Local2P free-path helper split did not break safety or the main route shapes
- helper split is acceptable as source cleanup; keep future commonization at the
  validation/recycle-boundary level unless measurements justify more

Remote-batch cap sweep:

```text
private/raw-results/linux/local2p_remotebatch_cap_runs10

remote pairs/s median:
  hz5-local2p-remotebatch32  15.28M
  hz5-local2p-remotebatch    15.22M
  hz5-local2p-remotebatch8   14.87M
  hz5-p25                    12.42M
  hz4                        11.06M
  tcmalloc                    2.38M

local median:
  hz5-local2p-remotebatch8   203.1M ops/s
  hz5-local2p-remotebatch    199.1M ops/s
  hz5-local2p-remotebatch32  198.6M ops/s

mixed final median:
  hz5-local2p-remotebatch8   204.9M ops/s
  hz5-local2p-remotebatch    204.7M ops/s
  hz5-local2p-remotebatch32  199.5M ops/s
```

Interpretation:

- cap32 is marginally best for producer/consumer remote-free
- cap8/cap16 are better for local/mixed robustness
- keep cap16 as the default `hz5-local2p-remotebatch`; use cap32 only when
  explicitly measuring a remote-only lane

Freefirst-fastcookie explicit alias measurement:

```text
private/raw-results/linux/local2p_freefirst_fastcookie_runs10

local median:
  hz5-local2p-freefirst-fastcookie  205.1M ops/s
  hz5-local2p-remotebatch           202.3M ops/s
  hz5-local2p-fastcookie            200.4M ops/s
  hz5-local2p-freefirst             199.3M ops/s
  hz4                               130.8M ops/s
  tcmalloc                          254.6M ops/s

mixed final median:
  hz5-local2p-fastcookie            204.6M ops/s
  hz5-local2p-freefirst             202.3M ops/s
  hz5-local2p-freefirst-fastcookie  202.3M ops/s
  hz5-local2p-remotebatch           202.0M ops/s
  hz4                               137.0M ops/s
  tcmalloc                          270.4M ops/s

remote pairs/s median:
  hz5-local2p-remotebatch           15.26M
  hz5-p25                           12.28M
  hz4                               11.47M
  hz5-local2p-fastcookie             8.24M
  hz5-local2p-freefirst-fastcookie   8.03M
  tcmalloc                           2.36M

rss plateau:
  hz5-local2p-freefirst-fastcookie  49.3K ops/s, final RSS 1.6MB
  hz5-local2p-fastcookie            49.9K ops/s, final RSS 1.6MB
  tcmalloc                         374.3K ops/s, final RSS 73.3MB
```

Interpretation:

- `hz5-local2p-freefirst-fastcookie` is useful as an explicit result label
  for the compound build, but it does not replace `fastcookie` as the
  local/mixed reference
- `remotebatch` remains the remote-free reference
- the remaining tcmalloc gap is still local hot-path cost, not P25/P43 route
  confusion
- safety smoke passed: `bench_hz5_standalone_safety`

TLS fast-return measurement:

```text
private/raw-results/linux/local2p_tlsfast_runs10

local median:
  hz5-local2p-tlsfast               216.3M ops/s
  hz5-local2p-remotebatch           203.0M ops/s
  hz5-local2p-freefirst-fastcookie  200.6M ops/s
  hz5-local2p-fastcookie            199.8M ops/s
  hz4                               131.1M ops/s
  tcmalloc                          253.5M ops/s

mixed final median:
  hz5-local2p-tlsfast               218.8M ops/s
  hz5-local2p-fastcookie            205.5M ops/s
  hz5-local2p-freefirst-fastcookie  204.5M ops/s
  hz5-local2p-remotebatch           203.4M ops/s
  hz4                               137.2M ops/s
  tcmalloc                          270.5M ops/s

remote pairs/s median:
  hz5-local2p-remotebatch           15.57M
  hz5-p25                           12.33M
  hz4                               11.12M
  hz5-local2p-tlsfast                8.18M
  hz5-local2p-fastcookie             8.18M
  tcmalloc                           2.37M

perf stat local 10M, one-run:
  fastcookie:  1.70B instructions, 375.7M cycles
  tlsfast:     1.50B instructions, 354.1M cycles
  tcmalloc:    1.31B instructions, 316.6M cycles
```

Interpretation:

- `tlsfast` is the new local/mixed speed reference
- it reduces Local2P local instructions by about 12% versus fastcookie in the
  one-run perf check
- it does not fix remote-free; keep `remotebatch` as the remote reference
- remaining tcmalloc gap is now smaller but still visible in alloc/free
  instruction count and cycle count
- safety smoke passed: `bench_hz5_standalone_safety`

Exact API measurement:

```text
private/raw-results/linux/local2p_exactapi2_runs10

local median:
  hz5-local2p-exactapi              222.7M ops/s
  hz5-local2p-tlsfast               214.7M ops/s
  hz5-local2p-remotebatch           201.2M ops/s
  hz5-local2p-fastcookie            198.6M ops/s
  hz4                               131.3M ops/s
  tcmalloc                          256.1M ops/s

mixed final median:
  hz5-local2p-tlsfast               222.6M ops/s
  hz5-local2p-exactapi              217.8M ops/s
  hz5-local2p-fastcookie            207.5M ops/s
  hz4                               135.8M ops/s
  tcmalloc                          270.6M ops/s

remote pairs/s median:
  hz5-local2p-remotebatch           14.23M
  hz5-p25                           12.69M
  hz4                               11.21M
  hz5-local2p-tlsfast                8.18M
  hz5-local2p-exactapi               7.51M
  tcmalloc                           2.31M

perf stat local 10M, one-run:
  exactapi:  1.15B instructions, 336.8M cycles
  tlsfast:   1.50B instructions, 355.6M cycles
  tcmalloc:  1.31B instructions, 318.8M cycles
```

Interpretation:

- `exactapi` is the new local-only speed reference for exact `64K/a8192`
- `tlsfast` remains the better same-public-API local/mixed reference
- `remotebatch` remains the remote-free reference
- exact API reduces instruction count below tcmalloc, so the remaining local
  gap is likely cycles from call boundaries, stores/fences, or cacheline
  behavior rather than raw instruction count
- safety smoke passed: `bench_hz5_standalone_safety`

Single-slot TLS measurement:

```text
private/raw-results/linux/local2p_slot1_runs10

local median:
  hz5-local2p-exactapi              225.2M ops/s
  hz5-local2p-slot1                 221.6M ops/s
  hz5-local2p-tlsfast               214.9M ops/s
  hz4                               130.6M ops/s
  tcmalloc                          251.8M ops/s

mixed final median:
  hz5-local2p-slot1                 238.8M ops/s
  hz5-local2p-tlsfast               225.8M ops/s
  hz5-local2p-exactapi              215.6M ops/s
  hz4                               137.0M ops/s
  tcmalloc                          268.5M ops/s

remote pairs/s median:
  hz5-local2p-remotebatch           15.78M
  hz5-p25                           12.99M
  hz4                               10.98M
  hz5-local2p-tlsfast                7.99M
  hz5-local2p-slot1                  7.49M
  tcmalloc                           2.36M

perf stat local 10M, one-run:
  slot1:     1.10B instructions, 353.9M cycles
```

Interpretation:

- `slot1` is not the local-only speed reference; `exactapi` remains faster in
  the local median and has lower cycles in the previous perf check
- `slot1` is a promising mixed-prelude candidate because it improved mixed
  final throughput materially in this run
- `slot1` reduced instruction count further but worsened local cycles, so the
  remaining tcmalloc gap should be attacked with link/call-boundary or memory
  ordering/cacheline A/B rather than more instruction-count-only reductions
- safety smoke passed: `bench_hz5_standalone_safety`

Speed linkflags measurement:

```text
private/raw-results/linux/local2p_linkflags_runs10

local median:
  hz5-local2p-linkflags             253.0M ops/s
  tcmalloc                          254.7M ops/s
  hz5-local2p-exactapi              223.2M ops/s
  hz5-local2p-slot1                 218.9M ops/s
  hz4                               131.5M ops/s

mixed final median:
  hz5-local2p-linkflags             280.9M ops/s
  tcmalloc                          268.9M ops/s
  hz5-local2p-slot1                 223.6M ops/s
  hz5-local2p-tlsfast               218.7M ops/s
  hz4                               136.5M ops/s

remote pairs/s median:
  hz5-local2p-remotebatch           14.83M
  hz5-p25                           13.02M
  hz4                               12.25M
  hz5-local2p-tlsfast                8.16M
  hz5-local2p-linkflags              7.45M
  tcmalloc                           2.34M

perf stat local 10M, one-run:
  linkflags: 1.03B instructions, 308.1M cycles
```

Interpretation:

- `linkflags` is the new local-only exact speed reference and reaches tcmalloc
  class on this exact workload
- `linkflags` also wins the mixed-prelude final throughput in this run
- it is still not a remote-free lane; keep `remotebatch` for remote
- speed-link flags are benchmark-lane flags, not a default production policy
  until the safety/build contract is reviewed
- safety smoke passed: `bench_hz5_standalone_safety`

Speed linkflags RUNS=30:

```text
private/raw-results/linux/local2p_linkflags_runs30

local median:
  hz5-local2p-linkflags 253.2M ops/s
  tcmalloc              252.6M ops/s
  exactapi              223.1M ops/s
  tlsfast               213.2M ops/s
  hz4                   129.0M ops/s

mixed final median:
  hz5-local2p-linkflags 282.6M ops/s
  tcmalloc              267.6M ops/s
  exactapi              220.9M ops/s
  tlsfast               220.6M ops/s
  hz4                   136.0M ops/s

remote pairs/s median:
  remotebatch           14.79M
  p25                   12.30M
  hz4                   11.80M
  tlsfast                8.05M
  linkflags              7.49M
  tcmalloc               2.34M
```

Perf repeat, local 10M x5:

```text
private/raw-results/linux/local2p_perf_repeat_20260524_022458

median:
  linkflags  ops/s=261.5M cycles=305.2M instructions=1.03B
  tcmalloc   ops/s=262.0M cycles=318.3M instructions=1.31B
  exactapi   ops/s=240.0M cycles=336.0M instructions=1.15B
```

Guard/safety:

```text
private/raw-results/linux/local2p_linkflags_guard_20260524_022516

2048:8192    status=5  unsupported exact-only row
4096:8192    status=0  existing HZ5 exact a8192 route, not Local2P claim
8192:8192    status=0  existing HZ5 exact a8192 route, not Local2P claim
65536:4096   status=5  unsupported exact-only row
65537:16     status=5  unsupported exact-only row
262144:4096  status=5  unsupported exact-only row
65536:8192   status=0  Local2P positive control
safety        status=0  hz5-standalone-safety ok
```

Interpretation:

- RUNS=30 confirms linkflags is tcmalloc-class for local exact `64K/a8192`
  and above tcmalloc in this mixed-prelude final row
- perf repeat confirms the remaining local result is not instruction-count
  limited; linkflags uses fewer cycles and instructions than tcmalloc in the
  perf median but measured ops/s is effectively tied
- guard rows did not crash; unsupported exact-only rows fail closed

Local2P direct free cleanup:

```text
commit pending

change:
  commonized direct Local2P decode/free dispatch into
  hz5_policy_local2p_try_free_direct()

scope:
  exact Local2P API free
  normal hz5_policy_free() Local2P direct path

rebuild smoke:
  private/raw-results/linux/local2p_cleanup_rebuild_smoke_20260524_023021

local median:
  hz5-local2p-linkflags 252.9M ops/s
  tcmalloc              254.1M ops/s
  exactapi              229.1M ops/s

mixed final median:
  hz5-local2p-linkflags 285.5M ops/s
  tcmalloc              269.1M ops/s

remote pairs/s median:
  remotebatch           13.23M
  p25                   11.94M
  hz4                   11.18M
  linkflags              6.70M

safety:
  linkflags/exactapi/remotebatch standalone safety passed
```

Interpretation:

- helper cleanup did not break build, safety, or the current local/mixed shape
- linkflags remains the exact local speed reference
- remotebatch remains the remote-free reference

## Branch

Use:

```bash
codex/hz5-linux-p43-port
```

Latest Local2P implementation commit:

```bash
see git log; current latest Local2P measurement confirmation is the linkflags
RUNS=30 record
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

Trace observation run:

```bash
OUTDIR=private/raw-results/linux/hz5_trace_observe_20260523_040820
```

Command shape:

```bash
./hakozuna-hz5/out/linux/trace-<lane>/bench_hz5_standalone_aligned64k \
  1 100000 65536 8192
```

Summary:

```text
lane         status  ops_s         trace
p25          0      40459549.525  alloc_p25_bridge=100000 free_p25_bridge=100000 wrapper_decode_ok=100000
p43          0      41376776.190  alloc_p25_bridge=100000 alloc_p43_source_free_slot=7 alloc_p43_source_new_segment=1 free_p43_lookup_prepared=100000 wrapper_decode_ok=100000
trustwrap    0      43237879.369  alloc_p25_bridge=100000 alloc_p43_source_free_slot=7 alloc_p43_source_new_segment=1 free_trustwrap=100000 wrapper_decode_ok=100000
token        0      34408317.407  alloc_p25_bridge=100000 alloc_p43_source_free_slot=99999 alloc_p43_source_new_segment=1 alloc_p43_token=100000 free_p43_token_direct=100000 wrapper_decode_ok=100000 wrapper_token_valid=100000
tokenbridge  0      23318854.157  alloc_p25_bridge=100000 alloc_p43_source_free_slot=99983 alloc_p43_source_new_segment=17 alloc_p43_token=100000 free_p43_token_bridge=100000 wrapper_decode_ok=100000 wrapper_token_valid=100000
```

Observation:

- `p25` is the clean baseline: alloc and free stay in the P25 bridge.
- `p43` and `trustwrap` still allocate through the P25 bridge on every op; P43
  source is only touched during initial bridge filling.
- `token` and `tokenbridge` allocate through the P43 source on almost every op.
- Therefore the local-only loss is not just ownership lookup. The token lanes
  also lose the P25 bridge reuse topology.
- `tokenbridge` is worst because it allocates from P43 source while freeing into
  the P25 bridge shape, causing topology mismatch.

Current conclusion:

- For local-only 64K/a8192, a competitive Linux lane must preserve P25 bridge
  acquire/reuse behavior.
- Direct P43 token release may still matter for remote-free/RSS workloads, but
  it is not the local-only path.
- Next useful experiment is not more direct-token tuning. It is either:
  - bridge-compatible token metadata carried through P25 bridge nodes, or
  - remote-free/RSS observation where direct P43 release has a workload reason.

### Linux P25 Bridge Attr Candidate

Implemented `BENCHLAB_HZ5_LINUX_P25_BRIDGE_ATTR=1`.

Build option:

```bash
./linux/build_linux_hz5_standalone.sh --linux-p25-bridge-attr
```

Design:

- alloc remains `hz5_lowpage64_acquire()`
- free remains `hz5_lowpage64_release()`
- does not enter `hz5_lowpage64_acquire_p43_token()`
- does not use P43 segment token / slot token
- wrapper gets bridge attribution fields only in this candidate build:
  - `bridge_cookie`
  - `bridge_state`
  - `bridge_generation`
- free path validates:
  - wrapper decode
  - source is `HZ5_WRAPPER_SOURCE_P25_HZ4LOWPAGE`
  - bridge cookie
  - `bridge_state` CAS `ACTIVE -> FREED`
- invalid/double-free cases do not call P25 release, P43 release, or fallback

Trace smoke:

```text
[HZ5_TRACE] alloc_p25_bridge=1000 free_p25_bridge_attr=1000 wrapper_decode_ok=1000 bridge_attr_valid=1000
```

Double-free / foreign smoke:

```text
[HZ5_TRACE] alloc_p25_bridge=1 free_p25_bridge_attr=1 free_fallback_or_invalid=2 wrapper_decode_ok=2 wrapper_decode_miss=1 bridge_attr_valid=1 bridge_attr_cas_fail=1
```

Safety smoke note:

- The public `hz5_free()` API now returns the real `Hz5FreeResult`.
- Local double-free smoke on the cleaned build reported `first=1` and
  `second=3`.
- Trace counters are still useful for route attribution:
  `free_fallback_or_invalid`, `bridge_attr_valid`, and
  `bridge_attr_cas_fail`.

Latest A/B:

```text
lane             runs  median_ops_s  median_ru_maxrss_kb
p25              5     6.53799e+07   2048
p25attr          5     6.17908e+07   2048
p43              5     5.31132e+07   2048
p43-nolookup     5     5.95992e+07   2048
p43-rawalloc     5     5.45538e+07   2048
p43-rawfast      5     5.36162e+07   2048
p43-rawlookup    5     5.29635e+07   2048
p43-source       5     6.44827e+07   2048
p43-token        5     4.57138e+07   1920
p43-tokenbridge  5     4.15979e+07   6144
p43-trustfast    5     5.29963e+07   2048
p43-trustwrap    5     6.41839e+07   2048
```

Latest raw result folder:

```bash
private/raw-results/linux/hz5_p43_ab_20260523_042633
```

Interpretation:

- `p25attr` is within about 3.8% of `trustwrap` on this local-only run.
- It keeps P25 bridge topology and adds a real double-free-before-reuse CAS
  guard.
- This is the current best Linux local-only candidate.
- Next checks should be producer/consumer remote-free, RSS plateau, and mixed
  prelude robustness.

P25 attr focus workloads:

Added:

- `bench/bench_hz5_standalone_remote64k.c`
  - producer thread allocates exact `64K/a8192`
  - consumer thread frees through an SPSC queue
- `bench/bench_hz5_standalone_rss_plateau.c`
  - repeated live-set allocation/free plateau
  - records `/proc/self/status` `VmRSS`
- `bench/bench_hz5_standalone_mixed_prelude.c`
  - 64K/a8192 plateau prelude
  - 256K/a8192 unsupported/source-control probe
  - final exact 64K/a8192 throughput measurement
- `linux/run_linux_hz5_p25attr_focus.sh`
  - compares `p25`, `p25attr`, `p25attr-nocas`,
    `p25attr-nocookie`, `p25attr-readonly`, `p43-trustwrap`,
    `p43-token`, `p43-tokenbridge`, and `p43-source`
  - emits `results.tsv`, `summary.unsorted.tsv`, and `summary.tsv`

Focus run:

```bash
./linux/run_linux_hz5_p25attr_focus.sh --skip-build --runs 5 \
  --local-iters 1000000 \
  --remote-iters 200000 \
  --rss-blocks 1024 \
  --rss-rounds 5 \
  --mixed-blocks 1024 \
  --mixed-rounds 3 \
  --mixed-iters 1000000 \
  --probe-attempts 256
```

Raw result folder:

```bash
private/raw-results/linux/hz5_p25attr_focus_20260523_043838
```

Summary:

```text
workload  lane             runs  median_ops_s  median_pairs_s  median_rss_peak_kb  median_rss_final_kb  median_ru_maxrss_kb
local     p25              5     6.55953e+07   NA              NA                  NA                   2048
local     p25attr          5     5.94897e+07   NA              NA                  NA                   2048
local     p43-source       5     6.59753e+07   NA              NA                  NA                   2048
local     p43-token        5     4.53132e+07   NA              NA                  NA                   1920
local     p43-tokenbridge  5     4.12146e+07   NA              NA                  NA                   6144
local     p43-trustwrap    5     6.43589e+07   NA              NA                  NA                   2048
remote    p25              5     2.01962e+07   1.28961e+07     NA                  NA                   2944
remote    p25attr          5     1.68670e+07   9.82878e+06     NA                  NA                   2816
remote    p43-source       5     1.94148e+07   1.25122e+07     NA                  NA                   2688
remote    p43-token        5     8.81305e+06   4.56299e+06     NA                  NA                   6528
remote    p43-tokenbridge  5     1.07016e+07   5.57982e+06     NA                  NA                   8192
remote    p43-trustwrap    5     2.05715e+07   1.31625e+07     NA                  NA                   2688
rss       p25              5     169464        NA              19056               6780                 19072
rss       p25attr          5     170549        NA              19044               6760                 19072
rss       p43-source       5     332253        NA              14948               5716                 14976
rss       p43-token        5     1.73313e+06   NA              13824               13824                13952
rss       p43-tokenbridge  5     1.13263e+06   NA              22016               22016                22144
rss       p43-trustwrap    5     1.35054e+06   NA              17920               17920                18048
mixed     p25              5     5.74585e+07   2.87292e+07     19056               6768                 19064
mixed     p25attr          5     5.08157e+07   2.54078e+07     18912               6652                 18944
mixed     p43-source       5     5.93805e+07   2.96903e+07     14808               5588                 14848
mixed     p43-token        5     4.78834e+07   2.39417e+07     13696               13696                13952
mixed     p43-tokenbridge  5     4.35051e+07   2.17526e+07     21888               21888                22144
mixed     p43-trustwrap    5     5.74353e+07   2.87176e+07     17920               17920                18176
```

All 120 focus sub-runs exited status `0`.

Mixed prelude boundary:

```text
probe_size=262144 probe_align=8192 probe_attempts=256
probe_successes=0 probe_nulls=256
```

This holds for every lane/run in the focus sweep. The 256K probe is therefore a
fail-closed unsupported-route check, not a HZ5 performance row.

Focus interpretation:

- `p25attr` preserves the P25 topology but now shows visible safety overhead:
  about `-9%` vs `p25` and `-8%` vs `trustwrap` on local-only in this run.
- The remote-free path is worse for `p25attr`: producer/consumer pairs/s is
  about `9.83M` vs `12.90M` for `p25` and `13.16M` for `trustwrap`.
- The most likely remote-free cost is the bridge-state CAS and cacheline
  transfer on the consumer thread.
- RSS plateau for `p25attr` is effectively the same as `p25`.
- `p43-source` remains useful as an RSS/source-control candidate: lower peak and
  final RSS than the P25 family in this focus run.
- Direct token lanes should not be local-throughput lanes; they remain
  remote/RSS controls until a workload justifies the extra topology cost.
- Next useful attack is attribution cost isolation:
  - trace `p25attr` remote-free to confirm attr-valid/CAS counts
  - test a diagnostic lane with cookie check but no CAS
  - test a diagnostic lane with CAS but no cookie recompute
  - keep those diagnostic lanes out of paper-speed claims

Diagnostic attr cost isolation:

Added build-only diagnostic options. These are not safe paper-speed lanes:

- `--linux-p25-bridge-attr-no-cas`
  - validates cookie/source/state, then stores `FREED` without CAS
- `--linux-p25-bridge-attr-no-cookie`
  - validates source/state CAS but skips bridge-cookie recompute
- `--linux-p25-bridge-attr-readonly-state`
  - validates cookie/source/state but does not mark state freed

Diagnostic focus run:

```bash
./linux/run_linux_hz5_p25attr_focus.sh --skip-build --runs 5 \
  --local-iters 1000000 \
  --remote-iters 200000 \
  --rss-blocks 1024 \
  --rss-rounds 5 \
  --mixed-blocks 1024 \
  --mixed-rounds 3 \
  --mixed-iters 1000000 \
  --probe-attempts 256
```

Raw result folder:

```bash
private/raw-results/linux/hz5_p25attr_focus_20260523_044332
```

Relevant medians:

```text
workload  lane                 median_ops_s  median_pairs_s  median_rss_peak_kb  median_rss_final_kb
local     p25                  6.60714e+07   NA              NA                  NA
local     p25attr              6.10760e+07   NA              NA                  NA
local     p25attr-nocas        6.17011e+07   NA              NA                  NA
local     p25attr-nocookie     6.26056e+07   NA              NA                  NA
local     p25attr-readonly     6.11869e+07   NA              NA                  NA
local     p43-trustwrap        6.26294e+07   NA              NA                  NA
remote    p25                  1.97224e+07   1.26647e+07     NA                  NA
remote    p25attr              1.66343e+07   9.74082e+06     NA                  NA
remote    p25attr-nocas        1.73193e+07   1.01587e+07     NA                  NA
remote    p25attr-nocookie     1.61868e+07   9.39875e+06     NA                  NA
remote    p25attr-readonly     1.70301e+07   1.00128e+07     NA                  NA
remote    p43-trustwrap        2.12492e+07   1.35192e+07     NA                  NA
rss       p25                  165478        NA              19048               6876
rss       p25attr              166655        NA              18932               6764
rss       p25attr-nocas        170092        NA              18932               6760
rss       p25attr-nocookie     170588        NA              18936               6768
rss       p25attr-readonly     169912        NA              18944               6780
mixed     p25                  5.85253e+07   2.92627e+07     19048               6780
mixed     p25attr              5.36538e+07   2.68269e+07     18936               6772
mixed     p25attr-nocas        5.35988e+07   2.67994e+07     19068               6880
mixed     p25attr-nocookie     5.44043e+07   2.72022e+07     18940               6780
mixed     p25attr-readonly     5.26114e+07   2.63057e+07     19056               6884
mixed     p43-trustwrap        5.78714e+07   2.89357e+07     17792               17792
```

All 180 diagnostic focus sub-runs exited status `0`.
Every mixed-prelude 256K/a8192 probe stayed fail-closed:

```text
probe_successes=0 probe_nulls=256
```

Trace confirmation:

```bash
./linux/build_linux_hz5_standalone.sh --linux-p25-bridge-attr --trace-lane \
  --out-dir hakozuna-hz5/out/linux/trace-p25attr
./hakozuna-hz5/out/linux/trace-p25attr/bench_hz5_standalone_aligned64k \
  1 100000 65536 8192
./hakozuna-hz5/out/linux/trace-p25attr/bench_hz5_standalone_remote64k \
  100000 65536 8192 1024
```

Both local and remote trace runs:

```text
[HZ5_TRACE] alloc_p25_bridge=100000 free_p25_bridge_attr=100000 wrapper_decode_ok=100000 bridge_attr_valid=100000
```

Diagnostic interpretation:

- Local-only overhead is partly bridge-cookie recompute. `nocookie` is the only
  diagnostic variant that moves meaningfully back toward `p25`/`trustwrap`.
- Remote-free does not recover with `nocas`, `nocookie`, or `readonly`; the loss
  is not isolated to one CAS or one cookie recompute.
- The remote-free penalty is likely the extra producer-written attr metadata
  being read by the consumer, plus the larger validation sequence on the free
  side.
- A safe next Linux lane should not add more P43/token work to the local/remote
  hot path. If safety is required, optimize the attribution cookie/check layout
  first; if remote throughput is the goal, keep trustwrap/P25 as the control.

### Cleanup Inventory And Harness Fixes

Worker source inventory found two classes of issues: benchmark validity issues
that should be fixed before interpreting Ubuntu results, and HZ5 API/source
organization issues that should be handled in separate focused commits.

Fixed immediately in the benchmark harness:

- `bench_hz5_standalone_remote64k.c`
  - replaced shared `q->ops++` with per-thread producer/consumer counters
  - producer now observes `status` while waiting for queue space
  - partial thread-create failure now signals the already-created thread before
    joining
- `bench_hz5_standalone_rss_plateau.c`
  - changed RSS plateau touch from first/last byte to one byte per 4K page
- `bench_hz5_standalone_mixed_prelude.c`
  - changed prelude/probe plateau touch to one byte per 4K page
  - kept final throughput phase edge-touch so it remains comparable to local
    throughput
- `linux/build_linux_hz5_standalone.sh`
  - writes `hz5_build_config.env` with commit, dirty state, arch, and lane flags
- `linux/run_linux_hz5_p25attr_focus.sh`
  - `--skip-build` now rejects missing, stale, or dirty-build lane binaries
  - summary now carries `median_probe_successes` and `median_probe_nulls`
  - diagnostic lanes are marked as diagnostic in usage text

Important consequence:

- RSS medians recorded before this cleanup used first/last-byte touching only.
  Treat those RSS rows as directional harness-development data, not final RSS
  evidence for the paper.
- Remote-free medians recorded before this cleanup had a benign-looking but real
  data race in the `ops/s` counter. `pairs/s` was still based on `iters/time`,
  but the harness itself should be rerun after this fix.

Smoke after harness cleanup:

```bash
./linux/run_linux_hz5_p25attr_focus.sh --runs 1 \
  --local-iters 10000 \
  --remote-iters 10000 \
  --rss-blocks 128 \
  --rss-rounds 2 \
  --mixed-blocks 128 \
  --mixed-rounds 2 \
  --mixed-iters 10000 \
  --probe-attempts 32
```

Raw result folder:

```bash
private/raw-results/linux/hz5_p25attr_focus_20260523_045924
```

All 36 cleanup-smoke sub-runs exited status `0`.
The smoke build metadata recorded `dirty=0`.

Remaining source cleanup items:

- Linux wrapper decode is now documented as a trusted primitive, but it is not a
  general foreign-pointer safety boundary. A real boundary would need a separate
  ownership/span proof before `ptr - sizeof(Hz5WrapperHdr)` is attempted.
- Lowpage split target: the P42 OS/cold path is now split into
  `hz5_lowpage64_os.c`, but `hz5_lowpage64.c` still combines P25 bridge, P40
  controls, P43g counters, P44/P45 diagnostics, and release policy. Split the
  diagnostic/reporting half from the acquire/release core before adding more
  Linux lanes.

### Measurement Checkpoint Before Next Development

Environment:

```text
commit=42c5dad4505e6a19aa3c01161b727e6445aee2f3
kernel=Linux 6.8.0-90-generic x86_64
cpu=AMD Ryzen 7 5825U with Radeon Graphics, 8C/16T
gcc=gcc (Ubuntu 11.4.0-1ubuntu1~22.04.3) 11.4.0
glibc=ldd (Ubuntu GLIBC 2.35-0ubuntu3.13) 2.35
```

Local external allocator comparison after cleanup:

```bash
MIMALLOC_SO=private/bench-assets/linux/allocators/x86_64/libmimalloc2.0/usr/lib/x86_64-linux-gnu/libmimalloc.so.2 \
TCMALLOC_SO=private/bench-assets/linux/allocators/x86_64/libtcmalloc-minimal4/usr/lib/x86_64-linux-gnu/libtcmalloc_minimal.so.4 \
./linux/run_linux_hz5_standalone_compare.sh --arch x86_64 --runs 10 \
  --threads 1 --iters 1000000 --cases 65536:8192 --skip-prepare-allocators
```

Raw result folder:

```bash
private/raw-results/linux/hz5_standalone_20260523_052111
```

Summary:

```text
case          alloc     runs  median_ops_s
s65536_a8192  hz5       10    6.64724e+07
s65536_a8192  system    10    4.94777e+07
s65536_a8192  mimalloc  10    1.38424e+06
s65536_a8192  tcmalloc  10    2.60549e+08
```

Interpretation:

- HZ5 is above system and this mimalloc `posix_memalign` path.
- tcmalloc is still the local-only target to beat; its 64K/a8192 reuse/cache
  path is about 3.9x the current HZ5 standalone lane.
- mimalloc is not a meaningful top competitor on this exact Linux local-only
  benchmark unless a separate mimalloc API/path proves otherwise.

Focus runner fix before measurement:

- `linux/build_linux_hz5_standalone.sh` used `((count++))` under
  `set -euo pipefail`.
- Bash returns status `1` when post-increment evaluates from zero, so the focus
  runner exited immediately after the first diagnostic lane selector.
- The mode counters now use `((count += 1))`, which keeps the mutual-exclusion
  checks but does not abort valid single-selector builds.

Post-fix smoke:

```bash
./linux/run_linux_hz5_p25attr_focus.sh --arch x86_64 --runs 1 \
  --local-iters 10000 --remote-iters 1000 --rss-blocks 16 --rss-rounds 1 \
  --mixed-blocks 16 --mixed-rounds 1 --mixed-iters 10000
```

Raw result folder:

```bash
private/raw-results/linux/hz5_p25attr_focus_20260523_053043
```

Full focus measurement:

```bash
./linux/run_linux_hz5_p25attr_focus.sh --arch x86_64 --runs 5
```

Raw result folder:

```bash
private/raw-results/linux/hz5_p25attr_focus_20260523_053057
```

Key medians:

```text
workload  lane              median_ops_s  median_pairs_s  rss_peak_kb  rss_final_kb  ru_maxrss_kb
local     p25               6.60664e+07   NA              NA           NA            2048
local     p25attr           6.18991e+07   NA              NA           NA            2048
local     p25attr-nocookie  6.44440e+07   NA              NA           NA            2048
local     p43-source        6.75595e+07   NA              NA           NA            2048
local     p43-token         4.77805e+07   NA              NA           NA            1920
local     p43-tokenbridge   4.24126e+07   NA              NA           NA            6016
local     p43-trustwrap     6.40538e+07   NA              NA           NA            2048
remote    p25               2.56398e+07   1.28199e+07     NA           NA            2944
remote    p25attr           1.80581e+07   9.02904e+06     NA           NA            2688
remote    p43-source        2.65844e+07   1.32922e+07     NA           NA            2816
remote    p43-token         9.27008e+06   4.63504e+06     NA           NA            5248
remote    p43-tokenbridge   1.02006e+07   5.10030e+06     NA           NA            8320
remote    p43-trustwrap     2.54433e+07   1.27217e+07     NA           NA            2816
rss       p25               61231.6       NA              76348        21096         76416
rss       p25attr           61639.6       NA              76256        20964         76288
rss       p43-source        79311.7       NA              72320        20096         72320
rss       p43-token         358759        NA              71168        71168         71296
rss       p43-tokenbridge   273347        NA              93696        93696         93824
rss       p43-trustwrap     324923        NA              75264        75264         75392
mixed     p25               5.75716e+07   2.87858e+07     76280        21052         76288
mixed     p25attr           5.15415e+07   2.57708e+07     76388        21092         76416
mixed     p43-source        6.00562e+07   3.00281e+07     72192        19968         72192
mixed     p43-token         4.71513e+07   2.35756e+07     71168        71168         71424
mixed     p43-tokenbridge   4.42704e+07   2.21352e+07     93568        93568         93824
mixed     p43-trustwrap     5.68181e+07   2.84090e+07     75136        75136         75392
```

Boundary rows on `p25attr`:

```text
2048:8192    status=5 fail-closed
4096:8192    status=0 supported existing exact route, 1M local ops/s=3.32758e+07
8192:8192    status=0 supported existing exact route, 1M local ops/s=3.47630e+07
65536:4096   status=5 fail-closed
65537:16     status=5 fail-closed
262144:4096  status=5 fail-closed
```

Measurement conclusion before the next development pass:

- The current Linux local-throughput lane is already good enough to beat system
  and mimalloc on this exact benchmark, but not tcmalloc.
- `p25attr` is the current safe local candidate, but the attribution validation
  cost is still visible: about `-6.3%` vs `p25` local and about `-29.6%` vs
  `p25` remote pairs/s in this run.
- `p25attr-nocookie` nearly recovers local speed, so cookie recompute/layout is
  the first optimization target for local-only.
- Remote-free does not recover with the current diagnostic variants; optimize
  the producer-written / consumer-read metadata footprint before adding more
  token or P43 direct-release work.
- `p43-source` remains the strongest control for RSS/mixed and is the candidate
  source layer for a separate RSS lane, but direct token lanes are not
  competitive throughput lanes yet.

### HZ3/HZ4/Tcmalloc Comparison Checkpoint

Reason:

- Before developing specifically against tcmalloc, compare the same
  `64K/a8192` local-only row against HZ3 and HZ4 too.
- This avoids optimizing HZ5 against a target already explained by an existing
  HZ4 Linux path.

Runner cleanup:

- `linux/run_linux_hz5_standalone_compare.sh`
  - default allocator list now includes `hz3` and `hz4`
  - records per-run `status`
  - records `ru_maxrss_kb` through `/usr/bin/time`
  - writes `summary.unsorted.tsv` plus stable header-first `summary.tsv`
  - finds both `libtcmalloc-minimal4` and `libtcmalloc-minimal4t64` package
    layouts
  - supports `--build-hz3-hz4` to rebuild preload libraries before measuring

Build cleanup needed by `--build-hz3-hz4`:

- `hakozuna/src/hz3_inbox.c`
  - marks `owner_live_count` used in feature combinations where it is otherwise
    compiled but not consumed
- `hakozuna/src/hz3_shim.c`
  - limits the Windows-only page-medium-aligned cache variable to `_WIN32`

Measurement command:

```bash
MIMALLOC_SO=private/bench-assets/linux/allocators/x86_64/libmimalloc2.0/usr/lib/x86_64-linux-gnu/libmimalloc.so.2 \
TCMALLOC_SO=private/bench-assets/linux/allocators/x86_64/libtcmalloc-minimal4/usr/lib/x86_64-linux-gnu/libtcmalloc_minimal.so.4 \
./linux/run_linux_hz5_standalone_compare.sh --arch x86_64 --runs 10 \
  --threads 1 --iters 1000000 --cases 65536:8192 \
  --allocators hz5,system,hz3,hz4,mimalloc,tcmalloc \
  --skip-build --skip-prepare-allocators
```

Raw result folder:

```bash
private/raw-results/linux/hz5_standalone_20260523_053752
```

Summary:

```text
case          alloc     runs  median_ops_s  median_ru_maxrss_kb
s65536_a8192  tcmalloc  10    2.54533e+08   7296
s65536_a8192  hz4       10    1.30350e+08   2176
s65536_a8192  hz5       10    6.68287e+07   2048
s65536_a8192  system    10    4.94237e+07   1664
s65536_a8192  hz3       10    3.98182e+07   3200
s65536_a8192  mimalloc  10    1.38729e+06   2176
```

All allocator runs exited `status=0`.

Interpretation:

- HZ4 is the current in-tree Linux reference for this local-only row:
  about `130M ops/s`, roughly `1.95x` slower than tcmalloc but `1.95x` faster
  than HZ5.
- HZ5 should not try to beat tcmalloc from the current P25 attr shape alone.
  The first internal target is to explain and recover the HZ4 gap.
- tcmalloc remains the external target. Its local `posix_memalign(64K, 8192)`
  path is still about `3.8x` faster than current HZ5.
- mimalloc remains pathologically slow on this exact Linux aligned path and is
  not the useful optimization target for this row.

### HZ4/Tcmalloc/HZ5 Observation Pass

Raw result folder:

```bash
private/raw-results/linux/hz5_hz4_tcmalloc_observe_20260523_054137
```

Perf stat, local-only `64K/a8192`, 1M iterations:

```text
allocator  ops/s     cycles      instructions  branches    cache-misses
tcmalloc   249.5M     46.4M      147.5M        32.8M       153K
hz4        131.5M     64.2M      270.2M        54.8M        44K
hz5         68.7M    121.3M      337.6M        87.9M       133K
```

Perf report hot symbols:

```text
hz5:
  hz5_policy_alloc_aligned
  hz5_lowpage64_release
  worker_main
  hz5_wrapper_decode / hz5_wrapper_init / hz5_lowpage64_acquire

hz4:
  hz4_large_malloc
  hz4_malloc
  posix_memalign
  hz4_large_free_header_checked

tcmalloc:
  aligned_alloc
  operator delete[]
  tc_posix_memalign
```

HZ4 source/path observation:

- `posix_memalign(8192, 65536)` over-allocates by
  `(8192 - 1) + sizeof(hz4_aligned_hdr_t)`, so it requests `73759` bytes from
  `hz4_malloc`.
- That routes to `hz4_large_malloc`; after the large header and 64K page round,
  total allocation size is `131072` bytes, a 2-page large object.
- The steady-state local-only path is the HZ4 large TLS span cache for 2-page
  large allocations, not the shared pagebin/lock-shard/global fallback path.
- HZ4 aligned free first decodes a tiny aligned header (`magic`, `raw`, `cookie`,
  `requested`) and then frees the raw large object.

HZ4 stats observation with `HZ4_OS_STATS=1`, 100k iterations:

```text
large_acq=131072
large_rel=0
pagebin_acq_miss=1
b15_acq_call=1
remote_free_seen=0
```

Interpretation:

- HZ4 hits OS once for the first 2-page large allocation and then reuses it
  entirely from local cache.
- This row is not syscall-bound.

Strace count, 10k iterations:

```text
hz5      mmap/munmap/brk total=18
hz4      mmap/munmap/brk total=46
tcmalloc mmap/munmap/brk total=59
```

HZ5 trace observation, 100k iterations:

```text
[HZ5_TRACE] alloc_p25_bridge=100000 free_p25_bridge=100000 wrapper_decode_ok=100000
```

Interpretation:

- HZ5 has no fallback or P43 route contamination in the measured speed lane.
- The HZ5 loss is pure P25 bridge hot-path cost, not an unsupported/fallback
  artifact.
- HZ5 spends more instructions and branches than HZ4 and far more than
  tcmalloc.
- The first HZ5 development target should be a HZ4-like local 2-page span cache
  for `64K/a8192`, or an equivalent fast path before shared/global bridge work.
- Candidate HZ5 costs to isolate next:
  - `hz5_lowpage64_note_raw_range()` global raw min/max atomics
  - wrapper init/decode cookie/layout validation
  - P25 acquire/release stash/global path versus a tiny TLS 2-page span cache
  - `g_hz5_policy_seen_allocation` atomic branch on free

### Linux Local2P Design Decision

Design document:

```bash
hakozuna-hz5/docs/HZ5_LINUX_LOCAL2P_DESIGN.md
```

Decision:

- Proceed with a Linux-only `hz5-linux-local2p` candidate/control lane.
- Target only exact `64K/a8192` initially.
- Store direct raw `131072` byte spans in a thread-local cache before entering
  the P25 shared/global bridge path.
- Start with TLS `cap1`, then test `cap8`.
- Keep Windows P43i/P45 untouched.
- Keep P43 direct token work out of the local-only speed lane.
- Keep invalid and unsupported routes fail-closed.

First implementation order:

1. Add `BENCHLAB_HZ5_LINUX_LOCAL2P` build flag and lane descriptor. DONE.
2. Add wrapper source tag and Local2P metadata behind the build flag. DONE.
3. Add trace-only Local2P counters. DONE.
4. Implement cap1 TLS raw 128K span stack. DONE.
5. Wire only exact `65536:8192` alloc/free to Local2P under the flag. DONE.
6. Add safety smokes for valid, double-free, mutated cookie/source, foreign
   pointer, cross-thread free, and unsupported routes. DONE.
7. Run local-only A/B against HZ5 P25, HZ4, and tcmalloc. PARTIAL.

Initial implementation notes:

- Build flag: `./linux/build_linux_hz5_standalone.sh --linux-local2p`.
- Lane descriptor: `hz5-linux-local2p`.
- Local2P source tag: `HZ5_WRAPPER_SOURCE_LINUX_LOCAL2P`.
- Span source: `posix_memalign(8192, 131072)`.
- Cache: thread-local raw span stack, cap1 by default.
- Free policy: wrapper decode, Local2P cookie check, `ACTIVE -> FREED` CAS,
  same-owner TLS push.
- Invalid Local2P-looking frees fail closed and do not fall through to P25,
  P43, or HZ3 fallback.

Initial smoke results:

- Trace route for `1 x 100000 x 65536:8192`:
  `alloc_local2p_os=1`, `alloc_local2p_tls_hit=99999`,
  `free_local2p_tls=100000`, `wrapper_decode_ok=100000`.
- Safety smoke: `bench_hz5_standalone_safety` passes for valid free,
  double-free, mutated Local2P cookie, mutated source, foreign pointer,
  cross-thread free, and unsupported guard rows.
- Speed smoke, `1 x 1000000 x 65536:8192`:
  Local2P observed around `74-80M ops/s` versus current P25 around
  `57-70M ops/s` on this machine/run set.
- Perf sample:
  Local2P currently reduces cycles versus P25, but still has higher
  instruction count than P25/HZ4 due to wrapper decode, cookie, and active
  state safety checks.
- Quick compare runner smoke:
  `./linux/run_linux_hz5_standalone_compare.sh --hz5-local2p --runs 1
  --iters 100000 --cases 65536:8192 --allocators hz5,hz4,tcmalloc
  --skip-prepare-allocators --outdir
  private/raw-results/linux/local2p_quick_compare`.
  Result: HZ5 Local2P `69.5M`, HZ4 `113.2M`, tcmalloc `267.6M`.

Fast safe A/B implementation:

- Added safe A/B knobs:
  `BENCHLAB_HZ5_LINUX_LOCAL2P_TLS_PACKED`,
  `BENCHLAB_HZ5_LINUX_LOCAL2P_TLS_INITIAL_EXEC`,
  `BENCHLAB_HZ5_LINUX_LOCAL2P_DIRECT_ROUTE`,
  `BENCHLAB_HZ5_LINUX_LOCAL2P_DIRECT_INIT`.
- Build helpers:
  `--linux-local2p-tls-packed`, `--linux-local2p-initial-exec`,
  `--linux-local2p-direct-route`, `--linux-local2p-direct-init`,
  and combined `--linux-local2p-fast`.
- Compare helper:
  `./linux/run_linux_hz5_standalone_compare.sh --hz5-local2p-fast`.
- Single-run A/B, `1 x 1000000 x 65536:8192`:
  base `74.9M`, packed TLS `77.2M`, initial-exec `106.8M`,
  direct-route `86.7M`, direct-init `86.9M`, combined fast `133.6M`.
- Fast trace route for `1 x 100000 x 65536:8192`:
  `alloc_local2p_os=1`, `alloc_local2p_tls_hit=99999`,
  `free_local2p_tls=100000`, `wrapper_decode_ok=100000`.
- Fast perf sample:
  `257.9M instructions`, `51.4M branches`, `62.1M cycles`.
- Fast compare, `runs=3`, `iters=100000`, `65536:8192`:
  HZ5 Local2P fast median `116.6M`, HZ4 median `115.6M`,
  tcmalloc median `206.5M`.
- Guard smoke:
  `2048:8192`, `65536:4096`, `65537:16`, and `262144:4096`
  fail closed; `4096:8192` and `8192:8192` still use the existing exact
  route and do not enter Local2P.

Next implementation target:

- Keep `--linux-local2p-fast` as the safe speed candidate.
- Next isolate remaining tcmalloc gap with optional diagnostics:
  no-local-cookie, no-state/CAS, no-owner-check, and inline decode upper-bound.

### Linux Local2P Focus Measurements

Measurement infrastructure:

- Added generic LD_PRELOAD-compatible benchmarks:
  `bench/bench_remote64k.c`, `bench/bench_rss_plateau.c`,
  `bench/bench_mixed_prelude.c`.
- `linux/build_linux_hz5_standalone.sh` now builds the generic local, remote,
  RSS, and mixed-prelude benchmark set.
- Added `linux/run_linux_hz5_local2p_focus.sh` to compare HZ5 direct lanes
  against `system`, `hz4`, `mimalloc`, and `tcmalloc` with the same workload
  shapes.

RUNS=10 focus command:

```bash
./linux/run_linux_hz5_local2p_focus.sh \
  --runs 10 \
  --local-iters 1000000 \
  --remote-iters 200000 \
  --rss-blocks 1024 \
  --rss-rounds 5 \
  --mixed-blocks 1024 \
  --mixed-rounds 3 \
  --mixed-iters 1000000 \
  --probe-attempts 256 \
  --allocators hz5-local2p-fast,hz5-p25,hz4,tcmalloc,mimalloc,system \
  --skip-prepare-allocators \
  --outdir private/raw-results/linux/local2p_focus_runs10
```

RUNS=10 summary, median:

- Local `64K/a8192`:
  HZ5 Local2P fast `131.98M ops/s`, HZ4 `131.45M`, HZ5 P25 `67.59M`,
  tcmalloc `254.38M`, system `48.69M`, mimalloc `1.39M`.
- Mixed prelude final `64K/a8192`:
  HZ5 Local2P fast `133.40M ops/s`, HZ4 `136.36M`, HZ5 P25 `57.33M`,
  tcmalloc `269.64M`.
- RSS plateau:
  HZ5 Local2P fast `49.6K ops/s`, peak `77.3MB`, final `1.6MB`;
  HZ4 `331.2K ops/s`, peak/final `75.5MB`;
  tcmalloc `375.0K ops/s`, peak/final `73.3MB`.
- Producer/consumer remote-free:
  HZ5 Local2P fast `137K ops/s`, HZ5 P25 `24.5M`,
  HZ4 `22.4M`, tcmalloc `4.69M`.

Interpretation:

- Local2P fast is now HZ4-class for local-only and mixed-prelude exact
  `64K/a8192`.
- Local2P fast was not a remote-free or RSS-throughput lane at this point.
  cap1 + remote OS release was intentionally conservative and very slow under
  producer/consumer.
- HZ5 P25 remains the stronger Linux remote-free/control lane.
- tcmalloc remains the local throughput ceiling for this Ubuntu microbench.
- mimalloc remains anomalously weak on this aligned workload in this setup.

Guard trace:

```bash
private/raw-results/linux/local2p_guard_trace/results.tsv
```

- `65536:8192`: Local2P trace active:
  `alloc_local2p_os=1`, `alloc_local2p_tls_hit=999`,
  `free_local2p_tls=1000`.
- `2048:8192`, `65536:4096`, `65537:16`, `262144:4096`: fail closed.
- `4096:8192`, `8192:8192`: succeed through existing exact route, with no
  Local2P trace counters.

### Paper Benchmark Suite Organization

Problem:

- Existing paper results are general allocator benchmarks using the same binary
  plus LD_PRELOAD.
- HZ5 Local2P results are direct-API exact `64K/a8192` measurements.
- Mixing those in one main table would confuse general allocator claims with a
  narrow exact-overaligned profile claim.

Documentation:

```bash
hakozuna-hz5/docs/HZ5_PAPER_BENCH_SUITE.md
```

Tier split:

- `paper-main`: existing paper/hakmem MT remote matrix and Redis-like app lane.
  HZ5 is excluded until a general LD_PRELOAD-compatible HZ5 lane exists.
- `appendix-hz5`: HZ5 exact-overaligned profile evaluation, including local,
  remote, RSS, mixed prelude, and guard rows.
- `diagnostic-hz5`: trace/perf/knob smoke results used only for development.

Single entry point:

```bash
./linux/run_paper_allocator_suite.sh --tier paper-main
./linux/run_paper_allocator_suite.sh --tier appendix-hz5
./linux/run_paper_allocator_suite.sh --tier diagnostic-hz5
```

Smoke checks:

```bash
./linux/run_paper_allocator_suite.sh \
  --tier diagnostic-hz5 \
  --outdir private/raw-results/linux/paper_suite_diagnostic_smoke

./linux/run_paper_allocator_suite.sh \
  --tier appendix-hz5 \
  --runs 1 \
  --local-iters 10000 \
  --remote-iters 1000 \
  --rss-blocks 32 \
  --rss-rounds 1 \
  --mixed-blocks 32 \
  --mixed-rounds 1 \
  --mixed-iters 10000 \
  --probe-attempts 8 \
  --skip-prepare-allocators \
  --outdir private/raw-results/linux/paper_suite_appendix_smoke
```

Both smoke tiers completed successfully.

### HZ5 LD_PRELOAD Hybrid Bridge

Purpose:

- Provide a diagnostic bridge for same-binary `LD_PRELOAD` experiments.
- Do not treat it as a pure/general HZ5 allocator.

Implementation:

- Added `hakozuna-hz5/preload/hz5_preload_hybrid.c`.
- `linux/build_linux_hz5_standalone.sh` now emits:
  `libhakozuna_hz5_preload_hybrid.so`.
- The shim routes only exact `65536` bytes with `8192` alignment to HZ5 and
  delegates everything else to libc.
- The shim uses a side table to identify HZ5-owned pointers on `free()`,
  avoiding unsafe wrapper decode on arbitrary libc pointers.
- Reentrant libc allocation calls from inside HZ5 are guarded and delegated to
  the real libc symbols.

Smoke:

```bash
env LD_PRELOAD=$PWD/hakozuna-hz5/out/linux/hz5-preload-hybrid-smoke/libhakozuna_hz5_preload_hybrid.so \
  bench/out/linux/x86_64/bench_aligned64k 1 100000 65536 8192

env LD_PRELOAD=$PWD/hakozuna-hz5/out/linux/hz5-preload-hybrid-smoke/libhakozuna_hz5_preload_hybrid.so \
  bench/out/linux/x86_64/bench_aligned64k 1 10000 4096 8192
```

Focus runner support:

- `linux/run_linux_hz5_local2p_focus.sh` now accepts
  `hz5-preload-hybrid` as an allocator.
- Short smoke:
  `private/raw-results/linux/hz5_preload_hybrid_focus_smoke/summary.tsv`.
- Initial result: hybrid works, but side-table/mutex overhead is visible.
  It is useful as a same-binary diagnostic, not as a paper-main candidate.

Paper-main hit-rate probe:

- Added `linux/run_hz5_preload_hit_probe.sh`.
- Added suite tier:
  `./linux/run_paper_allocator_suite.sh --tier diagnostic-hz5-preload`.
- Probe command:

```bash
./linux/run_hz5_preload_hit_probe.sh \
  --iters 100000 \
  --outdir private/raw-results/linux/hz5_preload_hit_probe_paper_shape
```

Result:

- `guard_r0/r50/r90`: `malloc_hz5=0`.
- `main_r0/r50/r90`: `malloc_hz5=0`.
- `cross128_r0/r50/r90`: `malloc_hz5=3` out of about `801589`
  malloc calls.

Conclusion:

- Existing paper-main random_mixed workloads almost never exercise the HZ5
  Local2P exact `64K/a8192` route.
- Adding the hybrid preload library to paper-main would mostly measure libc
  plus shim overhead, not HZ5.
- HZ5 Local2P remains an appendix exact-overaligned profile unless a true
  general HZ5 preload lane is developed.

Go/no-go:

- Minimum: Local2P beats HZ5 P25 by at least 50% on local-only `64K/a8192`.
- Strong: Local2P reaches at least HZ4 minus 10%.
- Stretch: Local2P reduces instruction/branch count toward tcmalloc.

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
  - builds `bench_hz5_standalone_remote64k`
  - builds `bench_hz5_standalone_rss_plateau`
  - builds `bench_hz5_standalone_mixed_prelude`
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
- `bench/bench_hz5_standalone_remote64k.c`
  - measures producer/consumer remote-free handoff for HZ5 standalone lanes
- `bench/bench_hz5_standalone_rss_plateau.c`
  - records RSS plateau behavior with exact HZ5 standalone calls
- `bench/bench_hz5_standalone_mixed_prelude.c`
  - runs 64K/a8192 prelude, unsupported probe, then final 64K/a8192 throughput
- `linux/run_linux_hz5_p25attr_focus.sh`
  - runs local, remote, RSS, and mixed-prelude focus sweeps for Linux HZ5 lanes
- `BENCHLAB_HZ5_LINUX_P25_BRIDGE_ATTR_NO_CAS`
  - diagnostic-only p25attr cost isolation, not a safe speed lane
- `BENCHLAB_HZ5_LINUX_P25_BRIDGE_ATTR_NO_COOKIE`
  - diagnostic-only p25attr cookie recompute isolation
- `BENCHLAB_HZ5_LINUX_P25_BRIDGE_ATTR_READONLY_STATE`
  - diagnostic-only p25attr state-write isolation
- `BENCHLAB_HZ5_STANDALONE_EXACT_ONLY`
  - makes unsupported HZ5 routes return `NULL`
  - prevents fallback to HZ3 or CRT allocation paths in standalone mode
- `hakozuna-hz5/include/hz5.h`
  - now exposes only the public HZ5 ABI; P1/P2 entrypoints moved to
    `hz5_internal.h`
- `hakozuna-hz5/wrapper/hz5_wrapper.h` / `hakozuna-hz5/wrapper/hz5_wrapper.c`
  - wrapper headers now carry explicit layout version/size metadata
- `hakozuna-hz5/contract/hz5_contract.c`
  - emits lane-specific descriptor names for `hz5-linux-*` variants
  - embeds the source commit in the descriptor payload
- `linux/build_linux_hz5_standalone.sh`
  - rejects mixed `p25attr` / `p43` lane combinations
  - rejects incompatible P43 mode submodes
- `hakozuna-hz5/lowpage/hz5_lowpage64_os.c`
  - extracts the OS/cold-path raw alloc and release helpers from the lowpage core

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

## HZ5 Linux Route/Lane Cleanup

Added a route/lane/benchmark-profile SSOT:

```text
hakozuna-hz5/docs/HZ5_LINUX_ROUTE_LANE_MATRIX.md
```

Purpose:

- stop mixing actual allocation routes with build lanes and paper benchmark
  profiles
- keep `local2p`, `p25_bridge`, `p25attr`, `p43_token`, and
  `preload_hybrid` in separate categories
- prevent diagnostic lanes from becoming paper claims by accident

Current classification, using runner labels:

- `hz5-local2p-fast`: current Linux exact `64K/a8192` appendix speed
  candidate
- `hz5-p25`: Linux HZ5 control/baseline route
- `p25attr` variants: safety/cost diagnostics
- `p43-token` / `p43-tokenbridge` / `p43-trustwrap`: diagnostic or
  remote/RSS candidate-watch only
- `hz5-preload-hybrid`: libc+HZ5 diagnostic adapter, not paper-main and not a
  pure HZ5 allocator

Paper wording rule:

```text
HZ5 Local2P is a Linux exact-overaligned local-throughput candidate for
64K/a8192. It is not a general allocator profile.
```

Runner cleanup:

- `linux/run_linux_hz5_local2p_focus.sh` now writes `lane_metadata.tsv`.
- `linux/run_linux_hz5_p25attr_focus.sh` now writes `lane_metadata.tsv`.
- The metadata records runner label, role name, primary route, classification,
  claim scope, and note.
- Use this metadata before moving rows into paper appendix or diagnostic tables.

Local2P remote recycle control:

- Added a bounded global recycle stack inside the existing `local2p` route,
  cap 1024 by default.
- Same-owner free still pushes to the TLS cap1 stack.
- Cross-thread free now validates wrapper/cookie/state, marks `ACTIVE -> FREED`,
  then pushes the raw 128K span to the global recycle stack instead of calling
  OS `free(raw)` on every remote free.
- Allocation checks TLS first, then global recycle, then
  `posix_memalign(8192, 131072)`.
- New trace counters:
  `alloc_local2p_global_hit` and `free_local2p_global`.
- Smoke trace:
  `wrapper_decode_ok=10000 alloc_local2p_global_hit=9896 alloc_local2p_os=104
  free_local2p_global=10000 free_local2p_remote=10000`.
- Smoke remote-free:
  `bench_hz5_standalone_remote64k 200000 65536 8192 1024` reached about
  `7.06M pairs/s`.

Local2P safe hot-path optimization:

- Added diagnostic build switches:
  `--linux-local2p-no-cookie` and `--linux-local2p-no-cas`.
- Cost isolation result on local `64K/a8192`, 10M iterations:
  - fast baseline: about `124M ops/s`, `2.64B` instructions
  - no-cookie diagnostic: about `146M ops/s`, `2.14B` instructions, unsafe
    because mutated-cookie safety intentionally fails
  - no-CAS diagnostic: about `139M ops/s`, safety smoke still passes
- Kept cookie validation enabled, but simplified the Local2P attribution cookie
  mix to reduce hot-path instructions.
- Replaced Local2P free `compare_exchange` with `atomic_exchange`; double-free
  detection remains intact because only the first free observes `ACTIVE`.
- Safety smoke after the safe optimization: `hz5-standalone-safety ok`.
- Perf after safe optimization, local `64K/a8192`, 10M iterations:
  about `140M ops/s`, `2.42B` instructions.
- Repeat5 HZ5-only focus:
  - local: Local2P fast `136.4M`, P25 `67.5M`
  - mixed final: Local2P fast `136.2M`, P25 `57.1M`
  - remote pairs/s: Local2P fast `7.16M`, P25 `12.83M`
  - RSS throughput: Local2P fast `50.2K`, P25 `62.1K`

Conclusion:

- Local2P exact local/mixed is now HZ4-class or slightly above the prior HZ4
  median in this branch's Ubuntu measurements.
- The next performance gap is not local-only HZ4 parity; it is either:
  - tcmalloc's much shorter local hot path, or
  - Local2P remote-free, which needs an owner-inbox/MPSC design instead of a
    single global recycle stack.

## Next Attack Order

Ubuntu HZ5 will be developed as separate workload lanes:

1. `local2p-speed`
   - reduce local `64K/a8192` instruction count toward tcmalloc
   - first target: Local2P direct free decode before generic wrapper decode
2. `local2p-remote`
   - recover producer/consumer remote-free beyond the global recycle control
   - first target: owner inbox / MPSC queue
3. `rss-control`
   - keep low final RSS without polluting the speed lane
   - first target: separate release/decommit policy lane

Do not mix these claims. The next implementation step is `local2p-speed`
direct free decode.

### Local2P Direct Free Decode

Implemented a Local2P-specific free decode before the generic wrapper decode:

```text
hz5_policy_free(ptr)
  -> hz5_policy_local2p_direct_decode(ptr)
  -> hz5_policy_local2p_free(...)
  -> generic wrapper decode only for non-Local2P routes
```

Safety:

- Keeps wrapper magic/layout/cookie/source checks.
- Keeps Local2P cookie validation.
- Keeps `atomic_exchange(ACTIVE -> FREED)` double-free guard.
- `bench_hz5_standalone_safety` passed.

Perf, local `64K/a8192`, 10M iterations:

- before direct decode safe optimization: about `140M ops/s`, `2.42B`
  instructions
- after direct decode: about `150.5M ops/s`, `2.30B` instructions

Repeat5 HZ5-only focus:

- local: Local2P fast `143.5M`, P25 `68.1M`
- mixed final: Local2P fast `148.1M`, P25 `58.3M`
- remote pairs/s: Local2P fast `7.35M`, P25 `12.80M`
- RSS throughput: Local2P fast `49.4K`, P25 `62.5K`

Conclusion:

- `local2p-speed` moved above HZ4-class in the latest HZ5-only repeat.
- The next distinct attack is `local2p-remote`: owner inbox / MPSC remote-free
  queue. Do not try to solve remote-free by adding more global-lock work to the
  speed lane.

## Local2P Remote Owner Inbox Plan

Next candidate:

```text
--linux-local2p-owner-inbox
```

Goal:

- keep the `local2p` route and safety checks
- replace cross-thread global-lock recycle with owner-thread inbox recycle
- measure producer/consumer remote-free without making it the speed default

Candidate path:

- owner alloc checks TLS, owner inbox, global recycle, then OS source
- remote free validates wrapper/cookie/state and pushes raw 128K span to the
  owner inbox with an MPSC stack
- if owner inbox is unavailable, fall back to the bounded global recycle stack

Constraint:

- first candidate assumes the owner thread is still alive while remote frees
  arrive; thread-exit hardening is later work if the performance signal is good

### Local2P Owner Inbox Candidate

Implemented:

- build flag: `--linux-local2p-owner-inbox`
- runner label: `hz5-local2p-inbox`
- owner TLS MPSC inbox for remote free
- owner alloc checks TLS, inbox cache, global recycle, then OS source
- inbox pop uses batch drain via `atomic_exchange`, then non-atomic owner-local
  cache consumption

Smoke:

- `bench_hz5_standalone_safety` passed.
- trace showed:
  `alloc_local2p_inbox_hit=9923 alloc_local2p_os=77
  free_local2p_inbox=10000 free_local2p_remote=10000`.

Repeat5 HZ5-only focus:

- local: Local2P fast `141.8M`, Local2P inbox `147.8M`, P25 `67.8M`
- mixed final: Local2P fast `154.4M`, Local2P inbox `155.3M`, P25 `56.6M`
- remote pairs/s: Local2P fast `7.17M`, Local2P inbox `10.47M`, P25 `12.57M`
- RSS throughput: Local2P fast `50.2K`, Local2P inbox `50.2K`, P25 `78.6K`

Conclusion:

- owner inbox is a real remote-free improvement, but not yet a P25/HZ4 remote
  win.
- next remote work should reduce remote free push cost or add batching on the
  producer/consumer handoff path, not return to the global lock.
