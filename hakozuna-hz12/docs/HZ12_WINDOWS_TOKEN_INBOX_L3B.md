# HZ12 Windows Token-Aware Inbox L3-B

## Purpose

L3-B connects the L3-A generation token to a dedicated bounded inbox lane. It
validates lifecycle-aware publication without adding any owner lookup to normal
malloc/free.

## Publish Contract

```text
ACTIVE token:
  accept publish

RETIRING token:
  accept bounded final-flush publish

DEAD token:
  reject and ownerless-free the batch

stale generation:
  reject and ownerless-free the batch
```

An empty, never-bound inbox may be drained as a no-op. A generation rejection
is recorded only when a mismatched generation would otherwise detach queued
objects.

The owner may transition `RETIRING -> DEAD` only after producers stop publishing
that token and its inbox has been drained. This is the L3-B quiescent handoff
rule; it avoids treating a standalone registry lookup as a complete race proof.

## Module Boundary

`hz12_token_inbox` is a diagnostic lifecycle bridge. It has per-slot bounded
mutex inboxes and uses `hz12_owner_registry` only at batch publication. It is
not the existing L1 inbox and is not connected to the public allocator path.

## Acceptance

```text
two producer threads publish active batches
two producer threads publish final RETIRING batches
owner drains before DEAD
DEAD batches use ownerless fallback
replacement token rebinds the empty slot inbox
old token batches become stale rejects
replacement batches drain successfully
no generation-crossed inbox contents
```

## No-Go

```text
DEAD or stale batch enters the replacement inbox
RETIRING -> DEAD occurs before final drain
registry/inbox lookup enters normal malloc/free
token generation is omitted from inbox binding
fallback drops a batch
```

## Next Gate

L3-C adds a thread checkpoint/epoch acknowledgement before a real owner can be
declared DEAD. The token inbox remains a diagnostic lane until then.
