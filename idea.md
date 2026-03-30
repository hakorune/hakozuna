# Future Ideas

Shortlist of "interesting next uses" for Hakozuna that are worth exploring even
if Hakozuna does not always win.

The goal is not just to find a benchmark where `hz3` or `hz4` wins, but to find
real or app-like workloads where allocator behavior becomes understandable.

## Direction

- Prefer workloads where allocator behavior is visible.
- It is OK if Hakozuna loses, as long as the result teaches something.
- Use existing apps when possible.
- If a custom workload is needed, keep it allocator-neutral and publishable as a
  separate project.

## Candidate Ideas

### 1. Allocator-neutral benchmark project

Create a standalone workload bench outside the Hakozuna repo.

Why it is interesting:

- avoids the "self-serving benchmark" look
- keeps value even when Hakozuna loses
- can compare `hz3`, `hz4`, `mimalloc`, `tcmalloc`, `jemalloc`, `snmalloc`
- can focus on workload axes directly:
  - local vs remote free
  - churn
  - size mix
  - lifetime
  - thread scaling

Good first workloads:

- local churn
- mixed size churn
- cross-thread ring

### 2. Game-server-like state workload

Small real-time server with many short-lived objects and per-session state.

Why it is interesting:

- `hz3` may fit local-heavy state updates
- `hz4` may fit cross-thread event delivery
- easy to explain to readers
- can show both throughput and RSS

Possible shape:

- session map
- inventory/state updates
- chat/event dispatch
- worker-thread handoff

### 3. Event / JSON ingestion service

Build a small service that parses JSON events, transforms them, and stores or
forwards them.

Why it is interesting:

- many small allocations
- object churn is high
- allocator differences may show in throughput, p95, and RSS
- easier to build than a full database workload

### 4. Plugin / scripting host

Small host embedding Lua, Wasm, or a tiny DSL.

Why it is interesting:

- allocation-heavy in a realistic way
- portable across Linux, Windows, and macOS
- easy to run repeated scripts with controlled patterns

### 5. Queue / relay / pub-sub service

Message relay with worker handoff and backpressure.

Why it is interesting:

- remote-heavy path is easier to trigger
- can stress ownership transfer and free routing
- good place to compare `hz3` vs `hz4`

## Suggested Priority

1. Standalone allocator-neutral benchmark project
2. Small game-server-like workload
3. Event / JSON ingestion service

## What To Avoid

- giant infrastructure projects too early
- overly Hakozuna-specific benchmark shortcuts
- "win-only" benchmark design
- workloads where network, disk, or parser cost hides allocator behavior

## A Good Near-Term Path

1. Keep Hakozuna stable and documented.
2. Grow the standalone allocator-neutral bench repo.
3. Add one app-like workload that is small but believable.
4. Use real apps later to confirm what the smaller workloads suggest.
