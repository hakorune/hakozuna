# HZ5 Linux Route / Lane Matrix

This document is the Linux HZ5 naming and classification source of truth.
Keep route, lane, and benchmark profile separate.

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

## Current Classification

| Name | Type | Status | Paper use |
| --- | --- | --- | --- |
| `local2p` | route | active Linux exact route | appendix only |
| `p25_bridge` | route | active control route | control/baseline |
| `p25attr` | lane/route variant | diagnostic/safety evidence | not a claim |
| `p43_token` | lane/route variant | remote/RSS candidate-watch | diagnostic until proven |
| `p43_tokenbridge` | lane/route variant | topology-mismatch evidence | diagnostic only |
| `p43_trustwrap` | lane/route variant | unsafe speed control | diagnostic only |
| `preload_hybrid` | adapter | libc+HZ5 diagnostic bridge | not paper-main |
| `hz3_fallback` | fallback route | disabled in standalone exact-only | not a HZ5 win |
| `libc_passthrough` | adapter route | used by preload hybrid misses | not HZ5 |

## Routes

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
- Not a remote-free profile.
- Not an RSS-throughput profile.
- Not a Windows P43i/P45 replacement.

Remote-free is no longer forced through OS `free(raw)` on every operation. The
current lane uses a small global recycle stack after remote free, but it is
still a simple control path, not an owner-inbox remote-free design.

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
| `hz5-local2p-fast` | `hz5-linux-local2p-fast` | `--linux-local2p-fast` | `local2p` | current appendix speed candidate |
| `hz5-local2p-object` | `hz5-linux-local2p-object-node` | `--linux-local2p-object-node` | `local2p` | speed candidate: user-pointer freelist node |
| `hz5-local2p-inbox` | `hz5-linux-local2p-remote-inbox` | `--linux-local2p-fast --linux-local2p-owner-inbox` | `local2p` | remote-free candidate |
| `hz5-local2p` | `hz5-linux-local2p` | `--linux-local2p` | `local2p` | baseline Local2P implementation |
| `hz5-p25` | `hz5-linux-p25-control` | no Local2P/P43/P25Attr selector | `p25_bridge` | Linux control |
| `p25attr` | `hz5-linux-p25attr` | `--linux-p25-bridge-attr` | `p25attr` | safety diagnostic |
| `p25attr-nocas` | `hz5-linux-p25attr-nocas` | `--linux-p25-bridge-attr-no-cas` | `p25attr` | cost diagnostic |
| `p25attr-nocookie` | `hz5-linux-p25attr-nocookie` | `--linux-p25-bridge-attr-no-cookie` | `p25attr` | cost diagnostic |
| `p43-token` | `hz5-linux-p43-token` | `--linux-p43-token` | `p43_token` | remote/RSS candidate-watch |
| `p43-tokenbridge` | `hz5-linux-p43-tokenbridge` | `--linux-p43-token-bridge` | `p43_tokenbridge` | diagnostic only |
| `p43-trustwrap` | `hz5-linux-p43-trustwrap` | `--linux-p43-trust-wrapper-source` | `p43_trustwrap` | unsafe control |
| `hz5-preload-hybrid` | `hz5-preload-hybrid-diagnostic` | hybrid preload library | `local2p` hits + `libc_passthrough` misses | diagnostic adapter |

Build selectors for `local2p`, `p25attr`, and `p43` are mutually exclusive.
Keep that rule. It prevents mixed-route benchmark rows.

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

### `appendix-hz5`

Purpose:

```text
narrow exact-overaligned HZ5 profile claim
```

Primary rows:

```text
local-only 64K/a8192:
  hz5-local2p-fast vs hz5-p25 vs hz4/tcmalloc/mimalloc/system

mixed-prelude final 64K/a8192:
  checks whether exact speed survives route pressure

guard rows:
  verifies unsupported routes fail closed
```

Remote-free and RSS rows are allowed in the appendix, but they must say that
Local2P is not currently the remote/RSS candidate.

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
```

`--linux-local2p-object-node` stores the recycle `next` pointer in freed user
memory and skips invariant wrapper-prefix rewrites on cached reuse. It keeps
the Local2P cookie/state checks in the first A/B candidate.

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
HZ5 Local2P is a Linux exact-overaligned local-throughput candidate for
64K/a8192. It reaches HZ4-class throughput on local-only and mixed-prelude
exact rows, but it is not a general allocator or remote-free profile.
```

Not allowed:

```text
HZ5 is generally faster than HZ3/HZ4/mimalloc/tcmalloc.
```

## Next Cleanup Target

Do not add another route knob until one of these is done:

1. Rename result labels to match the lane table above.
2. Keep `p43_*` and `p25attr_*` out of appendix default allocator lists.
3. Use `lane_metadata.tsv` from focus runners before promoting any row.
4. Add route-attribution summaries to focus output when trace lanes are used.
5. If general HZ5 is required for paper-main, design a real general preload
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
