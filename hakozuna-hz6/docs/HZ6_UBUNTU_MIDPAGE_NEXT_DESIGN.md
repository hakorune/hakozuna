# HZ6 Ubuntu MidPage Next Design

This note defines the next Ubuntu `LD_PRELOAD` MidPage work after descriptor-out,
free-cache audit, and transfer-first skip experiments.

## Problem

The current selected preload lane is balanced and safety-clean, but the
4096..16384 target still pays hot-path costs that are mostly local reuse work:

```text
selected default:
  MidPage descriptor-out is selected
  route fallback after descriptor-out is already zero on the target diagnostic
  MidPage active-map free cache failures are zero
  source-run-slot route registration is too small to chase first
```

The strongest recent performance witness is:

```text
HZ6_MIDPAGE_DIRECT_LOCAL_SKIP_TRANSFER_FIRST_L1=1
  4096..16384 repeat-7: 34.804M -> 38.944M
```

But the same code-shape regressed non-MidPage guard rows:

```text
16..4096:    41.947M -> 40.394M
1024..4096: 40.552M -> 39.062M
```

So the next design is not "turn transfer-skip on". The next design is:

```text
recover the MidPage target win while isolating Toy/non-MidPage code shape.
```

## Design Constraints

Keep:

```text
HZ6_MIDPAGE_ALLOC_DESCRIPTOR_OUT_L1=1
HZ6_MIDPAGE_ACTIVE_FREE_MAP_L2=1
HZ6_MIDPAGE_ACTIVE_FREE_MAP_UNALIGNED_L2=1
HZ6_MIDPAGE_ACTIVE_FREE_MAP_CAPACITY=16384
HZ6_MIDPAGE_ACTIVE_FREE_MAP_PROBE_LIMIT=4
```

Do not chase first:

```text
route fallback
deeper MidPage active-map probing
source-run-slot route registration
whole-helper free-cache replacement
broad malloc code-shape changes
default transfer-skip promotion
```

Acceptance guard:

```text
4096..16384 improves materially
16..256 does not regress beyond noise
16..4096 does not regress beyond noise
1024..4096 does not regress beyond noise
R1 smoke remains clean
route_invalid=0
route_miss=0 for owned frees
alloc_fail=0
route_register_fail=0
```

## Lane D: MidPagePayloadTrimAudit-L1

Goal:

```text
preserve the new 4096..16384 speed/RSS lead while reducing 32K source payload
pressure after static table trim.
```

Rationale:

```text
Static table trim removed about 20 MiB of fixed RSS from selected Ubuntu
preload. At this checkpoint, the remaining 4096..16384 pressure was mostly
MidPage 32K source-run payload and source-block count. HZ6 beat tcmalloc on RSS
and ops-per-MiB on 4096..16384, but still trailed tcmalloc on speed:

  hz6      41.264M /  94.38 MiB
  tcmalloc 44.812M / 103.38 MiB
```

Current follow-up:

```text
HZ6_MIDPAGE_32K_RUN_BYTES=2097152 is now selected. It keeps the 4096..16384
tcmalloc lead in the full cross refresh and cuts source_alloc in the stats
confirmation. The next design pass should re-audit remaining MidPage supply /
frontcache pressure after run2048 rather than reopening transfer-skip shape.
```

Post-run2048 follow-up:

```text
32K fine ladder:
  raw: private/raw-results/linux/hz6_midpage_payload_trim_ab_20260615_113052
  2M remains the local peak. 2304K/2560K are weaker, and 3072K/4096K are
  clear no-go controls for the balanced preload target.

MidPage 8K->32K borrow:
  flag: HZ6_MIDPAGE_8K_BORROW_32K_ON_MISS_L1=1
  production raw: private/raw-results/linux/hz6_midpage_supply_map_ab_20260615_113433
  stats raw: private/raw-results/linux/hz6_midpage_supply_map_ab_20260615_113452
  broad borrow_larger is guard-negative.
  narrow mid8_borrow32 is guard-safe but target-flat/weak and shows
  borrow_success=0 on the selected 4096..16384 stats row.

Decision:
  keep run2048 selected.
  keep mid8_borrow32 as default-off control/no-go evidence.
  next work should not chase broader frontcache borrow without a candidate
  dry-run that shows real target hits and low guard false positives.
```

