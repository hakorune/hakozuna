# Post owner-split verification + fine x adoption interaction bug

Follow-up verification of commit 20448ec1 (persistent owner record split)
and the fine-class question it reopens.

## 1. Bimodality: CLOSED

Default shim larson_system (2 8 128 128 1 12345 4), 50ms ps-rss peaks:

```text
8/8 runs: 282.7 - 291.5MB  (was bimodal 1.8MB / ~600MB before the split)
```

The ~600MB bad mode was accumulated per-dead-thread Hz10ThreadOwner
state (which the census could never see -- it counts pages, and pages
were only ~20MB). The owner split fixed exactly that: larson RSS is now
stable competitor-range (glibc 272 / tcm 279 / mima 284 / hz10 ~286MB),
and steady-state census confirms adoption still works on the default
build post-split: t=1s adopted=283 pages, orphan_unadopted=3, total
19.2MB registered.

HZ10AdoptionBimodalDynamics-L0 is therefore RESOLVED-BY-FIX (the
attractor question died with the leak); SSOT updated.

## 2. libhz10_fine.so was stale-wired; fixed, then a REAL bug surfaced

The fine sibling built without the adoption flags (pre-adoption flag
set), so every fine-vs-default macro comparison since the default flip
was polluted (its larson 9.2GB was the old abandoned-page pathology).
Makefile now gives it the default's adoption flags + fine.

Re-measured with correct wiring:

```text
python_alloc (3 runs):    default wall 0.53-0.56 / rss ~117.0MB
                          fine    wall 0.50-0.52 / rss ~108.0MB  <- win
larson x4:                fine 9.0-9.2GB, throughput 0.94M ops/s <- BROKEN
larson census (fine, t=1s): orphan_unadopted=79,031 pages (4.94GB),
                          adopted=0, metadata_live=140,448 / free=0
                          (pages never destroyed, adoption NEVER fires)
larson census (default, t=1s): adopted=283, orphan_unadopted=3 -- fine
```

The adoption smoke (hz10_public_entry_orphan_adoption_smoke) PASSES when
built with the fine+adoption flag set, so the mechanism is not dead in
general -- it fails only under larson's shape with the fine table.
adopted=0 absolute (not slow) says the adopt path either never reaches a
matching registry class or rejects everything it pops.

Suspects for the bug hunt, in order:
  a. reject->repush loops burning the scan budget: pop rejects a page
     (class_id or owner-state check) and repushes to the SAME class list
     tail; with budget K and >=K rejectable pages at head, adoption
     starves globally. Instrument pop/reject/repush counters per reason.
  b. per-class registry vs allocation demand mismatch under 38 classes
     (publish class index vs page->class_id vs allocating class_id --
     all should agree, but count them, do not re-read code).
  c. owner-record state visibility on the popped page's old owner.
Deliverable first: one counter set (pops, adopts, reject_class,
reject_state, repushes) per class, printed in exit stats; run larson
fine vs default; the reason column IS the diagnosis.

## 3. Standing decision input once fixed

python row wants fine (wall AND -9MB RSS); larson is the only blocker
and it is a bug, not a trade-off. If the fix lands and fine larson
matches default (~286MB), the shim-default-fine decision reopens with
all rows green.

## 4. Codex follow-up: fine adoption failure did not reproduce after clean rebuild

Added per-class orphan-adoption exit counters:

```text
published / pop / adopted / reject_class / reject_state /
reject_no_capacity / repush / depth / max_depth
```

The counters are printed by `HZ10_SHIM_EXIT_STATS=1` as
`hz10_shim_orphan_adoption_stats ...` rows. They are diagnostic-only and do
not change adoption policy.

After rebuilding `libhz10_fine.so` from the corrected Makefile wiring
(default adoption flags + fine table), larson fine no longer shows
`adopted=0` on this checkout.

Direct 8-run probe:

```text
binary: /mnt/workdisk/public_share/hakmem/larson_system
args:   2 8 128 128 1 12345 4
env:    HZ10_SHIM_TOLERATE_FOREIGN=1
        HZ10_SHIM_EXIT_STATS=1
        HZ10_SHIM_EXIT_STATS_CLASSES=0
        LD_PRELOAD=libhz10_fine.so

run  max_rss_kb  throughput  published  pop     adopted  reject_no_capacity  depth
1    290816      2095609     294701     315446  294298   21148               403
2    287488      2095595     294701     315362  294342   21020               359
3    282356      2095611     294701     308945  294427   14518               274
4    283648      2095611     294701     312982  294409   18573               292
5    283520      2095604     294701     311826  294411   17415               290
6    281984      2095606     294701     314402  294437   19965               264
7    281344      2095546     294692     310129  294432   15697               260
8    286208      2095609     294701     312669  294368   18301               333
```

Verdict for this checkout: the reported fine-specific `adopted=0` failure is
not reproducible after a clean rebuild. The likely root cause was stale
artifact/build wiring, not a remaining class-registry bug. Keep the counters
for future triage because they made the diagnosis direct.

## 5. Default decision

A same-matrix RUNS=3 check then made fine a shim-default GO:

```text
python_alloc: hz10 0.930s / 116,756 KiB; hz10+fine 0.930s / 106,788 KiB
larson:       hz10 4.176s / 288,256 KiB; hz10+fine 4.173s / 281,856 KiB
redis:        both ~0.550s, client RSS within one sample quantum
```

`make preload` now builds `libhz10.so` with orphan + partial adoption + fine
classes. The coarse rollback lane is `libhz10_orphan_partial.so`.

Log: `bench_results/20260707T_fine_default_candidate_matrix/`.
