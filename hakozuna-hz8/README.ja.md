# Hakozuna HZ8

HZ8は、現時点で推奨するHakozuna allocator lineです。

HZ8は「最速だけ」を狙うallocatorではなく、次の要素をまとめた
balanced defaultです。

```text
HZ8 = HZ3系のlocal fast path
    + HZ4系のowner-stable remote free
    + HZ5系のpressure / retention制御
    + HZ6系のfail-closed境界安全性
```

HZ8を使うべき場面:

```text
post-workload RSSを低く戻したい
owned pointerのINVALIDをplatform freeへ落としたくない
medium run retentionを上限付きにしたい
cross-thread freeを安全に扱いたい
LD_PRELOADでmalloc/free/calloc/realloc互換を使いたい
速度も一定以上ほしい
```

HZ8はtcmallocのようなthroughput-only allocatorではありません。
価値は、**安全性・RSS回復・remote free correctness・実用速度のバランス**
にあります。

## 論文

```text
HZ8 論文 DOI:
  https://doi.org/10.5281/zenodo.21084279

HZ8 論文 Zenodo record:
  https://zenodo.org/records/21084279

論文向け Ubuntu/Linux x86_64 matrix:
  docs/HZ8_PAPER_PUBLIC_MATRIX_UBUNTU_X86_64.md
```

## 現在の状態

```text
current public line:
  HZ8 v2 / KeepRefill + remote span-lease publish + Mag16

recommended default:
  yes

release record:
  docs/HZ8_MEDIUM_KEEP_REFILL_EMPTY_L1.md
  docs/HZ8_PRELOAD_SHIM_SURFACE_F1.md
  docs/HZ8_REMOTE_SPAN_LEASE_PUBLISH_L0.md
  docs/HZ8_MEDIUM_RUN_V1_1_RC.md
  docs/HZ8_V1_1_RELEASE.md
  docs/HZ8_PAPER_PUBLIC_MATRIX_UBUNTU_X86_64.md

paper DOI:
  https://doi.org/10.5281/zenodo.21084279

paper Zenodo record:
  https://zenodo.org/records/21084279

experimental throughput lane:
  docs/HZ8_V2_HZ9_DESIGN.md

windows bring-up lane:
  docs/HZ8_WINDOWS_BRINGUP.md

Linux lane registry:
  docs/HZ8_LINUX_LANE_STATUS_L1.md
  public default: hz8-v2 / Mag16
  larger/local opt-in: hz8-v2-mag32
  Windows exact-8KiB selected opt-in: hz8-r3-page8k-integrated
  docs/HZ8_PAGE8K_R3_INTEGRATED_GATE.md
```

HZ8-v2はHZ8-v1.1のbalanced baseを維持しつつ、KeepRefillをremote-heavy
pressure fixとしてdefault化しています。現行defaultには、LD_PRELOAD host
互換性のための`malloc_usable_size` / aligned allocation surfaceと、xmalloc型の
remote publish stormを止めるspan-lease remote publish / bounded backoff、
owner-local reusable small-span inventoryであるMag16も含みます。

```text
small:
  small-v0 frozen behavior

medium:
  4097..65536 bytes
  8K / 16K / 24K / 32K / 48K / 64K classes
  q64-v12-48k2 geometry
  64KiB quantum directory

remote free:
  owner-attached medium pending queue
  pending bitmap as remote-claim authority
  qstate owner collector protocol
  active-full Defer4 remote-pressure collection
  medium capacity collect budgeting
  owner-local refill-candidate empty-run keep-live
  span-lease remote publish instead of owner-wide publish lease
  bounded transition backoff under OWNER_TRANSITION retry storms

residency:
  budgeted empty-resident retention
  lazy128 persistent owner-attached reservation
  conservative retained-empty overhead contract is about 212MiB
  owner exit / detach / destroy are hard drain points

compatibility:
  pure LD_PRELOAD malloc/free/calloc/realloc surface
  malloc_usable_size + aligned allocation entrypoints for common preload hosts
```

## ユーザーはどれを選ぶべきか

通常ユーザーに見せる選択肢は **HZ8だけ** で十分です。

HZ3〜HZ7は設計系譜・比較基準であり、公開プロダクトとして並べるものではありません。

| Line | 役割 |
|---|---|
| HZ3 | local速度のreference |
| HZ4 | owner-stable remote-freeのreference |
| HZ5 | pressure / RSS制御のreference |
| HZ6 | fail-closed境界安全性のreference |
| HZ7 | route-safe移行prototype |
| HZ8 | 推奨balanced allocator |
| HZ9 | throughput研究lane。release defaultではない |

HZ9は、HZ8とは違う明確なpublic promiseを証明するまで、opt-inの研究線に留めます。
普通のユーザーにHZ3〜HZ9を選ばせない方針です。

## HZ8が守る契約

```text
1. owned-looking INVALID pointerはplatform freeへ落とさない
2. MISS / VALID / INVALIDを分離する
3. slot_stateをallocation validity authorityとして維持する
4. remote duplicate freeはpending bitでclaimする
5. owner lifecycleでstale owner reuseを防ぐ
6. owner exitでpending / retained / active-live medium stateをhard drainする
7. medium retentionは上限付きで明示する
8. slow-path pressure policyをhot-path profile selectorにしない
```

残っている弱点は、一部local-only rowでthroughput-first allocatorに届かない点です。
これはHZ8のcorrectness issueではなく、HZ9研究laneの入力として扱います。

## HZ8 v2 default

