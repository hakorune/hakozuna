# Linux x86_64 HZ6 Remote Profile Frontier, 2026-06-20

Production-shaped profile frontier runs for the current HZ6 preload remote
profiles.  The runner uses real profile aliases instead of internal tax variants.

## Command

```bash
./hakozuna-hz6/linux/run_hz6_preload_remote_profile_frontier.sh \
  --runs 10 \
  --profiles selected,transfer_presence \
  --rows local0,remote50,remote90,cross128_r90

./hakozuna-hz6/linux/run_hz6_preload_remote_profile_frontier.sh \
  --runs 10 \
  --profiles selected,small_class_shard \
  --rows local0,remote50,remote90,cross128_r90
```

## Raw Results

- `hakozuna-hz6/private/raw-results/linux/hz6_remote_profile_frontier_r10_20260620_161529`
- `hakozuna-hz6/private/raw-results/linux/hz6_small_class_frontier_r10_20260620_161816`

## Median ops/s

| row | selected vs transfer | transfer_presence | selected vs shard | small_class_shard |
| --- | ---: | ---: | ---: | ---: |
| local0 | 16.87M | 16.53M | 16.58M | 16.17M |
| remote50 | 14.74M | 15.09M | 15.08M | 15.17M |
| remote90 | 3.84M | 10.94M | 4.00M | 4.09M |
| cross128_r90 | 10.02M | 4.51M | 7.04M | 8.82M |

The selected medians differ between the two batches because the profile pairs
were run separately.

## Decision

- `transfer_presence` is the current high-remote profile path.
- `small_class_shard` remains a cross128/remote50 specialist profile.
- Do not mix the two profiles: the prior combined tax row lost the useful
  transfer-presence balance.
- Do not promote either profile to selected/default from this evidence because
  each still has a row-specific regression.
