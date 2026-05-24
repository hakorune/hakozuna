# HZ5 Linux Route / Lane Matrix

This document is the Linux HZ5 naming and classification source of truth.
Keep route, lane, and benchmark profile separate.

For the short current-status summary, read:

```text
docs/HZ5_LINUX_DEV_BRIEF.md
```

## Rule

```text
Route:
  the memory path actually used by one allocation/free

Lane:
  a build/profile name selected by compile flags

Benchmark profile:
  the workload family used to judge a lane
```

Do not use a route name as a paper claim. Do not use a benchmark result without
the lane and profile that produced it.

## Reporting Rows

Current paper/appendix-facing HZ5 Linux rows are profile-specific:

| Reporting row | Lane | Use |
| --- | --- | --- |
| Low-final-RSS local/mixed exact speed | `hz5-local2p-linkflags` | local-only and mixed-prelude final `64K/a8192` throughput when low final RSS matters |
| RSS-throughput retained-cache profile | `hz5-local2p-rssretain2048tls` | 2048-block RSS plateau and retained working-set reuse |
| Producer/consumer remote-free profile | `hz5-local2p-remotebatch` | cross-thread free handoff rows |
| Linux HZ5 control | `hz5-p25` | P25 bridge baseline |
| Small ordinary malloc candidate | `hz5-smallfront-s1` | Linux full-preload route for ordinary malloc <=2KiB |
| Broad mid ordinary malloc candidate | `hz5-midfront-rb16` | Linux full-preload route for ordinary malloc 4KiB..64KiB, broad baseline/default candidate |
| Remote-heavy mid ordinary malloc candidate | `hz5-midfront-allgate` | MidFront owner-inbox drain-all plus empty-gated exchange co-lead candidate |
| Large ordinary malloc candidate | `hz5-largefront-l1` | Linux full-preload route for ordinary malloc >64K..1M, cross128 local coverage candidate |
| Remote-heavy large candidate | `hz5-largefront-inbox` | LargeFront owner-inbox candidate for remote-heavy large rows |
| General remote-tail candidate | `hz5-general-region-outbox` | SmallFront outbox + MidFront rb16 + LargeFront region-map candidate |

Older Local2P evolution lanes remain selectable diagnostics. Do not report them
as competing HZ5 profiles unless the result is explicitly an implementation
A/B.

Current comparison priority:

```text
local/mixed speed:
  linkflags is already tcmalloc-class; maintain reproducibility

RSS plateau:
  rssretain2048tls is the accepted retained-RSS profile; do not chase tcmalloc
  further unless one optional B-lite experiment clearly wins

remote-free:
  remotebatch already beats P25/HZ4/tcmalloc in the producer/consumer row;
  tune only with small cap/flush A/B

small 4K/8K a8192:
  guard/control only unless a separate SmallA8192 profile is required
```

## Current Classification

| Name | Type | Status | Paper use |
| --- | --- | --- | --- |
| `local2p` | route | active Linux exact route | appendix only |
| `p2_run_a8192` | route | active legacy exact 4K/8K route | guard/control until optimized |
| `p25_bridge` | route | active control route | control/baseline |
| `p25attr` | lane/route variant | diagnostic/safety evidence | not a claim |
| `p43_token` | lane/route variant | remote/RSS candidate-watch | diagnostic until proven |
| `p43_tokenbridge` | lane/route variant | topology-mismatch evidence | diagnostic only |
| `smallfront_emptygate` | lane variant | diagnostic only | not a claim |
| `p43_trustwrap` | lane/route variant | unsafe speed control | diagnostic only |
| `preload_hybrid` | adapter | libc+HZ5 diagnostic bridge | not paper-main |
| `preload_full` | adapter | HZ5 attribution control | not a performance claim by itself |
| `smallfront_s1` | route/lane family | active Linux small front-end | candidate for <=2KiB ordinary malloc |
| `midfront_m1` | route/lane family | active Linux mid front-end | candidate for 4KiB..64KiB ordinary malloc |
| `midfront_rb16` | lane | broad MidFront default candidate | general MidFront comparison row |
| `midfront_allgate` | lane | remote-heavy MidFront co-lead candidate | candidate for remote-heavy MidFront rows |
| `midfront_takefirst` | lane | direct-return A/B | diagnostic only |
| `midfront_maskhitstop` | lane | bounded mask A/B | diagnostic only |
| `midfront_globalrecycle` | lane | global recycle control | control only |
| `midfront_remote_outbox` | lane | MidFront owner/class sender outbox candidate | cross128 candidate only |
| `midfront_remote_rbuf` | lane | HZ4-style sender TLS ring grouped on flush | diagnostic only |
| `midfront_directfree` | lane | remote ACTIVE->LOCAL_FREE state diagnostic | candidate-watch |
| `preload_free_midfirst` | lane | preload free dispatch order diagnostic | diagnostic only |
| `midfront_lookup_cache` | lane | MidFront TLS page lookup cache diagnostic | diagnostic only |
| `largefront_l1` | route/lane family | active Linux large front-end candidate | candidate for cross128 coverage |
| `largefront_inbox` | lane | active remote-large candidate | candidate for remote-heavy large rows |
| `largefront_rb16` | lane | remote batch A/B | diagnostic only |
| `largefront_takefirst` | lane | direct-return A/B | diagnostic only |
| `largefront_region_map` | lane | LargeFront-L2 source-region lookup candidate | candidate after broader matrix |
| `largefront_lower_classes` | lane | 8K..64K LargeFront route-split diagnostic | diagnostic only |
| `smallfront_remote_outbox` | lane | SmallFront owner/class sender outbox candidate | diagnostic/candidate only |
| `general_region_outbox` | preset lane | combined general allocator candidate | not a default until broad matrix passes |
| `hz3_fallback` | fallback route | disabled in standalone exact-only | not a HZ5 win |
| `libc_passthrough` | adapter route | used by preload hybrid misses | not HZ5 |

