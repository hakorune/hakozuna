# Local Leaf Code Shape Sweep 20260623T144424Z

HEAD: f664e8f
Build: bench-release

Scope:

- observation only; no allocator behavior change
- inspected h8_malloc_inner, h8_free_inner, h8_remote_free_publish, h8_remote_free_publish_known

Saved asm:

- bench_results/20260623T144424Z_h8_malloc_inner.asm
- bench_results/20260623T144424Z_h8_free_inner.asm
- bench_results/20260623T144424Z_h8_remote_free_publish.asm
- bench_results/20260623T144424Z_h8_remote_free_publish_known.asm
- bench_results/20260623T144424Z_local_leaf_objdump.txt

Hard checks:

| Check | Result |
|---|---|
| malloc/free leaf has __tls_get_addr | no |
| target asm has div/idiv | no |
| h8_free_inner calls h8_span_from_ptr_checked | no |
| malloc active-hit calls h8_small_alloc_from_span | no symbol / no call |
| release remote path calls h8_remote_free_publish_locked | no symbol / no call |

Observed calls:

- h8_malloc_inner calls only slow/cold helpers: h8_thread_ctx_get_slow, h8_pressure_owner_collect, h8_find_active_span, h8_span_commit_for_class, h8_orphan_adopt_span.
- h8_free_inner local success path has no calls; remote path calls h8_remote_free_publish_known and transition retry calls h8_remote_free_publish.
- h8_remote_free_publish_known still calls lease/notify helpers: h8_span_publish_enter/exit, h8_owner_publish_enter/exit, h8_span_notify.

Remaining low-risk candidate:

- EvidenceKnobReleaseShape-L1: remove unsafe evidence knob loads from normal release hot path, or compile them only into explicit evidence builds.
- Rationale: h8_remote_free_publish_locked still loads h8_remote_pending_publish_elision_enabled, and h8_remote_free_publish_known still checks h8_remote_lease_elision_enabled. These are not correctness authorities for v0 release; they are unsafe evidence probes.

Hold:

- local_free_head / bump_index scalar cutover remains HOLD; current code shape already compiles relaxed atomics to ordinary moves.
- remote protocol changes remain HOLD; current interleaved remote lane clears the gate.
