# HZ12 Windows Reclaim Performance Gate P0

## Gate

HZ12 reclaim policy may not enter a production candidate until all of the
following hold against the ColdSpanOwner baseline in the same round-robin
session:

```text
safety:
  ledger/atomic mismatch == 0
  generation/address mismatch == 0
  real owner inbox empty at reclaim
  detach/decommit failure == 0

throughput:
  random_mixed small/medium/mixed >= baseline -3%
  balanced/wide_ws/larger_sizes >= baseline -3%
  hard no-go at any repeatable regression below -5%

RSS:
  wide working-set peak improves by at least 10% after reclaim policy
  a 4 MiB budget returns at least 3.2 MiB of observed RSS
  retained/depot/inbox state does not grow across epochs

policy:
  slow checkpoint only
  at most 64 spans / 4 MiB per checkpoint
  no per-operation atomic accounting
```

## P0 Build Contract

P0 links the owner-ledger core but leaves automatic reclaim disabled. Atomic
accounting and the diagnostic comparison module are not linked. The build
script writes the exact flags and SHA-256 hashes to
`out_win_broad_p0/hz12_reclaim_p0_manifest.txt`.

The ledger core and atomic comparator were split into separate translation
units before measurement. This prevents an unused diagnostic judge from
entering the production-shape binary.

## Results

Stable MT, initial R3:

| profile | baseline | ledger P0 | delta |
| --- | ---: | ---: | ---: |
| balanced | 62.132M | 65.606M | +5.6% |
| wide_ws | 36.105M | 33.608M | -6.9% |
| larger_sizes | 74.329M | 73.383M | -1.3% |

The wide row did not reproduce as a slowdown in the longer two-second R5:

| profile | baseline | ledger P0 | delta | RSS baseline | RSS P0 |
| --- | ---: | ---: | ---: | ---: | ---: |
| wide_ws | 34.442M | 35.753M | +3.8% | 88.29 MiB | 91.20 MiB |

Local random_mixed R5 is decisive:

| profile | baseline | ledger P0 | delta |
| --- | ---: | ---: | ---: |
| small | 99.188M | 83.007M | -16.3% |
| medium | 86.964M | 73.378M | -15.6% |
| mixed | 73.053M | 60.076M | -17.8% |

## Decision

P0 is **NO-GO** for production promotion. The fixed L6-A..L6-F lifecycle
remains valid safety/mechanism evidence, but its per-slot bitmap ledger is too
expensive for a production-shape local path.

Do not tune automatic reclaim policy yet. The next experiment must attribute
the local tax among:

1. second owner/side-table scan on flush;
2. per-slot bitmap transition;
3. acquire-side span lookup;
4. return-side ledger update.

The likely production direction is a compact owner-local per-span batch count
updated from ownership information already computed by flush routing. The
bitmap remains the diagnostic judge, not production state.