現在のHZ8 defaultには以下が入っています。

```text
MediumKeepRefillEmpty-L1
ReusableSpanMagazine-L1 Mag16
  docs/HZ8_MEDIUM_KEEP_REFILL_EMPTY_L1.md
  make bench-mediumkeeprefillempty
  make bench-release-mediumkeeprefillempty
  make preload-mediumkeeprefillempty

RemoteSpanLeasePublish-L0
  docs/HZ8_REMOTE_SPAN_LEASE_PUBLISH_L0.md
  default preload fix for the xmalloc remote publish livelock

PreloadShimSurface-F1
  docs/HZ8_PRELOAD_SHIM_SURFACE_F1.md
  malloc_usable_size / aligned allocation surface for LD_PRELOAD hosts
```

remote collectでmedium runが空になったとき、owner-local refill candidateなら
active-liveとして保持し、重いempty/reactivate loopを避けます。公開用の
cross-allocator matrixでもbalanced defaultとして確認済みです。ただし、HZ8が
tcmallocを全面的に超えたという主張ではありません。

## LargeDirect / ShardedHot evidence

HZ8 論文 line では、public default を保守的に維持します。

```text
default:
  HZ8-v2 / KeepRefill balanced default

opt-in evidence:
  LargeDirectOwned
  LargeDirectShardedHotCache-L1
```

LargeDirectOwned は、`cross128_r90` の弱点が主に large/direct boundary に
由来することを示す evidence です。cross128 row は大きく改善しますが、RSS
tradeoff も大きいため、default behavior ではなく opt-in profile / paper
evidence として扱います。

ShardedHotCache-L1 も evidence 扱いです。mechanism は動いており、現在の
sweep では 128 MiB total / 32 MiB per-shard cap が最も筋の良い形でした。
ただし throughput/RSS Pareto 点としてはまだ default-quality ではないため、
release default とは書きません。

## 論文向け公開マトリクスの要点

主スナップショット:

```text
docs/HZ8_PAPER_PUBLIC_MATRIX_UBUNTU_X86_64.md
Ubuntu 22.04.5 / Linux 6.8.0-90 / x86_64
RUNS=10, THREADS=16, ITERS=50000
```

代表行:

| Row | HZ8 KeepRefill | mimalloc | tcmalloc |
|---|---:|---:|---:|
| `small_interleaved_remote90` ops/s | 12.023M | 10.960M | 23.900M |
| `small_interleaved_remote90` post RSS | 2.91 MiB | 50.98 MiB | 32.94 MiB |
| `main_interleaved_r90` ops/s | 6.048M | 4.715M | 12.178M |
| `main_interleaved_r90` post RSS | 4.57 MiB | 183.12 MiB | 90.31 MiB |
| `medium_interleaved_r50` ops/s | 8.128M | 4.151M | 15.870M |
| `medium_interleaved_r50` post RSS | 3.81 MiB | 162.54 MiB | 79.06 MiB |

これは throughput / RSS tradeoff の表として読んでください。tcmallocは複数行で
raw throughput が強く、HZ8の主張は「実用速度を保ちつつ post-workload RSS が
非常に低い balanced allocator」です。

## Build

```bash
make smoke
make preload
make bench          # debug/counter build
make bench-release  # release throughput build
make bench-release-mediumkeeprefillempty  # v2 default互換alias
make preload-mediumkeeprefillempty        # v2 default DSO互換alias
make bench-release-largedirectdefault      # opt-in LargeDirect evidence
make bench-release-largedirectshardedhotcache  # opt-in ShardedHot evidence
```

よく使う確認:

```bash
./h8_smoke
./h8_bench_release --runs 10 --threads 16 --iters 100000 \
  --min-size 16 --max-size 2048 --remote-pct 0 --interleaved 0
```

pure preload matrix:

```bash
RUNS=5 THREADS=16 ITERS=50000 scripts/run_hz8_v11_same_run_matrix.sh
ALLOCATORS=hz8,system \
  ROWS=small_interleaved_remote90,main_interleaved_r90,medium_interleaved_r50 \
  RUNS=3 THREADS=16 ITERS=50000 scripts/run_hz8_v11_same_run_matrix.sh
MIMALLOC_SO=/path/to/libmimalloc.so \
TCMALLOC_SO=/path/to/libtcmalloc_minimal.so \
  scripts/run_hz8_keeprefill_public_matrix.sh
```

論文向けの公開マトリクス:

```text
docs/HZ8_PAPER_PUBLIC_MATRIX_UBUNTU_X86_64.md
```

HZ8 MT lane x remote% の snapshot:

```text
docs/HZ8_MT_LANE_REMOTE_PERCENT_SNAPSHOT.md
```

## ドキュメント入口

```text
docs/HZ8_V1_1_RELEASE.md
docs/HZ8_MEDIUM_RUN_V1_1_RC.md
docs/HZ8_SMALL_V0_RC1.md
docs/HZ8_OWNERSHIP_CONTRACT.md
docs/HZ8_OWNER_LIFECYCLE.md
docs/HZ8_BENCH_GATE.md
docs/HZ8_PUBLIC_RELEASE_PREP.md
docs/HZ8_PAPER_PUBLIC_MATRIX_UBUNTU_X86_64.md
docs/HZ8_MT_LANE_REMOTE_PERCENT_SNAPSHOT.md
docs/HZ8_WINDOWS_BRINGUP.md
docs/ALLOCATOR_MATRIX.md
docs/HZ8_V2_HZ9_DESIGN.md
```
