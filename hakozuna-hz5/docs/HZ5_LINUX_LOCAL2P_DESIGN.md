# HZ5 Linux Local2P Design

This file is the short current-status note for the Linux Local2P exact route.
The full design and measurement history is archived at:

```text
hakozuna-hz5/docs/archive/HZ5_LINUX_LOCAL2P_DESIGN_HISTORY_2026-05.md
```

## Scope

Local2P is Linux-only and targets the exact row:

```text
size == 65536
align == 8192
raw_bytes == 131072
```

It is not a Windows P43i/P45 change and should not be merged into ordinary
malloc front-end claims.

## Reporting Profiles

| Profile | Claim |
| --- | --- |
| `hz5-local2p-linkflags` | low-final-RSS exact local/mixed speed profile |
| `hz5-local2p-rssretain2048tls` | retained-cache RSS-throughput profile |
| `hz5-local2p-remotebatch` | producer/consumer remote-free profile |

Do not collapse these into one "HZ5 Local2P" result. They use the same exact
route with different cache/recycle policies and must be judged by different
workload families.

## Non-Goals

```text
4096:8192 and 8192:8192:
  stay on the legacy exact/P2 route unless a separate SmallA8192 profile is
  explicitly designed.

ordinary malloc:
  handled by SmallFront, MidPageFront, and LargeFront, not Local2P.

Windows exact overalign:
  stays under P43i/P45.
```

## Current Decision

Local2P is considered a stabilized appendix/specialized route. Current
development focus is ordinary Linux malloc, especially LargeFront 128K
remote-heavy behavior.
