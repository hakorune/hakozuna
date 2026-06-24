# Current Task

## Freeze Baseline

HZ8 small v0 is recorded as `hz8-small-v0-rc1`.

```text
rc1 record:
  docs/HZ8_SMALL_V0_RC1.md

matrix record:
  bench_results/hz8_same_run_matrix_20260623T174503Z/

archived detailed task log:
  docs/archive/current_task_20260624_small_v0_rc1.md
```

Small v0 behavior is frozen unless a hard safety issue is found.

```text
frozen:
  p2-v0 class map
  tagged slot_state authority
  pending bitmap / pending_word_mask / qstate protocol
  owner lifecycle
  purge policy
  startup-loaded Linux x86_64 ELF TLS contract
```

## Active Lane: MediumRun-v1

Goal:

```text
add >4KiB coverage and make main_* / cross128 rows meaningful
```

Current shape:

```text
range:
  4097..65536

classes:
  8K / 16K / 32K / 64K

identity:
  direct medium registry
  power-of-two slot decode
  interior pointers rejected

residency:
  empty run resident retention is budgeted
  owner exit drains retained empty payload

ownership:
  owner-attached runs publish owner tokens
  TLS active run must match current owner
  global allocation skips foreign-attached runs

remote:
  owner-attached foreign free uses owner lifecycle lease
  pending bit is remote claim authority
  per-owner medium pending queue transfers slot mutation to owner collect
  qstate uses IDLE / QUEUED / DRAINING / DRAINING_DIRTY
  detached direct-lock fallback remains
```

Key recorded evidence is archived in `bench_results/.../README.md`.

```text
owner affinity: active/list mismatch = 0
remote owned publish: attached remote frees use owner queue
collect cadence: eager call issue reduced
route authority: slot_state + pending authority clean
lockless publish shadow: match=29962 mismatch=0
```

## Current Box

### MediumUpper48KSizePolicyShadow-L1

Status: recorded; superseded by paired gate evidence.

```text
bench_results/20260624T_medium_upper48_shadow/README.md
upper48 rounded bytes ratio: 0.9045
run estimate unchanged: 5121
interpretation: RSS / first-touch evidence, not queue-episode evidence
```

Next decision:

```text
MediumUpper48KSizePolicyAB-L1:
  HOLD until paired hot-path A/B is explicitly worth the 48K decode cost

default:
  keep current p2 medium class map
```

### MediumRunOwnerLeaseCeiling-L1

Status: recorded.

```text
bench_results/20260624T094726Z_medium_owner_lease_ceiling/README.md
lease elision was +21% evidence only; direct promotion is NO-GO
```

### MediumRunOwnerLeaseSplitAudit-L1

Status: recorded.

```text
bench_results/20260624T095302Z_medium_owner_lease_split_audit/README.md
debug remote_lease_ms=11.120 enter=4.571 exit=6.549
```

### MediumRunCollectCadenceTuning-L1

Status:

```text
RECORDED
```

Goal:

```text
tune owner medium pending collect cadence after MPSC queue push
```

Scope:

```text
collect period/budget made compile-time overrideable
default changed from period=8,budget=4 to period=32,budget=8
```

Data:

```text
bench_results/20260624T093113Z_medium_collect_cadence_tuning/README.md
```

Result:

```text
release medium r50:
  period=8,budget=4:
    median 5.947M ops/s
    steady median 6.190M ops/s

  period=32,budget=8:
    median 6.714M ops/s
    steady median 7.002M ops/s

  default p32/b8 confirmation:
    median 6.869M ops/s
    steady median 7.191M ops/s

debug medium r50 p32/b8:
  remote_collect_call=9302
  remote_collect_run=71583
  remote_collect_slot=89696
  remote_collect_ms=6.670
  remote_qpush=71583
  remote_qpush_ms=4.609
  remote_lease_ms=7.408
  slots_per_run=[1.572,2.005,1.707,1.000]

small interleaved remote90 quick:
  median 49.283M ops/s
```

Interpretation:

```text
collect cadence was too eager
period=32,budget=8 improves medium r50 by allowing slot coalescing
64K class remains structurally one slot/run
remaining visible residual bucket is owner lifecycle lease and collect slot mutation
```

## MediumRunCollectWordCommitRearm-L1

Status: recorded.

Data:

```text
bench_results/20260624T100942Z_medium_collect_word_commit_rearm/README.md
```

Result:

```text
collector now commits one pending snapshot as a word:
  slot_state FREE publication
  allocated_mask batch clear
  pending word batch clear
  free_mask batch set
  remaining pending rearm/self-DIRTY

debug medium r50:
  collect_finish_pending_rearm=189
  qstate dirty set/self/requeue = 17/114/131
  empty_with_pending=0

release medium r50:
  median 7.583M ops/s
  steady median 7.867M ops/s

small interleaved remote90 quick:
  median 48.794M ops/s
```

