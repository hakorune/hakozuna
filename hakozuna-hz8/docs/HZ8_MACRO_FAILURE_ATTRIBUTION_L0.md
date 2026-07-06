# HZ8MacroFailureAttribution-L0

Status: COMPLETE. Investigation memo, separate from the RSS evaluation. HZ8 is
NOT broken on its own public matrix; this attributes why it fails when
LD_PRELOADed through HZ10's macro preload matrix, and classifies each failure.

Cross-references: `hakozuna-hz10/docs/HZ10_HZ8_MACRO_RSS_CHECK_L0.md` (the HZ10
judgment that motivated this), `hakozuna-hz8/bench_results/20260707T_hz8_public_matrix_recheck_l0/`
(HZ8 green on its own matrix).

## Purpose and scope separation

- HZ8 remains a valid low-RSS allocator on its own public same-run matrix (16
  stable threads, sizes 16..65536, fixed iterations). Recheck median post RSS:
  small_interleaved_remote90 2.98MiB, main_interleaved_r90 4.65MiB,
  medium_interleaved_r50 4.05MiB, main_local0 3.54MiB, medium_local0 3.01MiB
  -- all well below tcmalloc. That fact is preserved.
- The macro failures are out-of-envelope workloads (LD_PRELOAD of foreign
  programs: python churn, redis, larson thread stress, xmalloc stress). This
  memo classifies each as BUG / LIMITATION / HARNESS MISMATCH / MEASUREMENT BUG
  and proposes only small HZ8-side opt-in fixes. Nothing is ported to HZ10.

## HZ8 design scope (why "LIMITATION" is even on the table)

Small 16..4096 (9 power-of-2 classes), medium 4097..65536, >128KiB falls back
to system malloc. 64KiB spans, 64GiB small-arena virtual reservation. RSS
return via `madvise(MADV_DONTNEED)` on retirement/owner-exit; retained-empty
budget ~212MiB. Owner lifecycle via `pthread_key_create` destructor,
`H8_OWNER_MAX=64`. `h8_platform_commit` = `mprotect`; failure path = `abort()`
(`src/h8_arena_gate.c:82`). README states HZ8 is "not a universal tcmalloc
replacement" and targets stable thread pools. Preload shim exports only
**malloc/calloc/realloc/free** (4 symbols).

## Per-row table

| row | symptom | reproducible? | root-cause candidate | classification | confidence |
|---|---|---|---|---|---|
| python_alloc | RSS 4.3GiB, wall 2.6s | yes (3 dirs + standalone) | 67,235 small spans committed, never decommitted under single-thread churn | LIMITATION | high |
| redis_setget | SIGSEGV on server startup | yes (standalone, deterministic) | allocator split: malloc=HZ8, malloc_usable_size=jemalloc | HARNESS MISMATCH (borderline BUG) | high |
| larson | abort rc=134, commit ENOMEM, ~913MiB | yes (deterministic, small-arena only) | small-arena span commit hits an over-committed kernel; HZ8 aborts on commit failure | LIMITATION + ENVIRONMENT | medium-high |
| xmalloc_test | livelock (100% CPU, no progress) | yes (standalone) | yield-spin livelock in owner-lifecycle/remote-free under 4-worker contention | BUG | medium-high |

## python_alloc -- LIMITATION (high)

Standalone repro mirrors the harness exactly:

```text
PYTHONMALLOC=malloc LD_PRELOAD=libhakozuna_hz8_preload.so python3 <80-loop 20000x bytearray (i%4096)+1 churn>
  glibc : wall 1.21s  max_rss 92 MiB
  HZ8   : wall 2.63s  max_rss 4,324,824 KiB  (4.3 GiB, ~47x glibc)
```

Attribution (two independent methods agree):

```text
/proc/$pid/smaps at peak: one anonymous region holds 4,285,952 KiB (99% of RSS);
                      [heap] is only 32 MiB. The region is the HZ8 small arena.

HZ8_DUMP_STATS=1 (new opt-in knob, see Artifacts):
  arena_reserved=68719476736 (64GiB)  arena_committed=4406312960 (4.28GiB)
  small_spans=67235   owners=1  orphan_spans=0  owner_exit=0
  -> 67,235 spans * 64KiB = 4.28GiB committed, all held by the single owner.
```

Root cause: HZ8 commits 64KiB spans on demand but decommits only on
retirement/owner-exit. Python's pattern is a single stable thread with
high-frequency 1..4096 churn and no owner exit, so spans are committed and
never returned. The ~40MiB live set is swamped by ~4.28GiB of committed-but-
retained spans (~100x). No accounting bug is visible: `arena_committed` equals
`small_spans * 64KiB` exactly.

Boundary: this is HZ8's design (retention tuned for the public matrix's stable
multi-thread rows), not a correctness defect. The fix is a scope expansion
(decommit idle spans under sustained commit pressure), proposed below, not a
bug fix.

## redis_setget -- HARNESS MISMATCH (high; borderline BUG)

