# HZ9 Differentiation

## Summary

HZ9 is a throughput-first experimental line, not another public allocator choice
for users today.

```text
public release line:
  HZ8

experimental throughput line:
  HZ9
```

## Difference From HZ3

```text
HZ3:
  local-throughput reference
  simple fast local path
  not the current fail-closed / RSS-balanced product line

HZ9:
  borrows the local-throughput goal
  keeps HZ8-style owned-pointer safety where possible
  keeps explicit remote-free authority
  keeps owner lifecycle recovery as a testable contract
```

HZ9 should not become an HZ3 clone with new names.

## Difference From HZ8

```text
HZ8:
  balanced low-RSS allocator
  public default
  conservative remote/lifecycle/RSS contract

HZ9:
  opt-in experimental line
  may accept a heavier cache/RSS contract
  focuses on local medium/main throughput
  should not contaminate HZ8 default hot paths
```

## Difference From tcmalloc / mimalloc

```text
tcmalloc:
  production throughput baseline
  strong thread-cache/front-end design

mimalloc:
  page-local and remote-free split reference

HZ9:
  studies similar local/remote split ideas
  keeps HZ-owned INVALID / owner lifecycle / remote claim contracts explicit
```

## Product Policy

Do not expose HZ3..HZ9 as a confusing menu.

```text
recommended user-facing allocator:
  HZ8

research documentation:
  HZ3..HZ7 as design references
  HZ9 as an experimental throughput lane
```

HZ9 can become a user-facing line only after it has:

```text
clear throughput advantage
explicit RSS contract
full safety zero gates
Linux and Windows story
cross-allocator matrix evidence
```
