# RemotePublishLockedInline-AB-L1

Date: 2026-06-23

## Scope

Inline `h8_remote_free_publish_locked` into `h8_remote_free_publish_known`.

This is a code-shape box.  It does not change:

```text
owner/span publish lease
slot_state validation
pending bitmap claim
post-claim slot_state revalidation
pending_word_mask publication
qstate notification
```

## Worker Evidence

The remote-side worker found that `h8_remote_free_publish_known` called the
locked publish body on every successful remote publish path.

The change only adds `static inline __attribute__((always_inline))` to the
locked body.

## Code Shape

After the change:

```text
h8_remote_free_publish_locked:
  no symbol
  no call from h8_remote_free_publish_known

h8_remote_free_publish_known:
  contains the pending claim / validation body inline
```

`h8_remote_free_publish_known` text grows because the regular, orphan, and
unsafe evidence branches each get an inline copy.  This is accepted only because
the short performance check stays clean.

## Checks

```text
make bench-release smoke safety-stress
./h8_smoke
./h8_safety_stress
```

Result: pass.

Focused checks:

```text
small_interleaved_remote90 RUNS=5:
  median 57.62M ops/s
  p25    57.02M ops/s
  min    44.42M ops/s
  steady median 59.72M ops/s

small_phase_remote90 RUNS=3:
  median 6.35M ops/s
  remote phase median 10.55 ms
```

## Decision

Keep the inline shape.

The remote protocol remains frozen.  Do not remove correctness-coupled pieces:

```text
post-claim slot_state revalidation
owner/span publish lease
qstate notify
pending bitmap claim
```