## Routes

### `smallfront_s1`

Claimed domain:

```text
ordinary malloc/free <= 2048 bytes
normal malloc alignment, 16 bytes
Linux full preload lane first
```

Path goal:

```text
alloc:
  malloc(size <= 2048)
  -> size class
  -> owner-local TLS free list
  -> partial 4KiB small page
  -> new small page on miss

free:
  ptr -> small page descriptor
  -> descriptor kind and slot-boundary check
  -> active-bit check
  -> owner-local free list or owner remote inbox
```

Classification:

- Active Linux full-preload small-object front-end.
- Not Local2P.
- Not a P43/P25 exact route.
- Not a direct hz3 small-bin port.
- Not dependent on the full-preload pointer table for HZ5-owned small frees.

Design source:

```text
docs/HZ5_SMALLFRONT_S1_DESIGN.md
```

Promotion requirement:

```text
safety smoke clean
malloc_hz5 high, malloc_real only bootstrap/reentrant
track_insert_fail == 0
paper-main guard/main/cross128 no longer wrapped-mmap slow path
```

### `midfront_m1`

Claimed domain:

```text
ordinary malloc/free 2049..65536 bytes
normal malloc alignment, 16 bytes
Linux full preload lane
```

Path:

```text
alloc:
  malloc(2049..65536)
  -> round to 4K/8K/16K/32K/64K class
  -> owner-local span list
  -> owner inbox drain on local miss
  -> batched source allocation on source miss

free:
  ptr -> MidFront page-map descriptor
  -> ptr == span->base check
  -> ACTIVE -> LOCAL_FREE owner-local transition
  -> or ACTIVE -> REMOTE_PENDING remote CAS and owner inbox publish
```

Source capacity:

```text
HZ5_MIDFRONT_SOURCE_BATCH_COUNT=64
HZ5_MIDFRONT_MAP_BITS=21
```

Reporting lanes:

| Lane | Build flags | Role |
| --- | --- | --- |
| `hz5-midfront-rb16` | `--linux-midfront-owner-fast-state --linux-midfront-remote-batch-cap 16` | broad default candidate |
| `hz5-midfront-allgate` | `--linux-midfront-owner-fast-state --linux-midfront-remote-batch-cap 16 --linux-midfront-drain-all-on-miss --linux-midfront-drain-empty-gated` | mid-heavy remote candidate |
| `hz5-midfront-drainmask` | `--linux-midfront-owner-fast-state --linux-midfront-remote-batch-cap 16 --linux-midfront-drain-mask-on-miss` | pending-mask candidate/control |
| `hz5-midfront-globalrecycle` | `--linux-midfront-owner-fast-state --linux-midfront-remote-global-recycle` | global recycle control |
| `hz5-midfront-outbox` | `--linux-midfront-owner-fast-state --linux-midfront-remote-outbox --linux-midfront-remote-batch-cap 16` | cross-size remote candidate; not default |
| `hz5-midfront-outbox-flush` | `--linux-midfront-owner-fast-state --linux-midfront-remote-outbox --linux-midfront-outbox-flush-on-miss` | timely-publish diagnostic; not default |
| `hz5-general-midrbuf` | `--linux-hz5-general-midrbuf` | HZ4-style sender rbuf diagnostic; not default |
| `hz5-midfront-directfree` | `--linux-midfront-owner-fast-state --linux-midfront-remote-direct-free-state --linux-midfront-remote-batch-cap 16` | remote state-machine diagnostic; candidate-watch |
| `hz5-general-midfirst` | `--linux-hz5-general-midfirst` | preload dispatch-cost diagnostic; not default |
| `hz5-general-midcache` | `--linux-hz5-general-midcache` | MidFront lookup-cache diagnostic; not default |
| `hz5-general-routesplit` | `--linux-hz5-general-routesplit` | Mid<=4K, Large>4K route split diagnostic; not default |

Diagnostic-only lanes:

```text
hz5-smallfront-emptygate:
  --linux-smallfront-drain-empty-gated
  skips empty SmallFront owner-inbox exchange; not broad enough for default

hz5-midfront-takefirst:
  --linux-midfront-drain-take-first
  direct-return A/B; safe but not the lead policy in the current matrix

hz5-midfront-maskhitstop:
  --linux-midfront-drain-mask-hit-stop
  bounded mask A/B; buildable but excluded from the default observe runner
```

Current decision:

