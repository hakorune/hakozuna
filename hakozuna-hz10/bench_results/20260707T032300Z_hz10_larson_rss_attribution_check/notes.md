# HZ10 Larson RSS Attribution Check

Purpose: verify the fable5 bimodality report before opening
HZ10AdoptionBimodalDynamics-L0.

## Reproduction

Command shape:

```text
make -C hakozuna-hz10 preload
LD_PRELOAD=./hakozuna-hz10/libhz10.so \
  /mnt/workdisk/public_share/hakmem/larson_system 2 8 128 128 1 12345 4
```

External 50ms RSS sampling:

```text
clean hz10 runs: 599,936 - 604,800 KiB, 8/8 bad-mode
census hz10 runs: 600,320 - 606,592 KiB, 8/8 bad-mode
```

`HZ10_SHIM_CENSUS_SEC=1` did NOT reproduce the reported ~1.8MB good mode on
this checkout.

## Key correction

The 600MB RSS is not mostly live/orphan page payload.

At t=1s in an RSS-600MB run, pagemap census reported:

```text
registered pages: 320 pages / 20,971,520 bytes
orphan_unadopted: 6 pages / 393,216 bytes
adopted class 8: 275 pages / 18,022,400 bytes
```

So partial adoption is working: the orphan page payload is already dense and
small. The missing RSS is outside the live registered page census.

## VMA attribution

HZ10 smaps at plateau:

```text
total RSS: ~606,896 KiB
8192 KiB VMAs: 32,588 mappings, ~260,708 KiB RSS
1024 KiB VMAs:    313 mappings, ~317,356 KiB RSS
registered HZ10 page payload by census: ~20 MiB
```

glibc larson under the same workload:

```text
total RSS: ~273,444 KiB
8192 KiB VMAs: 32,742 mappings, ~261,940 KiB RSS
```

Read:

* the 8MiB VMA mass is the pthread stack-cache baseline also seen in glibc;
  it explains why glibc/tcmalloc sit around 270-280MB on a workload whose
  true live allocation demand is tiny.
* HZ10's extra ~317MB is the persistent owner-record slab path
  (`HZ10_OWNER_SLAB_BYTES=1MiB`). `sizeof(Hz10ThreadOwner)=9992`, so a
  high-churn larson run creates hundreds of 1MiB owner slabs. These records
  are intentionally never freed after HZ10PersistentOwnerRecord-L1 to avoid
  owner-token ABA.

## Verdict

Do not open a policy-only adoption dynamics fix yet. The next measured box is
owner-record footprint/retirement, not adopt-k.

Candidate direction: split the persistent owner identity token from the large
per-thread class-state arrays. Pages only need a stable owner identity after
thread exit; the full `Hz10ClassState[HZ10_CLASS_COUNT]` storage is a live
thread cache and should not become permanent 10KB/thread baggage.