Interpretation:

```text
collector finish race is closed by remaining-word rearm
collect mutation is now closer to run-word granularity
next safe lease step is shadowing a dedicated medium owner admission word
```

## MediumRunOwnerLeaseWordShadow-L1

Status: recorded.

Scope:

```text
debug-only owner-scoped medium_publish_ctl
existing owner control word remains authority
shadow enter decision compared against existing h8_owner_publish_enter
shadow refs closed and checked at owner exit
release hot path has no shadow call
```

Hard zero gates:

```text
medium_lease_enter_decision_mismatch = 0
medium_lease_ref_underflow = 0
medium_lease_ref_nonzero_at_owner_exit = 0
medium_lease_enter_after_close = 0
medium_owner_reuse_with_medium_refs = 0
```

Data:

```text
bench_results/20260624T110022Z_medium_owner_lease_word_shadow/README.md
```

Result:

```text
debug medium r50:
  decision_mismatch=0
  ref_underflow=0
  refs_at_exit=0
  enter_after_close=0
  reuse_with_refs=0

release medium r50:
  median 7.479M ops/s
  steady median 7.751M ops/s

small interleaved remote90 quick:
  median 50.412M ops/s
  steady median 55.011M ops/s
```

Interpretation:

```text
medium-specific owner admission word matches existing owner control decisions
owner exit observes zero shadow refs
next step can A/B this word as medium remote publish lease authority
```

## MediumRunOwnerLeaseWord-L1

Status: recorded, HOLD as default.

Data:

```text
bench_results/20260624T110834Z_medium_owner_lease_word/README.md
```

Candidate:

```text
release medium remote publish used owner->medium_publish_ctl
owner exit closed medium_publish_ctl and waited medium refs zero
release exit path used fetch_sub
debug still compared medium_publish_ctl decision with packed owner control
```

Result:

```text
debug medium r50:
  decision_mismatch=0
  ref_underflow=0
  refs_at_exit=0
  enter_after_close=0
  reuse_with_refs=0

release medium r50:
  candidate median 7.429M ops/s
  candidate steady median 7.751M ops/s

prior shadow baseline:
  median 7.479M ops/s
  steady median 7.751M ops/s

small interleaved remote90 quick:
  median 50.124M ops/s
```

Decision:

```text
do not default
safety proof passes, but performance is flat/slightly below baseline
keep debug shadow only; packed owner control remains medium remote authority
```

## MediumRunResidualCostReaudit-L1

Status: recorded.

Data:

```text
bench_results/20260624T112027Z_medium_residual_reaudit/README.md
```

Change:

```text
bench output only
added derived medium_residual line from existing counters
allocator behavior unchanged
```

Result:

```text
debug medium r50:
  lease_ns_per_pub=155.4
  claim_ns_per_pub=31.0
  qpush_ns_per_push=64.1
  collect_ns_per_run=91.6
  collect_runs_per_call=6.846
  collect_slots_per_run=1.219

class density:
  8K  slots/run=1.416
  16K slots/run=1.784
  32K slots/run=1.613
  64K slots/run=1.000

release medium r50:
  median 7.621M ops/s
  steady median 7.906M ops/s
```

Interpretation:

```text
lease is still the largest measured debug bucket, but lease-word A/B was flat
queue push fixed cost is visible, but retry pressure is low
collect cadence is already batching calls; remaining density is limited by 64K one-slot runs
```

## MediumRunSizePolicy/ChunkArenaEvidence-L1

Status: recorded.

Data:

```text
bench_results/20260624T113104Z_medium_size_policy_evidence/README.md
```

Change:

```text
bench output only
added medium class distribution counters
added simple current-run vs hypothetical two-slot-64K run-mix estimate
allocator behavior unchanged
```

Result:

```text
debug medium r50:
  throughput median 3.188M ops/s
  steady median 3.230M ops/s
  one_slot_alloc_ratio=0.531900
  one_slot_remote_ratio=0.531512
  current_runs=63529
  two_slot_64k_runs=39646
  two_slot_64k_ratio=0.624061

release medium r50:
  throughput median 7.345M ops/s
  steady median 7.589M ops/s

short debug phase r90:
  one_slot_alloc_ratio=0.537600
  one_slot_remote_ratio=0.540055
  current_runs=12818
  two_slot_64k_runs=7968
  two_slot_64k_ratio=0.621626
```

Interpretation:

```text
about 53-54% of medium objects are 64K one-slot class
one queue episode per remote free is structurally common for that class
lease-word A/B and queue retry audit do not point to another narrow queue/lease win
medium size policy / run geometry is now the stronger evidence lane
```

## Archived Medium Boxes

