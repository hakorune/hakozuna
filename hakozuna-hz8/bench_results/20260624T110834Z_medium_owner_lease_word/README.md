# MediumRunOwnerLeaseWord-L1 A/B

Date: 2026-06-24 UTC

Baseline:

```text
3cc922e4 Shadow medium owner lease word
```

Candidate tested:

```text
medium remote publish uses owner->medium_publish_ctl as release authority
debug build keeps the packed owner lease in parallel for decision comparison
owner exit closes medium_publish_ctl and waits medium refs to reach zero
release exit path uses atomic_fetch_sub on medium_publish_ctl
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
  median 3.272M ops/s
  steady median 3.309M ops/s

medium lease:
  decision_mismatch=0
  ref_underflow=0
  refs_at_exit=0
  enter_after_close=0
  reuse_with_refs=0

existing medium gates:
  invalid_owned=0
  active_owner_mismatch=0
  owner_list_mismatch=0
  route_authority_mismatch=0
  writer_overlap=0
  writer_foreign=0
  writer_token_change=0
  collect_wrong_owner=0
  detached_while_attached=0
  empty_with_pending=0
```

Release medium r50:

```text
command:
  ./h8_bench_release --runs 9 --threads 2 --iters 30000 \
    --min-size 4097 --max-size 65536 --remote-pct 50 --interleaved 1

candidate:
  median 7.429M ops/s
  p25 7.198M
  p75 7.881M
  steady median 7.751M

prior shadow run:
  median 7.479M ops/s
  steady median 7.751M
```

Small frozen quick:

```text
command:
  ./h8_bench_release --runs 3 --threads 16 --iters 30000 \
    --min-size 16 --max-size 4096 --remote-pct 90 --interleaved 1

throughput:
  median 50.124M ops/s
  steady median 54.687M
```

Decision:

```text
HOLD / do not default.

Safety gates are clean, but the dedicated medium owner lease word does not
clear the promotion condition. The measured release median is effectively flat
to slightly below the shadow baseline, despite the fetch_sub exit path.

Keep the debug shadow proof, but leave existing packed owner control as the
medium remote publish authority for now.
```