Reproduced deterministically on startup (no client load needed):

```text
LD_PRELOAD=libhakozuna_hz8_preload.so redis-server --daemonize no  -> SIGSEGV (rc=139)
glibc baseline                                                                  -> starts fine
```

gdb backtrace:

```text
#0 malloc_usable_size  () from /lib/x86_64-linux-gnu/libjemalloc.so.2   <- SIGSEGV here
#1 zmalloc             () (redis)
#2 createClient        ()
#3 moduleInitModulesSystem ()
#4 main                ()
```

redis bundles jemalloc (`malloc=jemalloc-5.2.1`, links `libjemalloc.so.2`).
HZ8's shim exports only malloc/calloc/realloc/free -- NOT `malloc_usable_size`.
So redis's `malloc` binds to HZ8 (preloaded) but its `malloc_usable_size` binds
to jemalloc (linked, the only provider). jemalloc's `malloc_usable_size` on an
HZ8-allocated pointer reads jemalloc metadata as garbage and segfaults. This is
a classic allocator-split under LD_PRELOAD onto a program that bundles its own
allocator.

HZ10's shim exports 8 symbols incl. `malloc_usable_size` -- which is why HZ10
runs redis clean in the same matrix. That inverts as proof: the gap is HZ8's
interposition surface.

Classification is HARNESS MISMATCH (LD_PRELOAD vs bundled allocator). It is
borderline BUG because a correct LD_PRELOAD malloc must export
`malloc_usable_size`; the crash, however, is in jemalloc, not HZ8 code.

## larson -- LIMITATION + ENVIRONMENT (medium-high)

Deterministic, small-arena only:

```text
LARSON under HZ8 (arg order: seconds min max chunks rounds seed threads), rc + max_rss:
  threads=1 sec=2          -> rc=134 (SIGABRT)  ~913 MiB   "h8_platform_commit: Cannot allocate memory"
  threads=2 sec=2          -> rc=134            ~913 MiB
  threads=4 sec=2          -> rc=134            ~913 MiB
  threads=4 sec=1          -> rc=134            ~913 MiB
  threads=1 chunks=8       -> rc=134            ~913 MiB   (working set does not matter)
  sizes 4097..8192 (medium)-> rc=124 (timeout, ran full 10s, NO abort)
  glibc threads=4 sec=2    -> rc=0              272 MiB
```

`h8_span_commit_memory` (`src/h8_arena_gate.c:78`) does `mprotect` + `abort()` on
failure; there is NO internal budget check before the commit. The abort is the
OS refusing the commit. System state on this machine:

```text
CommitLimit:   8,680,404 KiB (~8.3GiB)
Committed_AS: 15,584,696 KiB (~14.9GiB)  -- already OVER CommitLimit
overcommit_memory: 0 (heuristic)
```

So the kernel is already over-committed 1.8x; larson's rapid small-arena span
commit pushes mprotect past the heuristic and it returns ENOMEM; HZ8's
fail-closed policy turns that into SIGABRT. The ~913MiB figure is therefore
machine-dependent, not portable (a machine with headroom would abort later or
not at all -- python reached 4.3GiB without aborting on the same machine, just
at a different moment of pressure).

NOT thread-churn (reproduces at threads=1), NOT `H8_OWNER_MAX` slot exhaustion,
NOT an HZ8 internal budget. Two facets: (a) LIMITATION -- HZ8's small-arena
retention under high-frequency small-alloc churn plus abort-on-commit-failure;
(b) ENVIRONMENT/MEASUREMENT -- the over-committed test machine makes the
absolute threshold non-portable.

## xmalloc_test -- BUG (medium-high): livelock

```text
LD_PRELOAD=HZ8 xmalloc-test -w 4 -t 2 -s -1   -> SIGKILL at 15s, max_rss 1.9MiB (barely started)
glibc baseline                                 -> rc=0, wall 2.04s, 199MiB
```

Workers run at 101% CPU (livelock, not a blocked deadlock). perf hot frames:

```text
9.74% xmalloc-test mem_allocator
5.69% HZ8 h8_owner_lifecycle_enter
5.10% kernel __schedule
4.96% kernel update_curr
4.94% kernel pick_next_task_fair
3.74% HZ8 h8_remote_free_publish_known
3.37% kernel do_sched_yield
3.16% HZ8 h8_span_collect_remote
2.57% HZ8 h8_malloc_inner
2.38% HZ8 h8_owner_lifecycle_exit
...     + __sched_yield (userspace)
strace: 554,989 syscalls in 6.5s -- a sched_yield spin
```

A `sched_yield` spin inside HZ8's owner-lifecycle / remote-free-publish /
span-collect path does not converge under xmalloc's 4-worker contention storm:
the workers burn 100% CPU making no allocation progress (RSS stuck at 1.9MiB),
so the 2-second workload never completes. This is a real livelock defect, not
merely "slow" -- there is no forward progress. The harness's lack of a timeout
meant the process ran until an external SIGTERM (the 211s in the original log).

