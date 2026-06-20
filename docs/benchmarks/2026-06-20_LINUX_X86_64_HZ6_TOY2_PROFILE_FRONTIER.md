# Linux x86_64 HZ6 Toy2 Profile Frontier, 2026-06-20

Follow-up to `ToyClass2FrontMaintenanceGate-L1` and
`Cross128Toy2SplitProfile-L1`.

Raw output:

- `hakozuna-hz6/private/raw-results/linux/hz6_toy2_profile_frontier_r10_20260620_181753`

Command shape:

```sh
RUNS=10 \
PROFILES=selected,transfer_presence,small_class_shard,toy2_split \
ROWS=remote50,remote90,cross128_r90 \
./hakozuna-hz6/linux/run_hz6_preload_remote_profile_frontier.sh
```

Production R10 medians:

| Profile | `remote50` | `remote90` | `cross128_r90` | Peak RSS median range |
| --- | ---: | ---: | ---: | ---: |
| `selected` | 14.34M | 3.83M | 7.95M | 68.4-81.0 MiB |
| `transfer_presence` | 14.25M | 10.80M | 3.92M | 69.5-72.1 MiB |
| `small_class_shard` | 15.59M | 3.93M | 8.37M | 68.1-79.3 MiB |
| `toy2_split` | 14.46M | 11.17M | 8.63M | 73.7-77.3 MiB |

Decision:

```text
toy2_split:
  GO(high-remote/cross128 profile candidate)
  HOLD(default)
```

`toy2_split` is the best balanced HZ6 profile in this frontier for
`remote90 + cross128_r90`: it matches or beats the best median on both rows.
It is not the best `remote50` profile; `small_class_shard` remains stronger
there.  Therefore this is a specialist profile candidate, not a selected/default
change.

Next useful comparison:

```text
Remote allocator compare with toy2_split included
```

That comparison should place `toy2_split` against HZ3/HZ4/mimalloc/tcmalloc on
the same `remote50`, `remote90`, and `cross128_r90` rows before any paper-facing
claim.