Detailed records live in each `bench_results/.../README.md`.

```text
025523 remote lockless claim: producer run mutex removed
085004 post-claim rollback closure: stranded-pending window closed
091112 single-writer shadow: active owner opportunity proven
091612 active owner lock elision: active alloc/free run lock elided
092050 pending queue MPSC: producer pending_lock push replaced
092503 MPSC retry audit: retry pressure about 0.22%
100942 collect word commit/rearm: finish race closed
110022 lease word shadow: dedicated medium lease shadow clean
110834 lease word A/B: safety-clean, performance flat, HOLD
112027 residual reaudit: no narrow queue/lease follow-up
113104 size-policy evidence: 64K one-slot class about 53-54%
```

## Next Boxes

```text
1. MediumChunkArenaQuantum-L1
   geometry lane next candidate after detached index / directory cap evidence
   do not reopen q64-run64k2 or upper48 as default without new gate evidence

2. MainFaultVarianceAttribution-L1
   main_interleaved_remote90 instability reproduces with high minor faults
   split medium one-run mmap / page-fault churn before changing size policy

3. MediumR50SlotCollectLane-L1
   medium r50 residual points to slot/collect/lease, not queue push first
```

## MediumV1GateRunner-L1

Status: implemented.

```text
script:
  scripts/run_medium_v1_gate.sh

make target:
  make medium-v1-gate

output:
  bench_results/*_medium_v1_gate/
```

Rows:

```text
medium_local0:
  4097..65536 remote_pct=0 interleaved=1

medium_interleaved_remote50:
  4097..65536 remote_pct=50 interleaved=1

medium_phase_remote90:
  4097..65536 remote_pct=90 interleaved=0

main_interleaved_remote90:
  16..32768 remote_pct=90 interleaved=1 live_window=4096
```

## MediumV1GateR10-L1

Status: recorded.

```text
bench_results/20260624T_medium_v1_gate_r10_medium_v1_gate/README.md
```

Result:

```text
medium_local0:
  median 13.04M ops/s

medium_interleaved_remote50:
  median 2.19M ops/s
  peak RSS 30.5MiB

medium_phase_remote90:
  median 140K ops/s
  peak RSS 65.1MiB

main_interleaved_remote90:
  median 21.8M ops/s
  p25 4.1M ops/s
  peak RSS 46.8MiB
```

Interpretation:

```text
MediumRun-v1 default is functional but not performance-ready
main row has severe stability variance
next implementation should target medium r50 / main instability
```

## MediumR50ResidualAttribution-L1

Status: implemented / short observed.

```text
bench_results/20260624T_medium_r50_residual_attribution/README.md
change: bench output only, added medium_residual_budget derived line
allocator behavior unchanged
debug R3: median 6.12M, attributed_ms 1133.828
split: slot 508.582ms, collect 316.914ms, lease 137.376ms, lock 100.512ms
release R3: median 5.77M, minor_faults_per_op 0.180497
interpretation: target slot/collect/lease or fault variance before queue push
```

## MainInterleavedStabilityAudit-L1

Status: recorded.

```text
bench_results/20260624T_main_interleaved_stability_audit/README.md
main_interleaved_remote90 R10:
  median 9.07M ops/s
  p25 2.83M ops/s
  minor_faults_median 713830
  work_median 394.178ms
  tail_median 48.266ms
interpretation: instability reproduced; page-fault variance is the next suspect
```

## MediumVariableRunGeometryScaffold-L1

Status: recorded.

```text
bench_results/20260624T123311Z_medium_variable_run_geometry_scaffold/README.md
behavior unchanged
directory now maps 64KiB quantum -> containing run
run_size is class-dependent scaffold state
```

## MediumRun64KTwoSlotAB-L1

Status: candidate implemented; medium gate passed; default promotion pending clean small rerun.

```text
bench_results/20260624T123643Z_medium_64k_two_slot_ab/README.md
64K class candidate: 128KiB run / 2 slots
release medium r50 short ratio: 1.192
debug qpush: 33259 -> 23000
debug collect_slots_per_run: 1.203 -> 1.739
short phase create: 6368 -> 3979
short phase global_skip_foreign: 20269338 -> 7912248
zero gates clean in short probes
```

## MediumRun64KTwoSlotPairedGate-L1

Status: recorded; HOLD as default.

```text
bench_results/20260624T123932Z_medium_64k2_paired_gate/README.md
```

Result:

```text
medium r50 R10 x2:
  batch1 ratio 1.175
  batch2 ratio 1.172
  PASS

small local0 R10 x2:
  batch1 ratio 0.938
  batch2 ratio 1.037
  inconclusive

small interleaved remote90 R10 x2:
  batch1 ratio 0.979
  batch2 ratio 0.744
  FAIL as paired gate

reverse-order sanity:
  small interleaved ratio 0.986
```

