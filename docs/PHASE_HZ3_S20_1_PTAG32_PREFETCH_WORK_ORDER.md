# S20-1: PTAG32 prefetch（mixed の +0.5〜1% 狙い / 低リスク A/B）

## 目的

`hz3_free()` の hot path（S17+S18-1: PTAG dst/bin direct + FASTLOOKUP）で発生しうる **PTAG32 の load レイテンシ**を、`__builtin_prefetch()` で隠して **mixed を +0.5〜1%** 底上げする。

前提: **hot path は太らせない**（新しい共有構造/ロック/統計/環境変数/関数呼び出しは追加しない）。A/B は `-D` のみ。

## 背景（SSOT）

- S17（GO）: PTAG32 に `dst/bin` を埋めて、free を `range check + tag load + push` に短縮。
- S18-1（GO）: range check / page_idx / tag load をまとめた FASTLOOKUP で mixed も含めて改善（RUNS=30 で再確認済み）。
- 以降の改善は「命令数 1〜2 / load 1個」単位になり、**memory latency hiding** が候補に入る。

## Box Theory（箱の位置づけ）

- **Hot-only**: `hz3_free()` の `ArenaRangeCheckBox + PageTagLoadBox + BinPushBox`
- **Event-only**: PTAG の set/clear、central/inbox/epoch など（触らない）

S20-1 は Hot-only の `PageTagLoadBox` に **prefetch 1回だけ**を加える変更。

## 実装方針（必須）

### 1) 新フラグ（compile-time）

- `HZ3_PTAG32_PREFETCH=0/1`（default: `0`）
- 可能なら追加で
  - `HZ3_PTAG32_PREFETCH_LOCALITY=0..3`（default: `3`）

※フラグは `hakmem/hakozuna/hz3/include/hz3_config.h` に集約し、既存の命名規則に合わせる。

### 2) prefetch の挿入位置（安全第一）

**必ず in-range を確定してから** `page_idx` を使って prefetch する。

- out-of-range ポインタに対する prefetch はアーキ依存でリスクがあるため、**range check 前の prefetch は禁止**。
- PTAG32 base が未初期化の可能性が残るなら、prefetch 前に NULL を弾く（ただし hot が太るなら init 側の publish 不変条件を優先）。

推奨の形（概念）:

1. `base` を load
2. `delta` を計算
3. `delta` で範囲外判定（4GB arena の場合は `delta>>32`）
4. `page_idx` を計算
5. `__builtin_prefetch(&ptag32[page_idx], 0, locality)`
6. `tag = ptag32[page_idx]` を load（既存）

### 3) prefetch の対象

- 対象は **PTAG32 の 1要素**のみ（`uint32_t`）。
- bin/head など他の構造体 prefetch は S20-1 ではやらない（当たり外れが大きく、hot を汚しやすい）。

## A/B（SSOT）

### 条件

- SSOT script: `hakmem/hakozuna/hz3/scripts/run_bench_hz3_ssot.sh`
- 推奨: `RUNS=30`（mixed のノイズ対策）

### 期待と GO/NO-GO

- **GO 条件**
  - mixed: **+0.5% 以上**
  - small/medium: **退行なし（-1% 以内）**
- **NO-GO 条件**
  - mixed: 変化なし（±0%）または悪化
  - small の悪化が継続（-1% 以上）

※判断は RUNS=10 ではなく、原則 RUNS=30 の median で確定する。

## 追加計測（任意だが推奨）

prefetch は「命令数ではなく cycles」を狙う変更なので、mixed だけ `perf stat` を追加する。

- 最低限: `cycles,instructions,branches,branch-misses`
- 可能なら: `L1-dcache-load-misses,LLC-load-misses,dTLB-load-misses`

期待方向:

- `instructions` はほぼ同等
- `cycles` が減る（IPC が上がる）

## 実装ファイル候補（目安）

- `hakmem/hakozuna/hz3/include/hz3_config.h`（フラグ追加）
- `hakmem/hakozuna/hz3/include/hz3_arena.h`（FASTLOOKUP内の tag load 直前に prefetch）
- もしくは `hakmem/hakozuna/hz3/src/hz3_hot.c`（`hz3_free` 側の tag load 直前）

※S18-1 の「FASTLOOKUP ルート」にだけ入れる（他のルートに分散させない）。

## 失敗時（NO-GO）の片付け規約

NO-GO になった場合は、**実装を revert**して以下を必ず残す:

1. `hakmem/hakozuna/hz3/archive/research/s20_1_ptag32_prefetch/README.md`
   - baseline/ON の SSOT ログパス
   - RUNS/ITERS/WS/commit hash
   - median 表（small/medium/mixed）
   - `perf stat` を取った場合はログパス
2. `hakmem/hakozuna/hz3/docs/NO_GO_SUMMARY.md` に追記
3. 本ファイル（work order）に最終判定を追記（GO/NO-GO）

## 補足（次案の扱い）

- **base NULL チェック削除**は「初期化順序・foreign ptr・終了処理」が絡むため、S20-1 では扱わない（別フェーズで fail-fast 不変条件から詰める）。
- **bank レイアウト変更**は影響範囲が広いので、prefetch が NO-GO のあとに `perf` の根拠を揃えてから着手する。