```text
rb16:
  broad baseline/default MidFront candidate

allgate:
  co-lead candidate for remote-heavy main/mid rows
  not a universal replacement for rb16

globalrecycle:
  control only after source batching fixed the original failure class

midfirst:
  diagnostic only. It tests whether MidFront frees are paying too much for the
  SmallFront miss lookup in preload free(). Do not promote global dispatch
  reordering unless broad mixed rows prove it; a page-kind/front-hint map is the
  cleaner follow-up if the signal is real.

midcache:
  diagnostic only. It caches two MidFront page-map lookups per thread. The
  first smoke gave only a small mid_only signal and regressed main/cross128, so
  it is not a broad profile.

midrbuf:
  diagnostic only. It groups sender-side remote frees by owner/class on flush.
  It gave selected cross-size signals but repeat-5 regressed main_r90 and
  mid_only_r90, so it is not a combined default.

routesplit:
  diagnostic only. It moves 8K..64K into lower LargeFront classes and caps
  MidFront at 4096. First smoke regressed main/mid_only heavily.
```

### `largefront_l1`

Claimed domain:

```text
ordinary malloc/free 65537..1048576 bytes
normal malloc alignment, 16 bytes
Linux full preload lane
```

Path:

```text
alloc:
  malloc(>64K..1M)
  -> round to 128K/256K/512K/1M class
  -> owner-local retained span
  -> global retained span
  -> batched mmap source block on miss

free:
  ptr -> LargeFront page-map descriptor
  -> ptr == span->base check
  -> ACTIVE -> LOCAL_FREE
  -> owner-local retained list or global recycle
```

Design source:

```text
docs/HZ5_LARGEFRONT_L1_DESIGN.md
```

Reporting lane:

| Lane | Build flags | Role |
| --- | --- | --- |
| `hz5-largefront-l1` | `--linux-largefront-l1 --linux-midfront-owner-fast-state --linux-midfront-remote-batch-cap 16` | first cross128 coverage candidate |
| `hz5-largefront-inbox` | `--linux-largefront-owner-inbox --linux-largefront-owner-fast-state --linux-midfront-owner-fast-state --linux-midfront-remote-batch-cap 16` | useful remote-large candidate |
| `hz5-largefront-rb16` | `--linux-largefront-remote-batch --linux-largefront-remote-batch-cap 16` | diagnostic only |
| `hz5-largefront-takefirst` | `--linux-largefront-drain-take-first` | diagnostic only |
| `hz5-largefront-emptygate` | `--linux-largefront-drain-empty-gated --linux-largefront-owner-fast-state --linux-midfront-owner-fast-state --linux-midfront-remote-batch-cap 16` | large/main diagnostic only |
| `hz5-largefront-map-base-only` | `--linux-largefront-map-base-only --linux-largefront-owner-fast-state --linux-midfront-owner-fast-state --linux-midfront-remote-batch-cap 16` | timeout/root-cause diagnostic only; weakens interior-pointer invalid-free attribution |
| `hz5-largefront-region-map` | `--linux-largefront-region-map --linux-largefront-owner-fast-state --linux-midfront-owner-fast-state --linux-midfront-remote-batch-cap 16` | LargeFront-L2 candidate; source-region lookup without losing interior invalid-free attribution |
| `hz5-smallfront-remote-outbox` | `--linux-smallfront-remote-outbox --linux-smallfront-remote-batch-cap 8 --linux-largefront-region-map --linux-largefront-owner-fast-state --linux-midfront-owner-fast-state --linux-midfront-remote-batch-cap 16` | SmallFront remote-handoff candidate; associative owner/class sender outbox |
| `hz5-general-region-outbox` | `--linux-hz5-general-region-outbox` | short preset for SmallFront outbox cap8 + MidFront rb16/owner-fast + LargeFront region-map |
| `hz5-midfront-directfree` | `--linux-hz5-general-directfree` | MidFront remote-free state-machine candidate/watch; main/mid remote-heavy only, not broad default |
| `hz5-midfront-trustdrain` | `--linux-hz5-general-region-outbox --linux-midfront-remote-trust-drain-state` | diagnostic upper-bound only; skips owner-drain state load/check after directfree |

Current decision:

```text
L1:
  retained descriptor spans only
  no RSS-return claim
  global recycle for remote frees

inbox:
  useful remote-large candidate

rb16/takefirst:
  diagnostic only after focused A/B

emptygate:
  helps some large/main rows
  not cross-size default

base-only:
  confirms per-page LargeFront map insertion can cause r90 timeout tails
  diagnostic only because interior pages no longer map to the descriptor
  production replacement should be LargeFront-L2 range/region lookup

region-map:
  first LargeFront-L2 candidate
  registers source regions instead of every covered 4K page
  keeps interior-pointer invalid-free attribution
  needs broad matrix before becoming the lead LargeFront row

smallfront-remote-outbox:
  copies HZ4 lcache's multi-outbox idea without changing HZ5 slot-state checks
  modest main/cross improvement in short r90 checks
  candidate only

midfront-remote-outbox:
  applies the same owner/class sender-outbox idea to MidFront spans
  useful as a cross128 candidate
  not a default because main/mid_only r90 can regress when remote frees are
  retained in sender outbox slots too long
  current default outbox slot count is 4

midfront-outbox-flush-on-miss:
  publishes matching-class sender outbox slots on local allocation miss
  diagnostic only; it did not become a broad win in short r90 checks

midfront-remote-direct-free-state:
  remote free transitions ACTIVE -> LOCAL_FREE before publishing to owner inbox
  owner drain accepts LOCAL_FREE spans and skips REMOTE_PENDING -> LOCAL_FREE CAS
  diagnostic/candidate-watch; keep separate from default until repeat/perf
  evidence is stronger

midfront-remote-trust-drain-state:
  requires direct-free-state
  owner drain trusts inbox provenance and skips the LOCAL_FREE state load/check
  no-go for promotion in the first focused general repeat; diagnostic only

deferred:
  hz3/hz4-style 2MiB page-run split/merge pool
  epoch release of empty large segments
  owner-inbox remote large transfer
  lower-overhead radix/range-map variant if source-region lookup is not enough
```