Decision:

```text
q64-run64k2 remains evidence candidate
do not default yet
order-rotated small rerun is required before promotion
```

## MediumRun64KTwoSlotOrderRotatedGate-L1

Status: recorded; q64-run64k2 HOLD as default.

```text
bench_results/20260624T124502Z_medium_64k2_order_rotated_gate/README.md
small_local0 ratios: 1.070 / 0.873
small_interleaved_remote90 ratios: 0.884 / 0.951
```

Decision:

```text
keep q64-run64k as default
proceed to MediumDetachedRunClassIndex-L1 on current default geometry
```

## MediumDetachedRunClassIndex-L1

Status: recorded.

```text
bench_results/20260624T124747Z_medium_detached_class_index/README.md
debug medium r50: global_skip_foreign=0 free_steps=0
short debug phase r90: global_skip_foreign=0 free_steps=175546277
release medium r50 median: 7.531M ops/s
small interleaved quick median: 53.156M ops/s
```

Decision:

```text
detached class index removes foreign-attached global scan
remaining phase lookup cost is direct-directory overflow/fallback
next box is MediumChunkArenaQuantum-L1
```

## MediumQuantumDirectoryCap-L1

Status: recorded.

```text
bench_results/20260624T125025Z_medium_quantum_directory_cap/README.md
directory cap 4096 -> 65536
short phase r90: global_skip_foreign=0 free_steps=0
release medium r50 median: 7.590M ops/s
small interleaved quick median: 53.244M ops/s
```

Decision:

```text
directory-capacity fallback is closed
full ChunkArena carving is still open
next box is MediumChunkArenaCarve-L1 or Upper48K size-policy shadow
```

## MediumChunkArenaCarve-L1

Status: evidence-only; HOLD as default.

```text
bench_results/20260624T125508Z_medium_chunk_carve_candidate/README.md
candidate build: H8_MEDIUM_CHUNK_CARVE
release medium r50 ratio: 0.984
chunk phase r90 short: free_steps=0 remote_ms=4.111
```

Decision:

```text
default remains per-run mmap
directory fallback is already closed
next lane is MediumUpper48KSizePolicyShadow-L1
```

## MediumUpper48KSizePolicyPairedGate-L1

Status: recorded; HOLD as default.

```text
bench_results/20260624T_medium_upper48_paired_gate/README.md
candidate build macro: H8_MEDIUM_UPPER48_CLASS
candidate geometry id: q64-upper48
resident budget remains fixed to four-class default cap
```

```text
medium r50 paired:
  batch1 ratio 1.069
  batch2 ratio 1.020

small local0 R5:
  ratio 1.076

small interleaved remote90 R5:
  ratio 0.889

phase r90 R5:
  throughput ratio 1.329
  peak RSS ratio 0.993
```

Decision:

```text
functional and smoke-clean
medium r50 signal is positive
phase peak RSS improvement is small in this sample
small interleaved frozen quick gate fails
current default remains 8K / 16K / 32K / 64K
```

## Safety Gates

Medium runtime hard gates:

```text
medium_invalid_owned_platform_fallback = 0
medium_remote_publish_lost = 0
medium_remote_claim_accepted_twice = 0
medium_collect_duplicate = 0
medium_postclaim_unaccounted = 0
medium_empty_with_pending = 0
medium_decommit_while_pending = 0
medium_owner_dead_queue_push = 0
medium_owner_detach_nonquiescent = 0
medium_owner_exit_pending_nonzero = 0
medium_active_alloc_owner_mismatch = 0
medium_owner_list_owner_mismatch = 0
```

Quiescent run gates:

```text
pending_bits == 0
qstate == IDLE
run is not pending-queued
free_mask & allocated_mask == 0
free_mask | allocated_mask == valid_slot_mask
popcount(allocated_mask) == ALLOCATED slot_state count
popcount(free_mask) == FREE slot_state count
resident charge accounting matches payload_state
```

Small frozen gates stay active:

```text
INVALID platform fallback = 0
duplicate claim = 0
quiescent pending bitmap = 0
qstate/mask quiescent = clean
timeout / abort = 0
```

## Documentation

```text
architecture:
  docs/HZ8_ARCHITECTURE.md
  docs/HZ8_MEDIUM_RUN_V1.md
  docs/HZ8_V1_LANES.md

contracts:
  docs/HZ8_OWNERSHIP_CONTRACT.md
  docs/HZ8_OWNER_LIFECYCLE.md
  docs/HZ8_REMOTE_COLLECT.md
  docs/HZ8_NO_GO_LEDGER.md

bench gates:
  docs/HZ8_BENCH_GATE.md
  docs/HZ8_SMALL_V0_RC1.md
```
