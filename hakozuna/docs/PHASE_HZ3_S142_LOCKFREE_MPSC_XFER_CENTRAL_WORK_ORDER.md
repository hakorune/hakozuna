# PHASE HZ3 S142: Lock-Free MPSC Xfer/Central（pthread_mutex 削減）— Work Order

Status: **GO（scale+p32 既定ON / fast 既定OFF）**

目的:
- `pthread_mutex_lock/unlock` の固定費を削り、`hz3_small_v2_alloc_slow` 周辺の命令数/遅延を落とす。
- scale lane の slow path（refill / remote push overflow）で **mutex 依存を外す**（hot path は触らない）。

背景（観測）:
- perf record（workload_bench）で `pthread_mutex_unlock` が大きい（xfer/central/slow path 起因）。
- S42 xfer / small_v2 central は「per-owner」構造で、remote 側が push、owner 側が pop するため **MPSC** に落とし込みやすい。

重要な前提（Box Theory の境界）:
- **MPSC 前提**（multi-producer / single-consumer）でのみ lock-free を GO とする。
- `threads > HZ3_NUM_SHARDS` で shard collision を許す運用では、同一 owner を複数スレッドが consumer になり得るため、
  **pop 側は `OwnerExclBox` で直列化**（single-consumer 化）するか、collision tolerant では mutex 経路に戻す。
- Treiber stack（MPMC）を安易に入れない（ABA で壊れやすい）。**`atomic_exchange` drain 型**を標準形にする。

---

## 1) 箱（Box）定義

### S142-1: XferMpscBox（S42 の lock-free 版）

境界:
- producer: `hz3_small_v2_push_remote_list()`（remote → owner）
- consumer: `hz3_small_v2_alloc_slow()` の refill 入口（owner のみ）

API（案）:
- `hz3_small_xfer_push_list(owner, sc, head, tail, n)`（既存の署名を維持）
- `hz3_small_xfer_pop_batch(owner, sc, out, want)`（既存の署名を維持）

実装方針（MPSC stack）:
- `head` を `_Atomic(void*)` にして **list-of-objects** を直接積む（slot 配列は捨てる）。
- push: `tail->next=old_head` → CAS publish（release）
- pop: `atomic_exchange(head,NULL)` で drain → want 分だけ取り、残りは再 push（release）
- pending 判定: `atomic_load(head)!=NULL`（count は近似で良い）
 - lock-free では **slot 容量制限がなくなる**ため、xfer は実質「無制限の MPSC キュー」になる（overflow は central に落とさない）。

### S142-2: CentralMpscBox（generic central bin の lock-free 版）

対象:
- `hz3_small_v2_central`（まずここだけ）→ 成功したら sub4k/medium へ横展開を検討

注意（debug 互換）:
- S86 shadow（dup/next 検出）は「publish と shadow record の順序」が絡むため、
  **S142 を有効にする build では S86 を無効**（または S86=ON のときは mutex 経路へ強制フォールバック）。

---

## 2) フラグ（戻せる）

フラグ（compile-time）:
- `HZ3_S142_XFER_LOCKFREE=0/1`（scale+p32 既定ON）
- `HZ3_S142_CENTRAL_LOCKFREE=0/1`（scale+p32 既定ON）
- collision 運用の安全ゲート:
  - `HZ3_OWNER_EXCL_ALWAYS=1`（`HZ3_SHARD_COLLISION_FAILFAST=0` のとき既定で 1）

運用:
- **scale+p32 既定ON / fast 既定OFF**。A/B で OFF にする場合は `-DHZ3_S142_*_LOCKFREE=0` を明示する。

---

## 3) A/B（SSOT）手順

前提:
- 出力は `>/dev/null` で潰す（perf ノイズ削減）。
- 比較は「同一バイナリ + LD_PRELOAD 切替」を守る。

### 3-1. MT remote（allocator 支配）
- `hakozuna/out/bench_random_mixed_mt_remote_malloc` を使い、
  `T=16/32/64/128` × `R=50/90` を 3 runs median で取る。
- 比較対象: hz3 scale / hz3(p32) / tcmalloc / mimalloc（必要なら system）

### 3-2. Redis（実アプリ寄り・回帰検知）
- `benchmarks/redis/workload_bench_system 1 100 1000 16 1024` を同条件で比較（T=1 を最初に）。
- fast lane は `malloc_usable_size` 周りの回帰を拾いやすいので、scale lane を主戦場にする。

### 3-3. perf stat（命令数の芯）
- `perf stat -e instructions,cycles,branches,branch-misses,L1-dcache-loads,L1-dcache-load-misses` で
  hz3 scale vs tcmalloc を比較し、S142 ON/OFF の差分が **mutex 起因の命令数**に効いているか確認する。

GO/NO-GO（目安）:
- GO: MT remote r90 で +5% 以上、かつ R=0 固定費の退行なし
- NO-GO: 退行が多い/衝突運用で不安定 → archive（stub 化）