Observation hygiene:

```text
linux/run_linux_hz5_midfront_observe.sh:
  raw.tsv runs with HZ5_PRELOAD_STATS unset
  attrib.tsv is separate and may set HZ5_PRELOAD_STATS=1
```

### `preload_full`

The full preload adapter routes ordinary allocation APIs into HZ5:

```text
malloc/calloc/realloc/posix_memalign/aligned_alloc -> HZ5 API
free/malloc_usable_size -> HZ5 ownership or bootstrap-real handling
```

Classification:

- Attribution/control adapter.
- Useful to prove benchmark traffic enters HZ5.
- Not a performance allocator until `smallfront_s1` or another general
  front-end handles ordinary malloc traffic.

Current smoke:

```text
private/raw-results/linux/hz5_preload_full_smoke_20260524_053601:
  malloc_hz5=5054
  malloc_real=0
  ops/s about 220K on short paper-main shape
```

Interpretation: attribution is fixed, but ordinary small/mid malloc still needs
SmallFront.

### Exact `a8192` Dispatch Map

The Linux standalone exact gate accepts several `align=8192` sizes, but they
do not share one implementation path:

| Request | Current route | Current role |
| --- | --- | --- |
| `4096:8192` | `p2_run_a8192` | legacy small exact route; weak guard/control row |
| `8192:8192` | `p2_run_a8192` | legacy small exact route; weak guard/control row |
| `65536:8192` with Local2P lanes | `local2p` | optimized Linux appendix profile family |
| `65536:8192` with P25 control | `p25_bridge` | Linux HZ5 control route |
| unsupported rows such as `2048:8192`, `65536:4096`, `65537:16`, `262144:4096` | fail closed in standalone exact-only | guard rows |

Do not describe these as one HZ5 exact-a8192 lane. `4K/8K a8192` currently use
the older P2 run/tcache path, while `64K/a8192` is the Local2P profile family.
If 4K/8K are optimized, add a separate small-a8192 profile instead of folding
it into Local2P.

### `local2p`

Claimed domain:

```text
size == 65536
align == 8192
Linux only
```

Path:

```text
alloc:
  hz5_aligned_alloc(65536, 8192)
  -> Local2P direct route
  -> TLS 128K span pop
  -> posix_memalign(8192, 131072) only on miss
  -> wrapper source HZ5_WRAPPER_SOURCE_LINUX_LOCAL2P

free:
  wrapper decode
  -> Local2P source check
  -> cookie/state validation
  -> same-owner TLS span push
  -> remote free pushes raw span to a bounded global recycle stack
```

What it is:

- Linux exact-overaligned local-throughput route.
- HZ4-like TLS 2-page span-cache experiment.

What it is not:

- Not a general allocator.
- Not one single profile for all workloads.
- Not a Windows P43i/P45 replacement.

Remote-free is no longer forced through OS `free(raw)` on every operation. The
remote-free reporting lane is `hz5-local2p-remotebatch`. RSS-throughput
reporting uses `hz5-local2p-rssretain2048tls`. The low-final-RSS local/mixed
speed lane remains `hz5-local2p-linkflags`.

### `p25_bridge`

Claimed domain:

```text
existing exact HZ5 lowpage64 route
not Local2P
```

Path:

```text
alloc:
  hz5_policy_p25_wrapper_alloc
  -> hz5_lowpage64_acquire(raw_bytes)
  -> wrapper source HZ5_WRAPPER_SOURCE_P25_HZ4LOWPAGE

free:
  wrapper decode
  -> hz5_lowpage64_release(raw)
```

Use this as the Linux HZ5 control route. It is the baseline that Local2P must
beat for local-only exact `64K/a8192`.

### `p2_run_a8192`

Claimed domain today:

```text
size == 4096 or size == 8192
align == 8192
Linux standalone exact route
```

Path:

```text
alloc:
  hz5_policy_alloc_aligned
  -> hz5_route_exact_a8192
  -> hz5_p2_alloc_aligned
  -> P2 run/tcache/segment path

free:
  hz5_policy_free
  -> policy ownership check
  -> hz5_p2_free
```

Classification:

- Guard/control route until optimized.
- Not Local2P.
- Not P25 lowpage64.
- Likely overhead sources are P2 front-door dispatch, ownership lookup,
  tcache/reference atomics, and diagnostic stats registration.

Next safe A/B:

```text
linux-p11-speed-core:
  compile existing P2 4K/8K route with HZ5_P11_SPEED_CORE=1
  keep 64K/a8192 on Local2P when Local2P is enabled
  measure before creating a new SmallA8192 route
```

Build selector:

```text
--linux-p11-speed-core
```

Ubuntu result:

```text
p11_speed_core_small_a8192_runs5_20260524_035412:
  4096:8192  hz5 43.6M vs hz4 112.4M vs tcmalloc 258.6M
  8192:8192  hz5 43.8M vs hz4  99.9M vs tcmalloc 257.9M

p11_speed_core_local2p_small_a8192_runs3_20260524_035441:
  4096:8192  hz5 47.9M vs hz4 115.9M vs tcmalloc 256.4M
  8192:8192  hz5 45.7M vs hz4 100.3M vs tcmalloc 257.6M
```

