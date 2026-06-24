# MediumRunOwnerLocalSingleWriterShadow-L2

Date: 2026-06-24

Base:

```text
f208fff1 Close medium post-claim rollback window
```

Change:

```text
debug-only writer guard added around medium run mask mutations
guard enters before the current run mutex for active/owner-local paths
actual allocator behavior unchanged
release build keeps debug writer fields and counters compiled out
```

Writer classes:

```text
OWNER_LOCAL_ALLOC
OWNER_LOCAL_FREE
OWNER_COLLECT
DETACHED_DIRECT_FREE
GLOBAL_ATTACH
OWNER_DETACH
```

Verification:

```text
make bench bench-release smoke safety-stress: pass
./h8_smoke: pass
./h8_safety_stress: pass
git diff --check: pass
```

Debug medium local:

```text
command:
  ./h8_bench --runs 3 --threads 2 --iters 30000 \
    --min-size 4097 --max-size 65536 --remote-pct 0 --interleaved 0

writer_overlap:
  0

writer_foreign:
  0

writer_token_change:
  0

collect_wrong_owner:
  0

detached_while_attached:
  0

lock_elide_alloc:
  179976

lock_elide_free:
  180000
```

Debug medium r50:

```text
command:
  ./h8_bench --runs 3 --threads 2 --iters 30000 \
    --min-size 4097 --max-size 65536 --remote-pct 50 --interleaved 1

writer_overlap:
  0

writer_foreign:
  0

writer_token_change:
  0

collect_wrong_owner:
  0

detached_while_attached:
  0

lock_elide_alloc:
  230632

lock_elide_free:
  90130

remote_pub:
  89870

remote_run_lock_ms:
  0.000
```

Interpretation:

```text
owner-local medium alloc/free have material lock-elision opportunities
no debug evidence of overlapping attached mask writers in these probes
owner collect did not race as a wrong-owner writer
OWNER_DETACH token changes are intentional and excluded from token-change gate
next behavior box can be MediumRunActiveOwnerLockElision-L1
```
