# HZ10MacroMatrixExpand-L0

Status: scoped design, not implemented.

Purpose: move the next decision from micro rows to a product-shaped
LD_PRELOAD matrix. This single box should answer both pending questions:

```text
Q1: Is HZ10_ENABLE_FINE_SIZE_CLASSES useful as a shim/product opt-in lane?
Q2: Is the v0 no-destructor thread-exit policy expensive enough to open an
    ownership-handoff/thread-exit design box?
```

The box is intentionally measurement infrastructure first. It must not
change allocator policy or thread-exit ownership.

## Why This Box

The recent RSS work closed the obvious micro-tuning loops:

```text
fine classes:       NO-GO as allocator default; useful macro/RSS opt-in.
retired-local hook: NO-GO as default; diagnostic/footprint lane only.
front cache:        opt-in only; not a fix for python_alloc RSS.
F3 handoff:         closed until macro/thread-churn evidence proves value.
```

The remaining questions require macro evidence. `python_alloc` already
showed that HZ10 can be faster than glibc while heavier in RSS; Redis was
too flat to rank speed. The next signal should be a wider matrix with:

```text
workloads:  python_alloc, redis_setget, larson
allocators: glibc, tcmalloc, mimalloc, hz10, hz10+fine
```

`hz10+fine` prices the fine-class shim lane directly. `larson` prices
thread churn and the no-destructor retained-page liability.

## Box Boundary

Keep the boundary in `scripts/run_hz10_macro_preload_matrix.sh` plus
Makefile preload artifacts.

Allowed changes:

```text
- Add sibling shared-library build products such as libhz10_fine.so.
- Add allocator columns to the macro matrix.
- Add larson as a workload row when a local executable is found.
- Add current-RSS sampling for long-running/server/thread-churn rows.
- Record median tables in bench_results and docs.
```

Not allowed in this box:

```text
- Do not enable fine classes by default.
- Do not change thread-exit behavior.
- Do not add automatic destructors.
- Do not rewrite allocator internals to improve a macro row discovered here.
```

## Step 1: Non-Clobbering Shim Libraries

Current Makefile targets `preload`, `preload-fine-size-classes`,
`preload-retired-local`, and `preload-fine-retired-local` all write the
same `libhz10.so` soname/output. That is fine for single-lane probes, but
it prevents a matrix from comparing default HZ10 and fine HZ10 in the same
run.

Add sibling artifacts:

```text
libhz10.so        default HZ10
libhz10_fine.so   HZ10_ENABLE_FINE_SIZE_CLASSES=1
```

The fine sibling should use its own soname:

```text
-Wl,-soname,libhz10_fine.so
```

The existing clobbering convenience targets may stay for manual probes, but
the matrix must depend on stable sibling outputs so allocator columns are
unambiguous and reproducible.

Initial matrix allocator list:

```text
glibc:       no LD_PRELOAD
hz10:        ${ROOT}/libhz10.so
hz10+fine:   ${ROOT}/libhz10_fine.so
tcmalloc:    hz10_bench_find_tcmalloc_lib, SKIP if absent
mimalloc:    source-build candidate only, SKIP if absent
```

Do not use the known-bad Ubuntu `libmimalloc2.0` package as the mimalloc
representative.

## Step 2: Larson Row

Use local artifacts only; do not fetch anything in this box. Known local
candidates observed on this machine:

```text
/mnt/workdisk/public_share/hakmem/larson_system
/mnt/workdisk/public_share/hakmem/mimalloc-bench/out/bench/larson
/mnt/workdisk/public_share/hakmem/mimalloc-bench/bench/larson/larson.cpp
```

The runner should select a prebuilt executable if present and otherwise
print a SKIP note. If the source tree exists but no executable exists, a
later box may add a build step; keep this box focused on matrix plumbing
unless the build is already established locally.

Larson is the priority row because it exercises thread churn and allocator
retention in a way `thread_reuse_bench` does not. `thread_reuse_bench`
does not exit threads in the macro shape and remains a micro diagnostic.

Required output for the larson row:

```text
workload
allocator
run
wall_sec
max_rss_kb
current_rss_kb, when the process can be sampled
larson command line / shape parameters
```

Use medians. Default `RUNS` should be at least 5 for the expanded macro
matrix; use 10 if the row is noisy enough to affect a GO/NO-GO decision.

## Thread-Exit Liability Measurement

For retained/orphan liability, `ru_maxrss` is not enough because it is a
high-water mark. The matrix should record `current_rss_kb` for rows where a
process lives long enough to sample.

The key comparison is:

```text
debt = hz10 default current_rss_kb - hz10 reclaim/reference current_rss_kb
```

Reference lanes:

```text
glibc/tcmalloc/mimalloc: external allocator baselines.
hz10: v0 no-destructor behavior.
hz10+fine: class policy opt-in.
HZ10_THREAD_EXIT_RECLAIM=1 or equivalent explicit-quiescent lane:
  only if an existing safe target exists. If not, record as TODO and do
  not invent a destructor in this box.
```

If no safe reclaim-on-exit lane exists yet, the matrix still has value:
compare HZ10 default/fine current RSS against external allocators and use
the row to decide whether an ownership-handoff design deserves to open.

