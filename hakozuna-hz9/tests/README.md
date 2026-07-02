# HZ9 Tests

HZ9 has its own standalone test binaries.

```text
make -C hakozuna-hz9 smoke
./hakozuna-hz9/h8_smoke
```

Current standalone smoke set:

```text
make -C hakozuna-hz9 smoke
  inherited allocator boundary smoke

make -C hakozuna-hz9 preload-smoke
  local preload library sanity check

make -C hakozuna-hz9 smoke-hz9slabroute
  HZ9 slab route VALID / INVALID / MISS classification

make -C hakozuna-hz9 smoke-hz9slabpage
  HZ9 slab page local, remote, owner-exit, and final-free behavior
```

Active-run magazine evidence tests are still available as historical L0
coverage:

```text
make -C hakozuna-hz9 smoke-hz9mediumlocalmag
./hakozuna-hz9/h8_smoke_hz9mediumlocalmag
```

Covered by the magazine smoke:

```text
LOCAL_MAG double free rejection
remote publish rejection for LOCAL_MAG
owner-exit magazine flush
owner-exit empty-magazine hard drain
48K / 64K active-run magazine push/pop
state_mismatch == 0
```

Must remain covered by inherited boundary gates:

```text
invalid-pointer fail-closed behavior
remote duplicate-claim behavior
owner-exit hard drain
cache residue after owner exit
```