Current implementation order:

```text
1. Stop run-size/borrow promotion attempts:
   - run2048 remains selected
   - broad borrow_larger is guard-negative
   - mid8_borrow32 is default-off control/no-go evidence

2. Keep mask-index and register fast-slot selected:
   - selected capacity is 16384, a power of two
   - mask-index removes modulo wrapping
   - register fast-slot removes the bounded loop for empty/same-pointer
     base-slot registrations

3. Keep free fast-slot controls off:
   - plain free fast-slot is target-flat and guard-weak in repeat-15
   - current-bias-gated free fast-slot adds branch cost and is weaker

4. Next work should be either:
   - frontcache storage/RSS design
   - a different active-map free-hit shape that avoids another per-attempt
     branch
```

Mask-index result:

```text
flag: HZ6_MIDPAGE_ACTIVE_MAP_MASK_INDEX_L1=1
production repeat-15:
  raw: private/raw-results/linux/hz6_midpage_supply_map_ab_20260615_131659
  4096..16384 49.443M -> 50.231M
  16..4096    41.770M -> 41.752M
  1024..4096  40.007M -> 40.019M
  16..256     57.755M -> 57.523M
stats repeat-3:
  raw: private/raw-results/linux/hz6_midpage_supply_map_ab_20260615_131717
  fail counters 0
post-promotion selected:
  raw: private/raw-results/linux/hz6_ubuntu_selected_balance_20260615_131844
  4096..16384 49.639M / 94.38 MiB
decision:
  promote mask-index for Ubuntu preload.
```

Register fast-slot result:

```text
flag: HZ6_MIDPAGE_ACTIVE_MAP_REGISTER_FAST_SLOT_L1=1
diagnostic probe audit:
  raw: private/raw-results/linux/hz6_midpage_supply_map_ab_20260615_145056
  4096..16384 selected:
    register avg_probe=1.15, base_slot=88.3%
    free hit avg_probe=1.15, base_slot=88.3%
production repeat-15:
  raw: private/raw-results/linux/hz6_midpage_supply_map_ab_20260615_145147
  4096..16384 selected 48.584M -> register_fast 50.160M
  16..4096    selected 41.344M -> register_fast 41.409M
  1024..4096  selected 39.847M -> register_fast 39.646M
  16..256     selected 56.980M -> register_fast 56.787M
stats repeat-3:
  raw: private/raw-results/linux/hz6_midpage_supply_map_ab_20260615_145220
  fail counters 0
post-promotion selected:
  raw: private/raw-results/linux/hz6_ubuntu_selected_balance_20260615_145322
  4096..16384 50.574M / 94.38 MiB
full cross:
  raw: private/raw-results/linux/hz6_ubuntu_selected_balance_20260615_145328
  4096..16384 hz6 48.961M / 94.50 MiB
  4096..16384 tcmalloc 43.192M / 106.62 MiB
decision:
  promote register fast-slot.
  keep free fast-slot as a control because both-fast is target-positive but
  guard-weaker.
```

Free fast-slot follow-up:

```text
flags:
  HZ6_MIDPAGE_ACTIVE_MAP_FREE_FAST_SLOT_L1=1
  HZ6_MIDPAGE_ACTIVE_MAP_FREE_FAST_SLOT_CURRENT_BIAS_L1=1
production repeat-15:
  raw: private/raw-results/linux/hz6_midpage_supply_map_ab_20260615_153012
  4096..16384 selected 50.375M -> free_fast 50.408M
  1024..4096  selected 39.763M -> free_fast 39.269M
  16..256     selected 57.028M -> free_fast 56.508M
current-bias gated repeat-7:
  raw: private/raw-results/linux/hz6_midpage_supply_map_ab_20260615_152942
  4096..16384 selected 50.566M -> free_fast_bias 50.081M
  16..256     selected 57.027M -> free_fast_bias 55.097M
decision:
  keep free fast-slot controls off.
```