Conclusion: P11 speed-core is not sufficient on Ubuntu. Keep it as a diagnostic
knob. If the small exact rows need to compete, create a separate SmallA8192
route/profile.

### `p25attr`

Path shape:

```text
alloc/free stay in P25 bridge topology
wrapper carries bridge attribution metadata
free validates attribution before P25 release
```

Classification:

- Safety/cost diagnostic.
- Not the current speed candidate.
- Useful for understanding how much cookie/CAS/state validation costs.

### `p43_token`

Path shape:

```text
alloc:
  P43 source/token acquisition

free:
  token validation
  -> direct P43 prepared slot release
```

Classification:

- Do not use for local-only throughput claims.
- Keep as remote-free/RSS/source-control candidate-watch.
- Needs workload evidence before promotion.

### `p43_tokenbridge`

Path shape:

```text
alloc:
  P43 source/token acquisition

free:
  token validation
  -> P25 bridge release shape
```

Classification:

- Topology-mismatch evidence.
- Diagnostic only.

### `p43_trustwrap`

Path shape:

```text
alloc/free stay close to P25 bridge speed shape
free trusts decoded wrapper source and skips stronger ownership lookup
```

Classification:

- Unsafe speed target/control.
- Never promote as a final lane.

### `hz3_fallback`

Standalone exact-only builds set unsupported routes to `NULL`.

Fallback is not allowed to count as an HZ5 exact-route win. If a benchmark row
uses fallback, label it as fallback or remove it from HZ5 claims.

### `libc_passthrough`

The preload hybrid delegates non-claimed allocations to libc.

This is a useful hit-rate diagnostic, but a miss is not an HZ5 allocation.

## Lanes

