# MediumRunOwnerLeaseWordShadow-L1

Date: 2026-06-24 UTC

Base:

```text
6742ec65 Rearm medium collect finish
```

Change:

```text
debug-only owner-scoped medium_publish_ctl shadow
existing packed owner control remains authority
shadow enter decision compared against h8_owner_publish_enter
shadow refs closed and checked at owner exit
release hot path has no shadow call
```

Build:

```text
git diff --check
make bench bench-release smoke safety-stress
```

Functional:

```text
h8_smoke:
  arena=68719476736 committed=2228224 owners=68 local=68 remote=32

h8_safety_stress:
  owners=9 owner_exit=8 handoff=68 remote=8192 collect=0 duplicate_claim=1 invalid=7
```

Debug medium r50:

```text
command:
  ./h8_bench --runs 3 --threads 2 --iters 30000 \
    --min-size 4097 --max-size 65536 --remote-pct 50 --interleaved 1

throughput:
  median 3.219M ops/s
  steady median 3.257M ops/s

medium lease shadow:
  decision_mismatch=0
  ref_underflow=0
  refs_at_exit=0
  enter_after_close=0
  reuse_with_refs=0

medium remote:
  remote_pub=89870
  remote_lease_ms=14.009
  remote_lease_enter_ms=6.107
  remote_lease_exit_ms=7.902
  remote_qpush=72733
  remote_qpush_ms=4.729
  remote_collect_call=10016
  remote_collect_run=72733
  remote_collect_slot=89870
  remote_collect_ms=6.610
  collect_finish_pending_rearm=171
  empty_with_pending=0

existing zero gates:
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
command:
  ./h8_bench_release --runs 7 --threads 2 --iters 30000 \
    --min-size 4097 --max-size 65536 --remote-pct 50 --interleaved 1

throughput:
  median 7.479M ops/s
  p25 7.411M
  p75 7.588M
  steady median 7.751M
```

Small frozen quick:

```text
command:
  ./h8_bench_release --runs 3 --threads 16 --iters 30000 \
    --min-size 16 --max-size 4096 --remote-pct 90 --interleaved 1

throughput:
  median 50.412M ops/s
  steady median 55.011M
```

Decision:

```text
MediumRunOwnerLeaseWordShadow-L1 passes.

The dedicated medium owner lease word matches the existing owner control
admission decision under this stress shape, and owner exit observes zero
shadow refs. This is sufficient to proceed to MediumRunOwnerLeaseWord-L1 A/B,
where the shadow word can become the medium lease authority for medium remote
publish only.
```
