# HZ6 Ubuntu MidPage 32K Run Closeout

Date: 2026-06-15

## Scope

This note closes the Ubuntu LD_PRELOAD MidPage 32K source-run sizing lane.
The current selected value is:

```text
HZ6_MIDPAGE_32K_RUN_BYTES=786432
```

## Selected Ladder

| Lane | Status | Read |
| --- | --- | --- |
| 128K/192K/224K | no-go | RSS stayed flat while source allocation rose and 4096..16384 slowed. |
| 256K | control | Earlier selected direct control. |
| 512K | control | Previous selected default; target-positive with flat RSS. |
| 768K | selected/default | Best balanced promotion after 512K. |
| 1M/1.5M | control | More target-positive, but less guard-clean. |

## Run512 Promotion Evidence

Run512 was the first successful post-trim upsize.

```text
HZ6_MIDPAGE_32K_RUN_BYTES=524288
```

Stats-off repeat-7 versus 256K:

| row | 256K ops/s | 256K peak MiB | 512K ops/s | 512K peak MiB |
| --- | ---: | ---: | ---: | ---: |
| `16..256` | 59530701.620 | 30.38 | 60730135.954 | 30.50 |
| `16..4096` | 41810730.766 | 79.88 | 43235284.212 | 79.88 |
| `1024..4096` | 40855743.986 | 91.12 | 41567595.499 | 91.00 |
| `4096..16384` | 42175742.820 | 94.38 | 45298101.255 | 94.50 |

Raw:

```text
private/raw-results/linux/hz6_midpage_payload_trim_ab_20260614_194352
private/raw-results/linux/hz6_midpage_run512_confirm_20260614_194437
```

## Run768 Promotion Evidence

Run768 is selected after the free-hint/free-fastslot no-go closeout.

```text
HZ6_MIDPAGE_32K_RUN_BYTES=786432
```

Stats-off repeat-7 versus run512:

| row | 512K ops/s | 512K peak MiB | 768K ops/s | 768K peak MiB |
| --- | ---: | ---: | ---: | ---: |
| `16..256` | 57622875.452 | 30.50 | 58278310.110 | 30.38 |
| `16..4096` | 41850938.091 | 79.75 | 42389468.930 | 79.88 |
| `1024..4096` | 40600013.800 | 91.00 | 40386578.716 | 91.00 |
| `4096..16384` | 43110203.756 | 94.50 | 44324019.040 | 94.50 |

Safety spot-check:

```text
route_invalid=0
route_miss=0
alloc_fail=0
descriptor_exhausted=0
route_register_fail=0
source_block_exhausted=0
```

## Decision

```text
Keep 768K selected for Ubuntu LD_PRELOAD.
Keep 512K as the direct control.
Keep 1M/1.5M as target-positive controls, not selected.
Do not reopen smaller-than-256K payload-trim unless RSS goals change.
```