| Runner label | Role name | Build selector | Primary route | Classification |
| --- | --- | --- | --- | --- |
| `hz5-local2p-fast` | `hz5-linux-local2p-fast` | `--linux-local2p-fast` | `local2p` | older appendix speed candidate |
| `hz5-local2p-object` | `hz5-linux-local2p-object-node` | `--linux-local2p-object-node` | `local2p` | speed candidate: user-pointer freelist node |
| `hz5-local2p-faststate` | `hz5-linux-local2p-same-owner-fast-state` | `--linux-local2p-same-owner-fast-state` | `local2p` | speed candidate: owner-local load/store state |
| `hz5-local2p-routecookie` | `hz5-linux-local2p-route-cookie` | `--linux-local2p-route-cookie` | `local2p` | speed candidate: Local2P cookie as route guard |
| `hz5-local2p-reusefast` | `hz5-linux-local2p-reuse-state-only` | `--linux-local2p-reuse-state-only` | `local2p` | speed candidate: TLS reuse updates state only |
| `hz5-local2p-slimcheck` | `hz5-linux-local2p-slim-check` | `--linux-local2p-slim-check` | `local2p` | speed candidate: reduced direct free checks |
| `hz5-local2p-fastcookie` | `hz5-linux-local2p-fast-cookie` | `--linux-local2p-fast-cookie` | `local2p` | speed candidate: lightweight cookie |
| `hz5-local2p-tlsfast` | `hz5-linux-local2p-tls-fast-return` | `--linux-local2p-tls-fast-return` | `local2p` | public-API-shape control |
| `hz5-local2p-exactapi` | `hz5-linux-local2p-exact-api` | `--linux-local2p-exact-api` | `local2p` | local-only exact API control |
| `hz5-local2p-slot1` | `hz5-linux-local2p-single-slot-tls` | `--linux-local2p-single-slot-tls` | `local2p` | mixed-prelude candidate: TLS_CAP=1 head-only cache |
| `hz5-local2p-linkflags` | `hz5-linux-local2p-speed-linkflags` | `--linux-local2p-speed-linkflags` | `local2p` | low-final-RSS local/mixed speed profile |
| `hz5-local2p-rssretain256` | `hz5-linux-local2p-rss-retain-cap256` | `--linux-local2p-rss-retain --linux-local2p-global-cap 256` | `local2p` | RSS retained-cache cap sweep |
| `hz5-local2p-rssretain512` | `hz5-linux-local2p-rss-retain-cap512` | `--linux-local2p-rss-retain --linux-local2p-global-cap 512` | `local2p` | RSS retained-cache cap sweep |
| `hz5-local2p-rssretain` | `hz5-linux-local2p-rss-retain` | `--linux-local2p-rss-retain` | `local2p` | RSS candidate: local overflow to bounded global cache |
| `hz5-local2p-rssretain1536` | `hz5-linux-local2p-rss-retain-cap1536` | `--linux-local2p-rss-retain --linux-local2p-global-cap 1536` | `local2p` | RSS retained-cache cap sweep |
| `hz5-local2p-rssretain2048` | `hz5-linux-local2p-rss-retain-cap2048` | `--linux-local2p-rss-retain --linux-local2p-global-cap 2048` | `local2p` | RSS control: retain full 2048-block plateau through global cache |
| `hz5-local2p-rssretain2048tls` | `hz5-linux-local2p-rss-retain-cap2048-tls2048` | `--linux-local2p-rss-retain --linux-local2p-global-cap 2048 --linux-local2p-tls-cap 2048` | `local2p` | current RSS throughput profile: retain full plateau in owner-local TLS |
| `hz5-local2p-rssretain2048array` | `hz5-linux-local2p-rss-retain-cap2048-tls2048-array` | `--linux-local2p-rss-retain --linux-local2p-global-cap 2048 --linux-local2p-tls-cap 2048 --linux-local2p-retain-array` | `local2p` | B-lite diagnostic: retain owner-local TLS entries in pointer array |
| `hz5-local2p-freefirst` | `hz5-linux-local2p-free-first` | `--linux-local2p-free-first` | `local2p` | speed candidate: Local2P free-first dispatch |
| `hz5-local2p-freefirst-fastcookie` | `hz5-linux-local2p-freefirst-fastcookie` | `--linux-local2p-freefirst-fastcookie` | `local2p` | explicit alias for the fast-cookie + free-first compound lane |
| `hz5-local2p-inbox` | `hz5-linux-local2p-remote-inbox` | `--linux-local2p-fast --linux-local2p-owner-inbox` | `local2p` | remote-free candidate |
| `hz5-local2p-remotebatch` | `hz5-linux-local2p-remote-batch` | `--linux-local2p-remote-batch` | `local2p` | remote-free candidate: batched owner inbox |
| `hz5-local2p-remotebatch8` | `hz5-linux-local2p-remote-batch-cap8` | `--linux-local2p-remote-batch --linux-local2p-remote-batch-cap 8` | `local2p` | remote-free A/B: batch cap 8 |
| `hz5-local2p-remotebatch32` | `hz5-linux-local2p-remote-batch-cap32` | `--linux-local2p-remote-batch --linux-local2p-remote-batch-cap 32` | `local2p` | remote-free A/B: batch cap 32 |
| `hz5-local2p` | `hz5-linux-local2p` | `--linux-local2p` | `local2p` | baseline Local2P implementation |
| `hz5-p25` | `hz5-linux-p25-control` | no Local2P/P43/P25Attr selector | `p25_bridge` | Linux control |
| `p25attr` | `hz5-linux-p25attr` | `--linux-p25-bridge-attr` | `p25attr` | safety diagnostic |
| `p25attr-nocas` | `hz5-linux-p25attr-nocas` | `--linux-p25-bridge-attr-no-cas` | `p25attr` | cost diagnostic |
| `p25attr-nocookie` | `hz5-linux-p25attr-nocookie` | `--linux-p25-bridge-attr-no-cookie` | `p25attr` | cost diagnostic |
| `p43-token` | `hz5-linux-p43-token` | `--linux-p43-token` | `p43_token` | remote/RSS candidate-watch |
| `p43-tokenbridge` | `hz5-linux-p43-tokenbridge` | `--linux-p43-token-bridge` | `p43_tokenbridge` | diagnostic only |
| `p43-trustwrap` | `hz5-linux-p43-trustwrap` | `--linux-p43-trust-wrapper-source` | `p43_trustwrap` | unsafe control |
| `hz5-preload-hybrid` | `hz5-preload-hybrid-diagnostic` | hybrid preload library | `local2p` hits + `libc_passthrough` misses | diagnostic adapter |
| `hz5-preload-full` | `hz5-linux-preload-full-control` | `--linux-preload-full` | HZ5 API + bootstrap real-libc | attribution/control adapter |
| `hz5-smallfront-s1` | `hz5-linux-smallfront-s1` | `--linux-smallfront-s1` | `smallfront_s1` | active general allocator candidate |
| `hz5-smallfront-remote-outbox` | `hz5-linux-smallfront-remote-outbox` | `--linux-smallfront-remote-outbox --linux-smallfront-remote-batch-cap 8 --linux-largefront-region-map --linux-largefront-owner-fast-state --linux-midfront-owner-fast-state --linux-midfront-remote-batch-cap 16` | `smallfront_s1 + midfront_m1 + largefront_l1` | SmallFront remote-handoff candidate |
| `hz5-midfront-rb16` | `hz5-linux-midfront-rb16` | `--linux-midfront-owner-fast-state --linux-midfront-remote-batch-cap 16` | `midfront_m1` | broad MidFront default candidate |
| `hz5-midfront-allgate` | `hz5-linux-midfront-allgate` | `--linux-midfront-owner-fast-state --linux-midfront-remote-batch-cap 16 --linux-midfront-drain-all-on-miss --linux-midfront-drain-empty-gated` | `midfront_m1` | remote-heavy MidFront co-lead |
| `hz5-midfront-drainmask` | `hz5-linux-midfront-drainmask` | `--linux-midfront-owner-fast-state --linux-midfront-remote-batch-cap 16 --linux-midfront-drain-mask-on-miss` | `midfront_m1` | pending-mask diagnostic/control |
| `hz5-midfront-outbox` | `hz5-linux-midfront-outbox` | `--linux-midfront-owner-fast-state --linux-midfront-remote-outbox --linux-midfront-remote-batch-cap 16` | `midfront_m1` | cross-size remote candidate; not default |
| `hz5-midfront-outbox-flush` | `hz5-linux-midfront-outbox-flush` | `--linux-midfront-owner-fast-state --linux-midfront-remote-outbox --linux-midfront-outbox-flush-on-miss` | `midfront_m1` | timely-publish diagnostic; not default |
| `hz5-largefront-l1` | `hz5-linux-largefront-l1` | `--linux-largefront-l1 --linux-midfront-owner-fast-state --linux-midfront-remote-batch-cap 16` | `largefront_l1` | first large ordinary malloc coverage candidate |
| `hz5-largefront-inbox` | `hz5-linux-largefront-inbox` | `--linux-largefront-owner-inbox --linux-largefront-owner-fast-state --linux-midfront-owner-fast-state --linux-midfront-remote-batch-cap 16` | `largefront_l1` | remote-heavy large candidate |
| `hz5-largefront-region-map` | `hz5-linux-largefront-region-map` | `--linux-largefront-region-map --linux-largefront-owner-fast-state --linux-midfront-owner-fast-state --linux-midfront-remote-batch-cap 16` | `largefront_l1` | LargeFront-L2 source-region lookup candidate |
| `hz5-general-region-outbox` | `hz5-linux-general-region-outbox` | `--linux-hz5-general-region-outbox` | `smallfront_s1 + midfront_m1 + largefront_l1` | current combined general remote-tail candidate preset |
| `hz5-general-midoutbox` | `hz5-linux-general-midoutbox` | `--linux-hz5-general-midoutbox` | `smallfront_s1 + midfront_m1 + largefront_l1` | cross128 candidate with MidFront sender outbox; not default |
| `hz5-general-midoutbox-flush` | `hz5-linux-general-midoutbox-flush` | `--linux-hz5-general-midoutbox-flush` | `smallfront_s1 + midfront_m1 + largefront_l1` | timely-publish diagnostic; not default |

