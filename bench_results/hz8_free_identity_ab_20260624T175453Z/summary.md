# HZ8 Medium Free Direct Identity A/B

Behavior candidate: `MediumFreeDirectIdentityShape-L1`.

Baseline:

```text
0b9b30fd Collapse HZ8 medium active allocation checks
```

Candidate:

```text
same-owner medium free reuses one owner-word load / owner-match decision
instead of repeated h8_medium_run_owned_by_ctx calls on the direct path
```

Harness:

```text
bench/out/bench_matrix_malloc
LD_PRELOAD switched between baseline and candidate libraries
RUNS=10
T=16
I=100000
allocator order alternated by run
```

## Result

```text
main_i0:
  base 109.61M
  cand 110.82M
  ratio 1.0111

medium_i0:
  base 99.53M
  cand 104.88M
  ratio 1.0537

medium_i1:
  base 95.06M
  cand 100.17M
  ratio 1.0538

fixed16:
  base 111.19M
  cand 114.18M
  ratio 1.0269
```

Promotion decision:

```text
MediumFreeDirectIdentityShape-L1:
  GO
```
