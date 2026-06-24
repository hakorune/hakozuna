# MediumRunCollectWordCommitRearm-L1

Date: 2026-06-24

Base:

```text
c5b4fb1f Condense medium task log
```

Change:

```text
medium collector commits one pending-word snapshot as a unit
slot_state FREE is published before pending clear
allocated_mask is cleared once per accepted snapshot
pending_bits is cleared once per snapshot
free_mask is set once per accepted snapshot
collector rechecks remaining pending bits before finish
collector self-arms DRAINING_DIRTY when remaining bits exist
collector finish rechecks pending and requeues if needed
```

Correctness target:

```text
close the race where a producer claims bit B while collector drains bit A,
old_word != 0 suppresses producer notify, and collector finishes IDLE
without noticing B
```

Debug medium r50:

```text
throughput median:
  3.338M ops/s

remote_pub:
  89870

qstate_dirty:
  set=17
  self_set=114
  requeue=131

collect_finish_pending_rearm:
  189

empty_with_pending:
  0

remote_collect_slot:
  89870

remote_collect_ms:
  6.946

remote_qpush:
  73630

remote_qpush_ms:
  4.723

zero gates:
  invalid_owned=0
  active_owner_mismatch=0
  owner_list_mismatch=0
  route_authority_mismatch=0
  writer_overlap=0
  writer_foreign=0
  writer_token_change=0
  collect_wrong_owner=0
  detached_while_attached=0
```

Release medium r50:

```text
median:
  7.583M ops/s

steady median:
  7.867M ops/s
```

Small quick rerun:

```text
small interleaved remote90:
  median 48.794M ops/s
  steady median 53.429M ops/s
```

Validation:

```text
smoke pass
safety_stress pass
```

Interpretation:

```text
collector now owns and rearms medium pending responsibility explicitly
qstate_dirty set+self_set equals requeue in the debug row
pending clear RMW count is now run-snapshot shaped instead of slot-shaped
medium r50 improves materially
next lane is medium owner lease word shadow
```