Build selectors for `local2p`, `p25attr`, and `p43` are mutually exclusive.
Keep that rule. It prevents mixed-route benchmark rows.

Local2P speed and remote lanes share decode/validation but not recycle policy:

```text
shared: direct decode, cookie/state validation, owner comparison
local:  owner-local TLS object-node push/pop
remote: owner inbox, remote batch, or handoff queue
RSS:    cap/release/decommit policy
```

Do not treat a local-speed candidate as a remote-free candidate without a
producer/consumer row. Do not move remote inbox or RSS release policy into the
local-speed reference only for code sharing.

RSS retain note:

```text
rssretain2048:
  TLS cap 1, global cap 2048

rssretain2048tls:
  TLS cap 2048, global cap 2048

rssretain2048array:
  TLS cap 2048, global cap 2048, owner-local TLS pointer array
```

The TLS-cap variant is the current RSS reporting row. On
`local2p_rsstls2048_runs10_20260524_041238`, it improved RSS plateau from
about `321.5K` to `325.3K ops/s`, essentially matching HZ4 in that run, but it
did not close the tcmalloc gap.

Stop rule: do not add more RSS knobs by default. The only optional continuation
is one B-lite retained pointer-array experiment; if it does not exceed
`rssretain2048tls` RSS throughput by at least 5% without local/mixed/safety
regression, stop allocator work and move to paper/reproducibility.

Measured B-lite result: `hz5-local2p-rssretain2048array` did not meet the stop
rule on `local2p_rssarray_runs5_20260524_050407`; RSS plateau was lower than
`rssretain2048tls` (`316.4K` vs `321.3K ops/s`). Keep it diagnostic-only.

## Benchmark Profiles

### `paper-main`

Purpose:

```text
general/default allocator claim
```

Allowed allocators today:

```text
hz3
hz4
mimalloc
tcmalloc
system
```

HZ5 is excluded until there is a true general LD_PRELOAD-compatible HZ5 lane.
The hybrid preload bridge is not enough because paper-shape hit probes show
near-zero Local2P hits.

The full preload control lane fixes attribution but is still excluded as a
performance row until `smallfront_s1` or a later general front-end replaces the
ordinary small/mid wrapped-mmap path.

### `appendix-hz5`

Purpose:

```text
narrow exact-overaligned HZ5 profile claim
```

Primary rows:

```text
local-only 64K/a8192:
  hz5-local2p-linkflags vs hz5-p25 vs hz4/tcmalloc/mimalloc/system

mixed-prelude final 64K/a8192:
  hz5-local2p-linkflags checks whether exact speed survives route pressure

RSS plateau 64K/a8192:
  hz5-local2p-rssretain2048tls vs hz4/tcmalloc/mimalloc/system

producer/consumer remote-free 64K/a8192:
  hz5-local2p-remotebatch vs hz5-p25 vs hz4/tcmalloc/mimalloc/system

guard rows:
  verifies unsupported routes fail closed
```

Remote-free and RSS rows are allowed in the appendix only with their matching
profile lane. Do not use `linkflags` as a remote-free or RSS-throughput result.

### `diagnostic-hz5`

Purpose:

```text
implementation decisions only
```

Examples:

- trace counters
- perf stat
- p25attr no-cookie/no-CAS/read-only variants
- p43 token/tokenbridge/trustwrap controls
- preload hit-rate probe
- single-run smoke

Diagnostic rows are not paper claims.

Local2P-specific diagnostic switches:

```text
--linux-local2p-no-cookie
--linux-local2p-no-cas
```

These are cost-isolation builds only. Keep them out of appendix defaults.

Local2P speed candidates:

```text
--linux-local2p-fast
--linux-local2p-object-node
--linux-local2p-same-owner-fast-state
--linux-local2p-route-cookie
--linux-local2p-reuse-state-only
--linux-local2p-slim-check
--linux-local2p-fast-cookie
--linux-local2p-tls-fast-return
--linux-local2p-exact-api
--linux-local2p-single-slot-tls
--linux-local2p-speed-linkflags
--linux-local2p-free-first
--linux-local2p-freefirst-fastcookie
```