Audit variants:

```text
selected:
  HZ6_MIDPAGE_32K_RUN_BYTES=262144

payload trim controls:
  HZ6_MIDPAGE_32K_RUN_BYTES=229376  # 7 slots
  HZ6_MIDPAGE_32K_RUN_BYTES=196608  # 6 slots
  HZ6_MIDPAGE_32K_RUN_BYTES=131072  # 4 slots, likely source-block stress
```

Acceptance:

```text
4096..16384 speed does not regress beyond noise
4096..16384 RSS improves
16..4096 and 1024..4096 do not regress materially
route_register_fail=0
source_block_exhausted=0
alloc_fail=0
malloc_real_fallback=0
```

Expected no-go signal:

```text
smaller 32K runs can increase source allocation count and route/source-block
pressure. If source_block_exhausted or alloc fallback appears, do not tune
around it with larger source tables first; that gives back the static table RSS
win.
```

Result:

```text
payload-trim controls below selected 256K were no-go:
  224K / 192K / 128K kept RSS essentially flat, increased source_alloc, and
  slowed 4096..16384.

upper run ladder showed the useful direction:
  320K / 384K / 448K / 512K reduce source_alloc.
  512K was strongest in the repeat-3 ladder.
```

Stats-off repeat-7 confirmation:

```text
selected 256K -> run512:
  16..256:      59.531M / 30.38 MiB -> 60.730M / 30.50 MiB
  16..4096:     41.811M / 79.88 MiB -> 43.235M / 79.88 MiB
  1024..4096:   40.856M / 91.12 MiB -> 41.568M / 91.00 MiB
  4096..16384:  42.176M / 94.38 MiB -> 45.298M / 94.50 MiB
```

Historical decision before current-bias:

```text
promote HZ6_MIDPAGE_32K_RUN_BYTES=524288.
Keep 262144 as the direct control.
Do not promote smaller payload-trim run sizes.
Superseded follow-up: HZ6_MIDPAGE_32K_RUN_BYTES=786432 was selected for the
Ubuntu preload bundle, then superseded by 1572864 after the run1536 promotion.
Keep 786432 and 524288 as direct controls.
```

## Lane E: MidPageSupplyMapResume-L1

Goal:

```text
after run768, decide whether the next balanced default should come from 8K
source-run widening or remaining active-map fallback removal.
```

Diagnostic read:

```text
selected diagnostic, 500K:

4096..16384:
  free_route_lookup_after_maps ~= 2.2K for 1M frees
  midpage_source_alloc=649
  midpage_8k_alloc_call=180
  midpage_32k_alloc_call=469
  midpage_8k_frontcache_pop_empty=362
  midpage_32k_frontcache_pop_empty=938
```

Decision:

```text
do not promote 8K source-run widening yet.

HZ6_MIDPAGE_RUN_BYTES=524288:
  reduces 4096..16384 source_alloc 653 -> 565
  improves 1024..4096 in repeat-7
  but leaves 4096..16384 speed essentially flat/slightly weak

Current selected after current-bias:
  HZ6_MIDPAGE_RUN_BYTES=786432
```

Active-map cap/probe read:

```text
cap32K/probe4 and cap64K/probe4 remove most remaining route-after-map fallback,
but speed and RSS regress. The remaining fallback count is already too small to
pay for a larger hot active map.

Keep selected:
  HZ6_MIDPAGE_ACTIVE_FREE_MAP_CAPACITY=16384
  HZ6_MIDPAGE_ACTIVE_FREE_MAP_PROBE_LIMIT=4
```

Runner:

```bash
./hakozuna-hz6/linux/run_hz6_midpage_supply_map_ab.sh
```

## Lane F: MidPageLowWaterRefill-L1

Goal:

```text
reduce MidPage frontcache empty pops without changing the normal reuse-hit
path or broadening active-map/source-run structures.
```

Behavior control:

