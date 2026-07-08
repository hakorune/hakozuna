# HZ11Sh6benchSizeClassPolicyCandidate-L1

## Box

```text
Goal:
  Add an opt-in finer size-class policy candidate for the sh6bench-heavy
  request distribution while preserving the batch32 transfer behavior.

Boundary:
  size-class policy only
  opt-in sibling only
  no macro promotion claim

Candidate:
  libhz11_span_transfer_thread_exit_cap_batch32_fineclass.so
```

## Design

Keep the current HZ11 default size policy unchanged:

```text
default:
  16, 32, 64, 128, ..., 65536
```

Add an opt-in fine-class policy for the candidate lane:

```text
HZ11_FINE_SIZE_CLASSES=1:
  16, 32, 48, ..., 1024
  2048, 4096, 8192, 16384, 32768, 65536
```

This targets the observed sh6bench request-to-slot waste:

```text
requested bytes high-water:      211116531
HZ11 slot bytes high-water:      359344128
request-to-slot waste:           148227597
request-to-slot waste ratio:          41.2%
```

Class 0 still has a 16-byte minimum slot. This box does not attempt a packed
sub-16-byte allocator or an alignment-specialized tiny lane.

## Required Evidence

```text
sh6bench wall/RSS versus tcmalloc and batch32
requested-to-slot waste improvement versus batch32 live diagnostic
span_create and transfer counters
focused correctness rows before any macro gate
```

## Decision Rule

GO as a candidate only if:

```text
sh6bench RSS improves materially
sh6bench wall does not regress enough to erase the batch32 wall win
python_alloc and mstress still pass in a focused correctness check
```

No macro promotion is allowed from this box alone. A later full macro gate is
required even if the focused results are good.

## Evidence

Commands:

```sh
RUNS=3 hakozuna-hz11/scripts/run_hz11_sh6bench_size_class_policy_candidate.sh

RUNS=3 BUILD=0 RUN_LARSON=0 RUN_XMALLOC=0 RUN_SH6BENCH=0 \
  RUN_CACHE_SCRATCH=0 RUN_PYTHON_ALLOC=1 RUN_MSTRESS=1 \
  hakozuna-hz11/scripts/run_hz11_macro_speed_lane_gate.sh \
  --allocators tcmalloc,hz11-thread-exit-cap-batch32,hz11-thread-exit-cap-batch32-fineclass \
  --candidate hz11-thread-exit-cap-batch32-fineclass \
  --skip-span-soa-check --runs 3
```

Outputs:

```text
bench_results/hz11_sh6bench_size_class_policy_20260708T072222Z/summary.md
bench_results/hz11_macro_speed_lane_20260708T072459Z/summary.md
```

## Results

sh6bench RUNS=3:

```text
tcmalloc:
  wall 0.359s
  max RSS 267776 KiB

batch32:
  wall 3.542s
  max RSS 350592 KiB
  span_create 16750

batch32-fineclass:
  wall 3.563s
  max RSS 301568 KiB
  span_create 14613
```

Fineclass keeps the batch32 wall result essentially flat while reducing
sh6bench max RSS by about 49 MiB versus batch32.

Class-packing diagnostics:

```text
batch32 live-diag:
  requested bytes high-water 211160049
  slot bytes high-water      359394944
  request-to-slot waste      148234895
  waste ratio                41.2%

batch32-fineclass live-diag:
  requested bytes high-water 211345643
  slot bytes high-water      306816544
  request-to-slot waste       95470901
  waste ratio                31.1%
```

Focused correctness rows RUNS=3:

```text
python_alloc:
  tcmalloc wall 0.032s, max RSS 8064 KiB
  batch32-fineclass OK 3/3, wall 0.032s, max RSS 7040 KiB

mstress:
  tcmalloc wall 0.189s, max RSS 212736 KiB
  batch32-fineclass OK 3/3, wall 0.218s, max RSS 234916 KiB
```

The focused gate reports a python_alloc current-RSS ratio of 1.769x on a very
small current-RSS denominator. This does not block the candidate box, but it must
be rechecked in a full macro gate.

## Verdict

GO as an opt-in candidate only. No macro promotion.

Next box should run the full macro speed gate with:

```text
hz11-thread-exit-cap-batch32-fineclass
```

against tcmalloc and batch32, watching python_alloc current RSS, larson RSS,
xmalloc/cache_scratch side effects, and sh6bench wall/RSS.
