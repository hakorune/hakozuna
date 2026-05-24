# HZ4 vs HZ5 Remote Path Audit

This note records the code-level differences found while comparing HZ4's
remote-heavy path against the current HZ5 Linux full-preload fronts.

## Question

HZ5 still trails HZ4 on remote-heavy `main`, `mid_only`, and `cross128` rows.
The next work should not add another broad knob until the concrete HZ4/HZ5 path
difference is named.

## HZ4 Mid / Remote Path

HZ4 mid is not equivalent to HZ5 MidFront-M1.

```text
HZ4 mid:
  size-classed 256-byte alignment
  one 4KiB page contains one class
  ptr -> page by masking with ~(4096 - 1)
  page owns owner_tag, remote_head, remote_count
  remote free pushes to that page's remote_head
  owner alloc drains that owned page when remote_count/tick says so
```

Important files in the reference tree:

```text
/mnt/workdisk/public_share/hakmem/hakozuna/hz4/src/hz4_mid_malloc.inc
/mnt/workdisk/public_share/hakmem/hakozuna/hz4/src/hz4_mid_free.inc
/mnt/workdisk/public_share/hakmem/hakozuna/hz4/src/hz4_mid_owner_remote.inc
/mnt/workdisk/public_share/hakmem/hakozuna/hz4/include/hz4_mid.h
```

The key free path is short:

```text
ptr -> page mask
check page magic and class
if owner self:
  push to owner local stack or page freelist
else:
  CAS push object to page->remote_head
```

The owner-remote path exists in the HZ4 source, but in the benchmark reference
configuration `HZ4_MID_OWNER_REMOTE_QUEUE_BOX` is off and
`HZ4_MID_FREE_BATCH_BOX` is on. Therefore the practical lesson is broader than
one owner-page mechanism:

```text
small remote:
  sender TLS rbuf, cap 128, flush threshold 96
  keyed owner/sc publish

mid:
  256-byte classes inside a 4KiB page
  free-batch / alloc-run cache amortizes locking

large:
  ptr-adjacent header validation and large span/cache return
  no MidFront-style owner inbox state machine
```

This matters because `mid_only=2049..32768` is not all HZ4 mid. It is HZ4
mid plus HZ4 large, while HZ5 currently routes the whole 2049..65536 band
through MidFront one-object spans.

## HZ5 Current Path

HZ5 full preload currently dispatches:

```text
free(ptr):
  SmallFront lookup/free
  MidFront lookup/free
  LargeFront lookup/free
  preload pointer table
  real free fallback
```

For MidFront pointers this means at least one SmallFront miss lookup before the
MidFront hit. MidFront then does:

```text
ptr -> page-map hash lookup
span magic/range/base check
owner token compare
state transition
owner/class sender batch or owner inbox publish
owner drain later transitions REMOTE_PENDING -> LOCAL_FREE
```

This preserves HZ5's descriptor ownership and fail-closed behavior, but it is
structurally heavier than HZ4's page-local owner path.

## Measured Gap

Recent perf spot checks showed the clearest gap on MidFront remote-heavy rows:

```text
mid_only_r90:
  HZ4:
    about 216.7 instructions/op
    about 44.4 branches/op

  HZ5 region:
    about 393.1 instructions/op
    about 87.6 branches/op
```

This points at dispatch/lookup/state-machine cost, not only remote batch
threshold tuning.

## Immediate Low-Risk Diagnostic

Added:

```text
--linux-hz5-general-midfirst
```

This keeps the `general-region-outbox` route set but changes preload `free()`
ownership dispatch to try MidFront before SmallFront:

```text
default:
  Small -> Mid -> Large

midfirst diagnostic:
  Mid -> Small -> Large
```

Safety boundary:

```text
ownership validation remains inside each front-end descriptor lookup
invalid HZ5-owned pointers still fail closed
no pointer-table performance path is added
```

Expected signal:

```text
mid_only_r90:
  should improve if the SmallFront miss lookup is significant

main/cross128:
  may regress if small traffic dominates or if the extra MidFront miss is costly
```

This is an attribution/cost isolation lane, not a default profile.

## Diagnostic Results

Short smoke, `threads=16`, `ws=400`, `remote=90`, repeat-3:

```text
private/raw-results/linux/midfirst_smoke_20260525_012438

main:
  region    25.02M
  midfirst  23.65M

mid_only:
  region    25.00M
  midfirst  15.97M

cross128:
  region    14.30M
  midfirst  11.56M
```

Decision:

```text
midfirst is no-go.
Trying MidFront first makes non-mid misses too expensive and does not isolate a
useful HZ4-like win.
```

Second diagnostic:

```text
--linux-hz5-general-midcache
```

This adds a two-entry thread-local `page_base -> span` cache before MidFront's
hash page-map probe. It still validates magic/base/end after the cache hit.

Short smoke, `threads=16`, `ws=400`, `remote=90`, repeat-3:

```text
private/raw-results/linux/midcache_smoke_20260525_012716

main:
  region    34.36M
  midcache  20.70M

mid_only:
  region    24.44M
  midcache  25.47M

cross128:
  region    17.59M
  midcache  11.16M
```

Decision:

```text
midcache is diagnostic-only.
It gives a small mid_only signal but regresses mixed rows badly. A tiny lookup
cache is not enough; the next useful HZ4-inspired candidate is a sender rbuf /
route-split design, not more preload dispatch reordering.
```

## Larger Design Candidates

If midfirst shows a useful signal, the cleaner follow-up is not to keep
reordering dispatch globally. It is to reduce front ownership lookup cost:

```text
1. Front hint / page-kind map:
   one cheap page-level discriminator before front-specific lookup

2. HZ5 MidPageFront:
   HZ4-like page-local owner/remote path for the lower mid range
   keep descriptor ownership, but avoid one-object span semantics where they
   are not buying enough

3. Mid/Large route split:
   verify whether 8K..64K ordinary malloc should stay in MidFront or move to a
   larger retained-span path closer to HZ4's large behavior

4. HZ4-style sender rbuf:
   replace one owner/class remote batch slot with a larger TLS ring of remote
   records, then group by owner/class at flush time
```

## MidFront Sender Rbuf Diagnostic

Added:

```text
--linux-hz5-general-midrbuf
--linux-midfront-remote-rbuf
--linux-midfront-remote-rbuf-cap N
--linux-midfront-remote-rbuf-threshold N
```

Design:

```text
remote free:
  validate descriptor/state as before
  append {owner, class, span} to sender TLS rbuf

flush:
  group entries by owner/class
  publish one list per group to the existing owner/class inbox
```

This keeps MidFront descriptor validation and fail-closed state transitions.
It only changes sender-side publication shape.

Short smoke showed possible cross-size signal:

```text
private/raw-results/linux/midrbuf_smoke_20260525_013236

cross128_r90:
  region   14.55M
  midrbuf  19.61M

large_only_r90:
  region    7.08M
  midrbuf  12.60M

main_r90:
  region   24.03M
  midrbuf  21.05M

mid_only_r90:
  region   23.29M
  midrbuf  16.46M
```

Threshold sweep:

```text
private/raw-results/linux/midrbuf_threshold_smoke_20260525_013354

rbuf16 was the least bad:
  main_r90      29.25M vs region 25.41M
  cross128_r90  15.69M vs region 15.34M
  large_only    9.51M vs region 5.99M
  mid_only     16.44M vs region 17.21M
```

Broad repeat-5 rejected it as a default:

```text
private/raw-results/linux/midrbuf16_broad_r5_20260525_013427

main_r90:
  region  21.24M
  rbuf16  18.53M

mid_only_r90:
  region  27.03M
  rbuf16  21.03M

cross128_r90:
  region  12.02M
  rbuf16  12.43M

large_only_r90:
  region   6.72M
  rbuf16   6.45M
```

Decision:

```text
midrbuf is no-go for the broad combined lane.
It can help selected cross-size samples, but delayed publication hurts
MidFront-dominant rows.
```

Updated next target:

```text
route split:
  HZ4 mid_only is not one broad mid front-end.
  HZ4 routes lower mid through 4KiB-page mid machinery and larger objects
  through large/header/cache paths.

HZ5 should test:
  MidFront <= 4096
  LargeFront lower classes for 8192/16384/32768/65536
```

Do not copy HZ4 wholesale. HZ5 should keep descriptor ownership, fail-closed
invalid frees, and Linux/Windows lane separation.