```text
HZ6_MIDPAGE_LOW_WATER_REFILL_L1=1
HZ6_MIDPAGE_8K_LOW_WATER_REFILL=128
HZ6_MIDPAGE_32K_LOW_WATER_REFILL=64
```

Shape:

```text
Only after MidPage alloc misses the cache, prefill_run() succeeds, and the
post-prefill cache pop returns an object:
  if class frontcache count <= low-water mark:
    prefill one more run

This keeps the common cache-hit path unchanged. It intentionally spends extra
source/prefill work at miss boundaries to avoid near-future empty pops.
```

Acceptance:

```text
4096..16384 speed improves or remains flat with lower empty-pop pressure
16..4096 and 1024..4096 do not regress materially
RSS does not rise materially
route_invalid=0
route_miss=0 for owned frees
alloc_fail=0
route_register_fail=0
```

No-go signal:

```text
If source_alloc/RSS rises or target speed remains flat, close as no-go. The
previous run-size and active-map controls already showed that simply adding
more MidPage supply can fail to translate into throughput.
```

Result:

```text
implemented as default-off control.

HZ6_MIDPAGE_LOW_WATER_REFILL_L1=1:
  default thresholds: 8K=128, 32K=64
  strong thresholds:  8K=256, 32K=128

Stats-on repeat-7 made the strong lane look plausible, but stats-off repeat-9
closed it as no-go:
  16..256      selected 58.383M -> strong 57.162M
  16..4096     selected 42.123M -> strong 41.786M
  1024..4096   selected 40.448M -> strong 39.934M
  4096..16384  selected 44.597M -> strong 44.458M

Decision:
  keep HZ6_MIDPAGE_LOW_WATER_REFILL_L1=0 in selected default.
```

## Lane A: TransferProbeAudit-L1

Goal:

```text
prove how much transfer-first work is empty probe overhead by front/class.
```

Add diagnostic-only counters:

```text
midpage_direct_transfer_probe_attempt
midpage_direct_transfer_probe_hit
midpage_direct_transfer_probe_empty
midpage_8k_direct_transfer_probe_attempt
midpage_32k_direct_transfer_probe_attempt
```

Read:

```text
If attempts are high and hits are zero, transfer-skip remains a valid target
witness. If hits are non-zero in broader rows, skip must stay target-only.
```

No behavior change in this lane.

First read:

```text
selected diagnostic, 500K:

4096..16384:
  midpage_direct_transfer_probe_attempt=333720
  midpage_direct_transfer_probe_hit=0
  midpage_direct_transfer_probe_empty=333720
  midpage_8k_direct_transfer_probe_attempt=333720
  midpage_32k_direct_transfer_probe_attempt=0

16..4096:
  midpage_direct_transfer_probe_attempt=63
  midpage_direct_transfer_probe_hit=0
  midpage_direct_transfer_probe_empty=63
```

Read:

```text
The target row pays a large number of empty transfer probes on 8K direct-local
reuse. The broad 16..4096 guard barely touches this path, so its earlier
regression is most likely code-shape/layout sensitivity rather than direct
semantic pressure.
```

## Lane B: MidPage Target DSO Control

Goal:

```text
separate target-specialized MidPage transfer-skip from the selected balanced
preload DSO.
```

Build output:

```text
hakozuna-hz6/out/linux/hz6_preload_midpage_target/libhakozuna_hz6_preload.so
```

Build:

```bash
./hakozuna-hz6/linux/build_hz6_preload_midpage_target.sh
```

Flags:

```text
HZ6_PRELOAD_MIDPAGE_MALLOC_SKIP_TRANSFER_L1=1
```

Status:

```text
control / target-specialized
not selected default
```

Reason:

```text
The target witness is too large to discard, but guard regressions block
promotion. Keeping a named DSO lets us compare it intentionally without
polluting the selected lane.
```

Boundary repeat-7:

