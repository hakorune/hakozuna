# ChatGPT Pro Prompt: S110 Free PTAG32 Optimization

## Goal
Reduce `hz3_free()` overhead in the **Perf-First lane (small max 2048)** by optimizing the **PTAG32 leaf fast path**. Current perf shows `hz3_free` ~14.08% (top hotspot) while `hz3_malloc` is ~13.10%. The target is a **+3–5% throughput** gain on MT remote workloads without increasing instruction count on the hot path.

## Constraints (Box Theory)
- **Do not** add new function boundaries in the hot path (calls increase instruction count).
- **Do not** add arena-base loads or range checks (S113 segmath NO-GO was slower due to these).
- Keep changes **A/B guardable** via compile-time flags.
- **No** always-on logging; SSOT/one-shot only if needed.
- Fail-fast only behind debug flags.

## Past NO-GO (Lessons)
- **S113 SegMath**: slower than PTAG32 due to `arena_base` load + extra checks.
- **S16-2D (Shape Split)**: +1.68% but +3.4% instructions → NO-GO.
- **S16-2C (Tag decode lifetime)**: no effect.
- **S114 (PTAG32 TLS cache)**: -0.7% (TLS load slower than global load).
- **S140 (PTAG32 leaf micro-opt)**: fused decode + branch inversion regressed, especially remote-heavy (IPC collapse / branch-miss).

## What we want from you
1. **2–3 concrete low-risk micro-optimizations** for the PTAG32 leaf path.
2. For each: expected benefit, risk, and an **A/B flag** suggestion.
3. Any **tag layout** or **bit-extract** ideas that reduce instructions or branches.
4. A short **perf test plan** (which counters to watch: `instructions`, `branch-misses`, `L1-dcache-load-misses`).

## Key Code (in bundle)
- `hakozuna/hz3/src/hz3_hot.c` — `hz3_free()` entry + PTAG leaf dispatch.
- `hakozuna/hz3/src/hot/hz3_hot_dispatch.inc` — `hz3_free_try_ptag32_leaf()`.
- `hakozuna/hz3/include/hz3_arena.h` — PTAG32 encode/load/lookup helpers.
- `hakozuna/hz3/include/hz3_tcache.h` — bin push helpers.
- `hakozuna/hz3/src/hz3_arena.c` — PTAG32 storage/arena setup.

## What NOT to do
- Do not propose seg-header or segmath-based bypass (S110-1 was NO-GO).
- Avoid adding branches that increase instruction count.
- Avoid TLS caching of arena base (S114 was NO-GO).

## Deliverable format
- Bullet list of proposals with: **change**, **why**, **expected perf effect**, **A/B flag**, **test**.