---

## 4) リスク（Fail-Fast）

- collision 運用で consumer が複数になると MPSC 前提が崩れる → `OwnerExclBox` で直列化（または mutex fallback）。
- debug shadow（S86）との順序問題 → S142 lane では S86 を OFF。
- list corruption は boundary で fail-fast（`HZ3_LIST_FAILFAST` / xfer 側の struct failfast は境界のみ）。

危険な組み合わせ（禁止）:
- `HZ3_S142_CENTRAL_LOCKFREE=1` + `HZ3_SHARD_COLLISION_FAILFAST=0`（OwnerExcl 未強制）
- `HZ3_S142_CENTRAL_LOCKFREE=1` + `HZ3_S86_CENTRAL_SHADOW=1`
- `HZ3_S142_XFER_LOCKFREE=1` + `HZ3_SHARD_COLLISION_FAILFAST=0`（pop 側の単一化が崩れる）

---

## 5) 実測結果（S142-A: Central lock-free）

条件:
- ビルド: `HZ3_S142_CENTRAL_LOCKFREE=1`（scale lane）
- ベンチ: `bench_random_mixed_mt_remote_malloc`（T=8 RANDOM, R=0 固定費, r90）

結果:
- T=8 RANDOM: baseline 1.19 → **1.33 M ops/s**（**+11.8%**、mimalloc/tcmalloc と同等）
- R=0 固定費（T=8, 10 runs median）: baseline 507.20M → S142-A 504.16M（**-0.6% ノイズ**）
- r90: T=8 **+5.9%**, T=16 **+25.7%**, T=32 **+56.3%**

判定:
- **GO（scale lane 既定ON候補）**
- 条件: collision failfast 前提 / S86 shadow 併用は避ける

---

## 6) S142-B: Xfer lock-free（次フェーズ）

狙い:
- S42 の mutex を外し、remote-heavy での slow path 固定費をさらに削減する。
- 期待値は **+5% 前後**（ただし perf で mutex 由来が残っている場合のみ）。
- 直近のトリガー（r50, T=8）: `pthread_mutex_unlock/lock`（xfer）合計 **4.38%** が残っているため着手可。

前提:
- per-owner MPSC（push=multi-producer / pop=single-consumer）
- `threads > HZ3_NUM_SHARDS` の collision を許す運用では **OwnerExcl で pop を直列化**するか、S142-B を使わない。

ビルド:
```
make -C hakozuna/hz3 clean all_ldpreload_scale \
  HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_S142_XFER_LOCKFREE=1'
```

Fail-Fast（任意）:
```
make -C hakozuna/hz3 clean all_ldpreload_scale \
  HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_S142_XFER_LOCKFREE=1 -DHZ3_S142_FAILFAST=1 -DHZ3_XFER_STRUCT_FAILFAST=1'
```

ベンチ（SSOT）:
- R=0 固定費: `./hakozuna/out/bench_random_mixed_mt_remote_malloc 8 2500000 400 16 2048 0`
- r50: `./hakozuna/out/bench_random_mixed_mt_remote_malloc 8 2500000 400 16 2048 50`
- r90: `./hakozuna/out/bench_random_mixed_mt_remote_malloc 8 2500000 400 16 2048 90`
- T=16/32 でも r90 を一巡（3 runs median）

perf（ノイズ対策）:
- `>/dev/null` でベンチ出力を潰してから `perf record -g` を再取得する。

GO/NO-GO:
- GO: r50/r90 が **+5% 以上**、R=0 固定費の退行なし
- NO-GO: 退行 or collision 運用で不安定 → archive / stub

---

## 7) 実測結果（S142-B: Xfer lock-free）

条件:
- ビルド: `HZ3_S142_XFER_LOCKFREE=1`（S142-A に追加）
- ベンチ: `bench_random_mixed_mt_remote_malloc`（T=8/16/32, R=0/r50/r90）

結果（10 runs median）:

| 条件 | Baseline (S142-A) | S142-A+B | 変化 |
|------|-------------------|----------|------|
| R=0 T=8 | 495.5M | 505.0M | **+1.9%** |
| r50 T=8 | 181M | 182M | ±0% |
| r90 T=8 | 123M | 174M | **+41%** |
| r90 T=16 | 134M | 137M | +2% |
| r90 T=32 | 84M | 172M | **+105%** |

判定:
- **GO（scale lane 既定ON候補）**
- R=0 固定費の退行なし（むしろ +1.9%）
- r90 で大幅改善（T=8 +41%, T=32 +105%）

補足（p32 lane, r90）:
- T=64: S142 ON **+4.6%**（median 93.5M vs 89.4M）
- T=128: shard collision failfast が発生し `shards=96` と表示される問題あり（`HZ3_P32_NUM_SHARDS=255` が反映されていない疑い）。要調査。
