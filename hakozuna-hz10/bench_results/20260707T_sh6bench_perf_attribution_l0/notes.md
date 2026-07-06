# HZ10Sh6benchPerfAttribution-L0

Status: attribution complete.

Input baseline:

```text
bench_results/20260707T_shim_speed_stack_macro_refresh_l0/
sh6bench RUNS=5 median:
  hz10     0.510s / 319,488 KiB
  tcmalloc 0.320s / 272,384 KiB
  mimalloc 0.250s / 272,844 KiB
```

## Commands

Perf stat:

```text
perf stat -x, -r 5 \
  -e cycles,instructions,cache-references,cache-misses,branches,branch-misses
```

Perf report:

```text
perf record -- env LD_PRELOAD=<allocator> sh6bench < sh6bench.in
perf report --stdio -g none --no-children
```

`sh6bench.in`:

```text
100000
1
1000
4
```

## sh6bench Stat

```text
allocator  cycles       instructions  cache-misses  branch-misses
hz10       15.553B      32.158B       118.302M      17.473M
tcmalloc    9.235B      18.310B       139.042M      14.856M
mimalloc    7.772B      13.239B        73.196M       9.279M
glibc      28.842B      59.744B        72.445M      68.934M
```

Read:

- HZ10 vs tcmalloc: 1.68x cycles and 1.76x instructions, but fewer cache
  misses.
- HZ10 vs mimalloc: 2.00x cycles and 2.43x instructions.
- The remaining sh6bench gap is primarily instruction count / entry shape,
  not cache locality.

## sh6bench Flat Profile

HZ10:

```text
34.78% hz10_free
22.66% hz10_malloc
 7.90% free
 5.74% malloc
 2.30% hz10_malloc@plt
 2.16% hz10_free@plt
 1.37% hz10_page_drain_remote
```

tcmalloc:

```text
30.48% tc_malloc
27.96% operator delete[](void*)
22.41% sh6bench doBench
```

mimalloc:

```text
29.24% sh6bench doBench
26.77% operator delete[](void*)
18.86% mi_malloc
```

The previous wrapper boxes are closed: `hz10_public_entry_current_owner*`,
`hz10_public_entry_owner_record`, and stats-marker helpers no longer appear.
The remaining visible HZ10 overhead is the shim entry (`malloc/free`) plus
public API body (`hz10_malloc/free`) and internal PLT edges.

## HZ10 Annotate Highlights

`hz10_malloc` hot path:

- fine size-class selection / `bsr`;
- TLS owner load;
- per-class state address arithmetic;
- active page local-free pop;
- local-free marker clear and `free_count--`.

`hz10_free` hot path:

- pagemap local route record loads;
- interior check including `offset % slot_size`;
- TLS owner load and owner-token compare;
- marker write, local free-list push, `free_count++`;
- stack-protector prologue/epilogue shows visible samples because the fallback
  route result lives on the stack.

## Bsymbolic Calibration

Probe only, not committed as a build change:

```text
make -B -C hakozuna-hz10 preload LDFLAGS='-Wl,-Bsymbolic-functions'
```

Five sh6bench runs:

```text
0.47s, 0.48s, 0.46s, 0.47s, 0.47s
```

Baseline was 0.51s median. The `@plt` entries for `hz10_malloc`,
`hz10_free`, and `hz10_page_drain_remote` disappear from the filtered report.
This proves internal symbol binding is a real but partial win, roughly 8% on
this row.

## mstress Cross-Check

Perf stat:

```text
allocator  cycles      instructions  cache-misses  branch-misses
hz10       2.999B      3.386B        20.294M       33.846M
tcmalloc   2.369B      2.480B        13.875M       13.267M
mimalloc   3.012B      2.530B        15.724M       15.404M
```

Flat profile shows benchmark body and kernel/page-fault work dominate
`mstress`; HZ10 allocator self time is small (`hz10_malloc` 2.47%,
`hz10_free` 1.34%). Do not optimize mstress first based on allocator-internal
guesswork.

## Verdict

Next implementation box should be a small shim entry/binding box, not a page
ownership or routing redesign:

```text
HZ10ShimInternalBinding-L0
  Option A: add -Wl,-Bsymbolic-functions to shim library builds.
  Option B: add hidden/internal hz10 malloc/free entry points and have the
            preload malloc/free wrappers call those directly.
  Gate: smoke, foreign-free fail-closed, codegen no internal @plt calls,
        sh6bench RUNS=5, all-allocator macro guard.
```

Expected win is bounded: Bsymbolic alone moves sh6bench about 0.51s -> 0.47s.
The remaining post-binding gap is inside `hz10_malloc/free` themselves:
size-class logic, pagemap route validation, marker writes, and metadata
updates.
