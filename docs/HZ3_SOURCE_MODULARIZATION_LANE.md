# HZ3 Source Modularization Lane

This lane keeps HZ3 source cleanup separate from allocator performance lanes.
The goal is to reduce large implementation hubs without changing behavior.

## Status 2026-06-14

Recent completed splits:

```text
14fe1b5 Split HZ5 LowPage stats print helpers
0a26f0c Split HZ7 v2 v4 state helpers
53856b1 Split HZ3 large cache policy helpers
94ec8ec Split HZ3 large allocation paths
this pass Split HZ3 remote stash helpers
this pass Split HZ3 central helpers
this pass Split HZ3 inbox helpers
this pass Split HZ3 tcache helpers
```

Current HZ3 cleanup result:

```text
hakozuna/src/hz3_large_cache_policy.inc:
  5156 lines -> 7-line router plus focused helper includes

hakozuna/src/hz3_large.c:
  2622 lines -> 63-line router plus focused helper includes

hakozuna/src/hz3_tcache_remote_stash.inc:
  1960 lines -> 12-line router plus focused helper includes

hakozuna/src/hz3_central.c:
  2938 lines -> 31-line router plus focused helper includes

hakozuna/src/hz3_inbox.c:
  1685 lines -> 26-line router plus focused helper includes

hakozuna/src/hz3_tcache.c:
  1521 lines -> 80-line router plus focused helper includes
```

The split is intentionally source-shape only. It should not change lane
defaults, benchmark profiles, or allocator semantics.

## Active Queue

Use `linux/audit_large_source_files.sh` before choosing the next target.
Prioritize hot HZ3 core files before Windows-only benchmark files.

Current next candidates:

```text
P0:
  hakozuna/src/hz3_tcache_slowpath.inc

P1:
  hakozuna/src/hz3_arena.c
  hakozuna/include/config/hz3_config_scale_part8_modern.inc

P2:
  linux/hz5_build_*.sh helper scripts still above 1000 lines
```

Windows benchmark sources are allowed to remain large unless they block Linux
allocator work or duplicate reusable benchmark helpers.

## Split Rules

Do:

```text
Keep the original top-level file as a short ordered router.
Move contiguous logical sections into focused .inc helpers.
Preserve include order exactly unless a dependency cleanup is explicit.
Keep feature flag and lane macro names unchanged.
Run a full clean build after splitting .inc files.
```

Be careful with preprocessor conditionals:

```text
Do not split an open #if/#else/#endif across include file EOF.
If a conditional owns several sections, make a small router include that owns
the conditional and includes focused child files inside it.
```

Do not:

```text
Mix cleanup with performance tuning in the same commit.
Rename public lane flags during a source split.
Move broad diagnostic or policy logic into headers to reduce line count.
Commit generated build outputs or scratch benchmark result trees.
```

## Verification

Minimum checks for an HZ3 source split:

```text
linux/audit_large_source_files.sh
make -C hakozuna clean all_ldpreload_scale
git diff --check
git status --short
```

If a split touches HZ5/HZ6/HZ7 sources, run the matching build or smoke script
for that family as well.

## Commit Shape

Use one commit per coherent split:

```text
Split HZ3 <area> helpers
```

The commit should contain:

```text
router file change
new helper includes
small doc update only when the lane queue or rule changes
```