## Later Rows

After `hz10+fine` and `larson` are in place, the same LD_PRELOAD pattern can
extend to the mimalloc-bench subset:

```text
xmalloc-test
cache-scratch
mstress
```

These are lower priority than larson. They should thicken the D4 evidence
table after the fine-lane and thread-churn decisions have initial numbers.

Do not promote tiny `grep`/`git status` smoke commands into headline macro
rows yet. They are useful compatibility smokes but too allocator-light for
performance decisions.

## Decision Gates

Fine-class shim lane:

```text
GO as opt-in shim lane if hz10+fine improves RSS on python_alloc and at
least one additional macro row without a material wall-time regression or a
large retained/current-RSS penalty.

NO-GO if the gain is isolated to python_alloc or if larson/thread-churn
shows worse retention/wall behavior than default HZ10.
```

Thread-exit ownership design:

```text
OPEN if larson shows HZ10 current RSS materially above external allocators
or above a safe reclaim/reference lane, and the difference persists across
median runs.

KEEP CLOSED if HZ10 current RSS is close to the external allocators or the
delta is smaller than the expected implementation/contract risk.
```

Macro headline:

```text
Do not claim product-level allocator competitiveness from one workload.
Promote only a table that includes at least python_alloc, redis_setget, and
larson, with explicit RUNS and median wall/RSS/current-RSS columns.
```

## Required Records

For the implementation run, record:

```text
bench_results/<stamp>_hz10_macro_matrix_expand_l0/combined.log
bench_results/<stamp>_hz10_macro_matrix_expand_l0/summary.tsv
bench_results/<stamp>_hz10_macro_matrix_expand_l0/median_summary.tsv
current_task.md restart pointer
docs/HZ10_PRELOAD_SHIM_DESIGN_L0.md short result section
docs/HZ10_NO_GO_LEDGER.md entries for any closed candidate
```

Keep the implementation reversible: one Makefile artifact group and one
runner expansion. No allocator semantic changes belong in this box.

## Implementation Record 20260707

Implemented:

```text
Makefile:
  preload-fine -> libhz10_fine.so with soname libhz10_fine.so.
  bench-macro-preload now depends on both libhz10.so and libhz10_fine.so.

scripts/run_hz10_macro_preload_matrix.sh:
  allocator column hz10+fine.
  larson workload row, using local prebuilt larson_system when present.
  sampled current_rss_kb for larson and redis server RSS rows.
```

Validation:

```text
make -B -C hakozuna-hz10 preload preload-fine smoke-shim-api smoke-shim-foreign
bash -n scripts/run_hz10_macro_preload_matrix.sh
short RUNS=1 matrix smoke with python/redis/larson
```

First expanded matrix:
`bench_results/20260707T010000Z_hz10_macro_matrix_expand_l0/`
with `RUNS=3`, `PYTHON_LOOPS=80`, `REDIS_OPS=20000`,
`LARSON_SECONDS=2`, `LARSON_MIN=8`, `LARSON_MAX=128`,
`LARSON_CHUNKS=128`, `LARSON_THREADS=4`.

Median excerpt:

```text
workload      allocator   wall_sec  max/current_rss_kb  wall/glibc
python_alloc  glibc         1.220          92016          1.000
python_alloc  hz10          0.960         116812          0.787
python_alloc  hz10+fine     0.990         106576          0.811
python_alloc  tcmalloc      0.830         104448          0.680
python_alloc  mimalloc      0.700         102540          0.574

larson       glibc          4.145         272384          1.000
larson       hz10           4.678        7817984          1.129
larson       hz10+fine      4.722        9238144          1.139
larson       tcmalloc       4.150         278912          1.001
larson       mimalloc       4.161         283928          1.004

redis_server_rss_kb:
  glibc 6924, hz10 7576, hz10+fine 8160, tcmalloc 10924, mimalloc 10036
```

Larson throughput is recorded in `summary.tsv` as `larson_throughput`.
Median-ish values from the first run set:

```text
glibc/tcmalloc/mimalloc: ~2.095M ops/s
hz10:                   ~1.061M ops/s
hz10+fine:              ~0.971M ops/s
```

Initial read:

```text
- The matrix plumbing works and gives side-by-side default/fine HZ10 rows.
- Fine classes still improve python_alloc RSS by about 10MB, but they do not
  look like a broad shim win: wall is slightly worse than default HZ10 on
  python_alloc, redis server RSS is higher, and larson RSS/wall/throughput
  are worse.
- The larson current-RSS delta is much larger than the expected noise:
  HZ10 is ~7.8GB vs external allocators around ~0.27-0.28GB in this shape.
  This is enough to open a focused thread-churn/orphan attribution box, but
  not yet enough to choose a fix.
```

Next box recommendation:

```text
HZ10LarsonThreadChurnAttribution-L0:
  Keep the macro runner as the scale. Use HZ10_SHIM_EXIT_STATS and a
  smaller larson shape sweep to split the 7-9GB RSS into live allocations,
  owner-thread orphan pages, page-pool retention, metadata slabs, and class
  rounding. Only after that decide whether thread-exit ownership handoff or
  a larson-specific retention cap is the right fix.
```
