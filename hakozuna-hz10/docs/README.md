# HZ10 Docs

HZ10 is a new-substrate research folder. It starts from design documents and
small implementation boxes rather than a full copy of HZ9.

```text
HZ10_LOCAL_PAGE_SUBSTRATE_TARGET.md:
  target performance band, architecture, safety split, RSS contract, and
  implementation boxes

HZ10_NO_GO_LEDGER.md:
  closed negative decisions and why not to retry them without new evidence

HZ10_DEBUG_NOTES.md:
  rare bug records, race fixes, and future load-bearing guard notes

HZ10_REMOTE_PUBLISH_BATCH_CONTRACT_L0.md:
  contract and allowed shapes for future remote-free publish batching work

HZ10_SPEED_ATTACK_PLAN_L0.md:
  post-RSS throughput attack plan, measurement order, candidate designs, and
  GO/NO-GO gates for the next speed boxes

HZ10_FRONT_CACHE_DESIGN_L0.md:
  review design for the opt-in per-thread per-class object front cache that
  attacks the slot_count 1-2 local-path gap without changing remote publish
  or retired/ready contracts

HZ10_PRELOAD_SHIM_DESIGN_L0.md:
  review design for the LD_PRELOAD shim (libhz10.so): interposition
  surface semantics, metadata self-hosting to break malloc recursion,
  the honest v0 thread-exit policy, and the macro-bench lane that
  replaces the micro rows as headline evidence

HZ10_ENTRY_TRIM_DESIGN_L0.md:
  review design for the per-op entry trim (route inline fast path,
  division-free interior check, size-class inline, record-carried owner
  token/class) that attacks the remaining main_local0/small_local0 gap the
  front cache measurement isolated; fail-closed route validation preserved
  via a differential route smoke
```