## Harness and measurement findings (apply to all rows)

```text
1. run_hz10_macro_preload_matrix.sh has `set -e`: the first HZ8 non-zero rc
   aborts the WHOLE matrix. redis/larson/xmalloc showing "single run only" is
   partly this artifact, not proof of non-reproducibility. -> per-allocator /
   per-run isolation so one failure does not mask the rest.

2. run_timed has NO timeout. xmalloc's 211s wall is an EXTERNAL kill, not a
   measured hang; the "2s contract" is xmalloc-test's own -t 2. -> wrap
   bounded-workload rows in `timeout`.

3. ru_maxrss (python, xmalloc via run_timed) vs current RSS (larson via
   run_timed_sampled) are different metrics and must not be cross-compared.

4. HZ10_SHIM_TOLERATE_FOREIGN=1 is set for HZ8 too; HZ8 ignores it. Harmless
   but HZ10-centric.

5. HZ8's shim exports only 4 symbols (the redis cause). HZ10 exports 8.

6. The test machine is over-committed (Committed_AS 14.9GiB > CommitLimit
   8.3GiB). Absolute RSS/abort thresholds here are not portable; macro runs
   need commit headroom to be valid.
```

## Small fix proposals (HZ8 opt-in lanes only; do NOT port to HZ10)

```text
F1 (redis)   DONE in `HZ8_PRELOAD_SHIM_SURFACE_F1.md`: export
             malloc_usable_size from the HZ8 shim (return slot/span size for
             HZ8-owned pointers; dlsym(RTLD_NEXT) for foreign), and add
             posix_memalign/aligned_alloc/memalign for surface completeness.
             Redis startup now reaches "ready to accept connections" under
             LD_PRELOAD instead of crashing in jemalloc malloc_usable_size.
F2 (larson)  add an opt-in graceful-commit-failure policy: instead of abort on
             h8_platform_commit ENOMEM, return NULL / fall back (the caller
             already handles NULL). Keeps fail-closed as default; opt-in for
             macro/foreign runs. Separate from the retention question.
F3 (larson)  investigate a decommit-under-pressure path for the small arena
             (decommit idle spans when arena_committed exceeds a budget). This
             is the retention lever shared with python. RESEARCH box, not a
             quick fix -- measure with the dump knob first.
F4 (xmalloc) NO-GO in `HZ8_REMOTE_TRANSITION_PUBLISH_F1.md`: a one-shot
             span-publish-lease assist before OWNER_TRANSITION kept smokes
             green but did not move the xmalloc repro (still 15s kill, no
             progress, ~1.8MiB RSS). Code reverted. Next work should target
             owner lifecycle lease contention directly, with counters before
             another repair.
Diagnostics  keep HZ8_DUMP_STATS (this box) and add an owner-slot-occupancy /
             remote-queue-depth dump to support F2/F4 diagnosis.
F4b (xmalloc) GO as opt-in in `HZ8_REMOTE_SPAN_LEASE_PUBLISH_L0.md`:
             replace the owner-wide remote publish lease with a span publish
             lease in `libhakozuna_hz8_preload_remotespanlease.so`, plus
             bounded transition backoff. xmalloc completed 5/5 instead of
             15s freeze, and the HZ8 public-row RSS guard stayed in the
             2.9-4.8MiB post-RSS band. Default enablement remains a separate
             decision box.
```

Each is a separate small box with its own gate (HZ8 smokes + standalone + the
relevant repro). None is implemented here; none touches HZ10.

## Verdicts

```text
python_alloc  : LIMITATION  (small-arena retention under single-thread churn;
                             ~100x live set committed; no decommit trigger fires).
                             Boundary: HZ8's design scope. Proposed F3.
redis_setget  : HARNESS MISMATCH (allocator split: HZ8 malloc + jemalloc
                             malloc_usable_size; SIGSEGV in jemalloc). Proposed F1.
                             Borderline BUG (LD_PRELOAD malloc should export
                             malloc_usable_size).
larson        : LIMITATION + ENVIRONMENT (small-arena commit hits an over-committed
                             kernel; HZ8 aborts on commit failure). Proposed F2;
                             threshold is machine-dependent.
xmalloc_test  : BUG  (sched_yield livelock in owner-lifecycle/remote-free under
                             4-worker contention; no forward progress). Proposed F4.

Harness/measurement: set -e + no-timeout + RSS-metric mix + over-committed host
                             all contributed to the appearance/severity. Proposed
                             harness fixes (isolation, timeout wrapper) are separate.
```

## Artifacts added by this box

```text
src/h8_preload_boundary.c : HZ8_DUMP_STATS opt-in atexit dump (default-off;
                             reads only existing H8Stats fields). Build clean
                             with -Werror; preload-smoke green; default lane
                             byte-identical (verified: no dump without the env).
                             Used for the python attribution above.
```

HZ8's own public matrix recheck remains green and unchanged.