`--linux-local2p-object-node` stores the recycle `next` pointer in freed user
memory and skips invariant wrapper-prefix rewrites on cached reuse. It keeps
the Local2P cookie/state checks in the first A/B candidate.

`--linux-local2p-same-owner-fast-state` builds on object-node and keeps remote
frees on locked atomic exchange, but owner-local frees use atomic load/store for
the ACTIVE->FREED state transition. Treat it as a speed candidate until the
safety contract is finalized.

`--linux-local2p-route-cookie` builds on fast-state and skips the generic
wrapper-cookie check in the direct Local2P decode. The Local2P cookie remains
the route attribution guard in `local2p_free`.

`--linux-local2p-reuse-state-only` builds on route-cookie and, for TLS cache
hits, only changes `local2p_state` back to ACTIVE. It does not rewrite owner,
generation, or Local2P cookie on the owner-local reuse path.

`--linux-local2p-slim-check` builds on reuse-state-only and treats the direct
Local2P decode as the source/requested/raw_bytes route guard. `local2p_free`
keeps the Local2P cookie and state checks but skips duplicate header field
checks.

`--linux-local2p-fast-cookie` builds on slim-check and uses a lightweight
Local2P cookie based on raw, aligned, and the process secret. It keeps mutated
cookie fail-closed behavior but intentionally drops generation/owner/raw-bytes
from the cookie mix for the speed A/B.

`--linux-local2p-tls-fast-return` builds on fast-cookie and short-circuits the
owner-local TLS hit path after restoring `local2p_state=ACTIVE`. It skips the
raw/bounds/header-init path only for object-node TLS hits; inbox/global/OS paths
still refresh metadata normally. Current decision: this is the public-API-shape
control, superseded by `linkflags` for appendix local/mixed speed reporting.

`--linux-local2p-exact-api` builds on tls-fast-return and makes the aligned64k
standalone benchmark call `hz5_local2p_alloc_64k_a8192()` and
`hz5_local2p_free_64k_a8192()`. It is a local-only exact API measurement lane,
not a general allocator or LD_PRELOAD lane.

`--linux-local2p-single-slot-tls` builds on exact API and specializes
`TLS_CAP=1` owner-local cache operations to `head` only. It avoids the local
`count` and `node->next` maintenance. Current decision: mixed-prelude
candidate, not local-only speed reference.

`--linux-local2p-speed-linkflags` builds on exact API and changes only
compile/link flags to reduce semantic interposition, PLT, stack protector, and
x86 CET overhead in the speed lane. Current decision: low-final-RSS local/mixed
speed reporting profile after RUNS=30 confirmation, not a production/default
build policy.

`--linux-local2p-free-first` builds on fast-cookie and moves Local2P direct
free decode before the generic P1/P2 ownership check. This is a dispatch-order
A/B for standalone Local2P speed lanes, not a remote/RSS policy change.

`--linux-local2p-freefirst-fastcookie` is an explicit alias for
`--linux-local2p-free-first`. Use the longer name in result tables when the
important point is that free-first is being measured on top of fast-cookie, not
as an independent route.

Current decision: `freefirst-fastcookie` is an A/B label only. It does not
replace the reporting profiles because the follow-up RUNS=10 result improved
local slightly but regressed mixed and remote.

## Decision Rules

Use these rules when adding a new result:

1. If it is not same-binary LD_PRELOAD and general-purpose, do not put it in
   `paper-main`.
2. If it only supports `65536:8192`, it belongs in `appendix-hz5`.
3. If it uses unsafe trust, disabled validation, hot counters, or route probes,
   it belongs in `diagnostic-hz5`.
4. If unsupported allocations fall back, label the row as fallback. Do not call
   it an HZ5 exact-route win.
5. If a preload run mostly delegates to libc, call it `libc+HZ5 hybrid`, not
   HZ5.

## Current Paper Wording

Allowed:

```text
HZ5 Local2P is a Linux exact-overaligned profile family for 64K/a8192. The
`linkflags` profile is tcmalloc-class on local/mixed exact throughput with low
final RSS, `rssretain2048tls` is the retained-cache RSS-throughput profile, and
`remotebatch` is the producer/consumer remote-free profile. These are appendix
profiles, not a general allocator claim.
```

Not allowed:

```text
HZ5 is generally faster than HZ3/HZ4/mimalloc/tcmalloc.
```

## Next Cleanup Target

Do not add another route knob until one of these is done:

1. Keep `p43_*`, `p25attr_*`, and old Local2P A/B lanes out of appendix default
   allocator lists.
2. Use `lane_metadata.tsv` from focus runners before promoting any row.
3. Add route-attribution summaries to focus output when trace lanes are used.
4. If general HZ5 is required for paper-main, design a real general preload
   lane instead of extending `preload_hybrid`.

## Development Order

Current Ubuntu HZ5 development order:

```text
1. local2p-speed:
   direct free decode, object-node reuse, and instruction-count reduction

2. local2p-remote:
   owner inbox / MPSC remote-free queue

3. rss-control:
   separate release/decommit policy lane
```

Keep these as separate lanes. A win in one lane must not be presented as a win
in the other two.

`local2p-remote` candidate selector:

```text
--linux-local2p-owner-inbox
```

This is a producer/consumer remote-free candidate. It is not the default
`hz5-local2p-fast` speed lane until measured across local, mixed, remote, and
RSS profiles.
