# Linux x86_64 HZ6 Transfer Small-Class Shard, 2026-06-20

## Box

`SmallClassTransferShard-L1`

This box keeps selected/default owner-slot transfer sharding unchanged and adds
a default-off partial class-id lane:

```text
HZ6_PROFILE_TRANSFER_SHARD_CLASS_MAX_ID=3
```

When enabled, transfer producer/consumer shard selection uses `class_id %
transfer_shards` for class ids `0..3`; class ids `4+` keep the default
owner-slot policy.  The broad `HZ6_PROFILE_TRANSFER_SHARD_CLASS_L1=1` lane still
forces class-id sharding for all classes.

## Verification

Commands:

```text
./hakozuna-hz6/linux/build_hz6_preload.sh
./hakozuna-hz6/linux/build_hz6_preload_transfer_small_class_shard_target.sh
./hakozuna-hz6/linux/build_hz6_r1_smokes.sh
./hakozuna-hz6/linux/run_hz6_preload_integrity_smoke.sh
HZ6_EXTRA_CFLAGS='-DHZ6_PROFILE_TRANSFER_SHARD_CLASS_MAX_ID=3' \
  ./hakozuna-hz6/linux/run_hz6_preload_integrity_smoke.sh
```

All passed.

## Focused Frontier RUNS=3

Raw root:

```text
hakozuna-hz6/private/raw-results/linux/hz6_preload_profile_frontier_20260620_062746
```

| row | selected | class-shard | small-class-shard |
| --- | ---: | ---: | ---: |
| `16_256` | 58.52M | 60.19M | 59.27M |
| `16_4096` | 56.51M | 57.31M | 57.23M |
| `1024_4096` | 73.01M | 67.52M | 70.49M |
| `4096_16384` | 62.68M | 61.75M | 63.93M |

## Remote MT RUNS=3

Raw root:

```text
hakozuna-hz6/private/raw-results/linux/hz6_transfer_shard_policy_ab_20260620_062821
```

| variant | local0 | remote50 | remote90 | peak MiB range |
| --- | ---: | ---: | ---: | ---: |
| selected | 15.93M | 13.97M | 10.83M | 67.38-71.97 |
| class-shard | 16.20M | 14.42M | 10.67M | 67.25-72.10 |
| small-class-shard | 16.31M | 14.79M | 11.13M | 67.38-71.97 |

## Remote MT RUNS=10

Raw root:

```text
hakozuna-hz6/private/raw-results/linux/hz6_transfer_shard_policy_ab_20260620_063108
```

| variant | local0 | remote50 | remote90 | median peak MiB |
| --- | ---: | ---: | ---: | ---: |
| selected | 16.17M | 15.00M | 10.93M | 67.25-72.08 |
| class-shard | 16.35M | 15.01M | 10.84M | 67.38-72.13 |
| small-class-shard | 15.95M | 15.29M | 10.87M | 67.31-72.16 |

## Decision

`GO(remote50 target)/NO-GO(default)`.

Small-class sharding keeps the useful small-row signal from broad class sharding
while avoiding part of the class 4/5 damage.  It also won the quick remote MT
RUNS=3 sample across local0, remote50, and remote90.

The RUNS=10 recheck did not justify selected/default promotion: remote50
improved, but local0 and remote90 were below selected.  Keep the boxed target
for remote50/profile exploration; do not promote it to default from this data.
