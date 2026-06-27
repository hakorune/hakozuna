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

## 現在の状態

```text
current public line:
  HZ8 MediumRun-v1.1

recommended default:
  yes

release record:
  docs/HZ8_MEDIUM_RUN_V1_1_RC.md
  docs/HZ8_V1_1_RELEASE.md

experimental throughput lane:
  docs/HZ8_V2_HZ9_DESIGN.md
```

HZ8-v1.1で固定しているdefault:

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

residency:
  budgeted empty-resident retention
  lazy128 persistent owner-attached reservation
  conservative retained-empty overhead contract is about 212MiB
  owner exit / detach / destroy are hard drain points

compatibility:
  pure LD_PRELOAD malloc/free/calloc/realloc surface
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

残っている弱点は、local-only rowやmedium remote-heavy rowでのthroughputです。
これはHZ8-v1.1のcorrectness issueではなく、v2 / HZ9研究laneの入力として扱います。

## Build

```bash
make smoke
make preload
make bench          # debug/counter build
make bench-release  # release throughput build
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
```

## ドキュメント入口

```text
docs/HZ8_V1_1_RELEASE.md
docs/HZ8_MEDIUM_RUN_V1_1_RC.md
docs/HZ8_SMALL_V0_RC1.md
docs/HZ8_OWNERSHIP_CONTRACT.md
docs/HZ8_OWNER_LIFECYCLE.md
docs/HZ8_BENCH_GATE.md
docs/ALLOCATOR_MATRIX.md
docs/HZ8_V2_HZ9_DESIGN.md
```
