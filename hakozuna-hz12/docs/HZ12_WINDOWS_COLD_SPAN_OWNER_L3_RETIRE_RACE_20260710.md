# HZ12 Windows ColdSpanOwner-L3 Retire Race (2026-07-10)

Status: GO as concurrent owner-lifetime evidence. Still opt-in.

The dedicated smoke races one allocating owner thread against four foreign
freeing threads, then creates a replacement owner to exercise slot reuse. Each
round allocates 8,192 objects across sizes 8..1024. The test repeats 20 times.

```text
rounds=20
attach_success=120
attach_reuse=116
attach_full=0
detach_success=120
stale_fallback=0
```

Publishing and detach serialize on the target owner inbox lock. A publisher
either completes under the matching active generation or observes an inactive
or different generation and uses the ownerless returned sink. FLS teardown
flushes the exiting thread cache before marking its owner slot inactive.

The smoke completed without crashes, owner-table exhaustion, leaked active
owners, or mismatched attach/detach counts. This closes the first concurrent
retirement gate. It does not prove automatic span reclaim and does not promote
ColdSpanOwner to the default profile.

Next: run a stable-duration MT broad gate. The legacy matrix rows are too short
for promotion decisions.