```text
preload-boundary target DSO:
  4096..16384: 34.712M -> 40.501M  (+16.68%)
  16..256:     57.596M -> 55.994M  (-2.78%)
  16..4096:    42.359M -> 41.369M  (-2.34%)
  1024..4096:  40.618M -> 39.912M  (-1.74%)
```

Read:

```text
The separate preload boundary recovers more target throughput than the
hz6_malloc() noinline/unlikely helper shapes and keeps RSS flat. It still fails
selected-default guards, so it remains a target-specialized DSO.
```

Outer-guard noinline repeat-15:

```text
preload-boundary noinline helper, called only under unlikely MidPage size guard:
  4096..16384: 33.879M -> 40.271M  (+18.87%)
  16..256:     55.884M -> 56.125M  (+0.43%)
  16..4096:    40.930M -> 40.929M  (-0.00%)
  1024..4096:  39.486M -> 39.731M  (+0.62%)
```

Read:

```text
The guard regression was code shape, not semantic MidPage pressure. The
outer-guard noinline shape avoids the small-path helper call, keeps the MidPage
body out-of-line, and passes the repeat guard. Promote this shape to the
selected preload bundle.
```

Selected-vs-control confirmation repeat-15:

```text
selected default vs boundary-off control:
  4096..16384: 33.735M -> 40.056M  (+18.74%)
  16..256:     55.957M -> 56.539M  (+1.04%)
  16..4096:    40.969M -> 41.339M  (+0.90%)
  1024..4096:  39.059M -> 39.953M  (+2.29%)
```

Confirmation lane:

```bash
./hakozuna-hz6/linux/build_hz6_preload.sh
./hakozuna-hz6/linux/build_hz6_preload_midpage_boundary_control.sh
./hakozuna-hz6/linux/run_hz6_preload_midpage_boundary_ab.sh --runs 15 --skip-build
```

## Lane C: Guard-Isolated Behavior Candidate

Goal:

```text
recover the target win without changing Toy/small hot code layout.
```

Allowed implementation shapes:

```text
1. out-of-line MidPage-only malloc helper called after the existing Toy fast
   classification and after generic front selection confirms HZ6_FRONT_MIDPAGE

2. MidPage-only direct reuse helper compiled noinline so the generic Toy path
   does not inherit the transfer-skip body shape

3. preload target DSO only, if selected default cannot isolate the guard cost
```

Disallowed implementation shapes:

```text
1. extra broad branch in the preload malloc wrapper
2. broad hz6_malloc() front classification rewrite
3. replacing generic free-cache helper on the selected path
4. static inline expansion of large MidPage helper bodies in hz6_malloc()
```

First attempts:

```text
noinline MidPage-only helper:
  4096..16384: 34.988M -> 39.397M
  16..256:     57.677M -> 56.022M
  16..4096:    42.410M -> 41.727M
  1024..4096:  40.993M -> 40.479M

noinline + unlikely MidPage branch:
  4096..16384: 34.698M -> 39.594M
  16..256:     58.059M -> 55.675M
  16..4096:    42.542M -> 41.452M
  1024..4096:  40.878M -> 39.858M
```

Read:

```text
Guard isolation by noinline/branch layout is not sufficient. The target win is
real, but every selected-default candidate so far leaks enough code-shape cost
into non-target rows to fail promotion. Keep this as target-specialized DSO /
control unless a separate preload dispatch boundary is designed.
```

## Promotion Rules

Promote to selected default only if repeat guards are clean:

```text
repeat-7 minimum:
  16..256
  16..4096
  1024..4096
  4096..16384

accept:
  target median >= selected +3%
  non-target median >= selected -0.5%
  no safety/stat failures
```

If target improves but any non-target guard fails:

```text
keep as target-specialized DSO/control
do not add to build_hz6_preload.sh selected flags
```

## Next Implementation Order

1. Mask-index is selected/default; do not reopen modulo-vs-mask unless
   active-map capacity stops being a power of two.
2. Do not broaden frontcache borrow without a dry-run that shows real target
   candidates and low guard false positives.
3. Prefer frontcache storage/RSS design or a fresh active-map success-path
   audit before changing active-map semantics.
