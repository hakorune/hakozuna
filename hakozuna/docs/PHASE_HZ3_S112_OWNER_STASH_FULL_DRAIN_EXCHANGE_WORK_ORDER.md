# PHASE HZ3 S112: OwnerStash FullDrainExchangeBox（S67-4 bounded drain の CAS retry を潰す）— Work Order

目的（ゴール）:
- `hz3_owner_stash_pop_batch()` のホットスポットである **S67-4 bounded drain**（`HZ3_S67_SPILL_ARRAY2 && HZ3_S67_DRAIN_LIMIT`）の
  **CAS retry + O(n) re-walk** を除去する。
- mimalloc-like に **atomic_exchange で全量 drain（retry なし）** し、余剰は TLS spill（`spill_array` + `spill_overflow`）で保持する。
- 変更は **フラグで A/B 可能**、観測は **atexit 1行 SSOT** のみ。

状態:
- **完了（GO）**。scale lane 既定で有効化（`HZ3_S112_FULL_DRAIN_EXCHANGE=1`）。

背景（切り分け）:
- ❌「S67-2 が CAS retry」
- ✅「S67-4 bounded drain ブロックが CAS retry（失敗時に max_take まで re-walk）」

非ゴール:
- TLS spill レイアウト（S67-2）の作り直し（別箱）。
- owner stash の packet 化（S93）や構造変更（別箱）。
- ここで pressure/flush の全体設計を変える（まずは A/B で勝ち筋確認）。

---

## 箱割り（境界は1箇所）

S112 の境界は `hakozuna/hz3/src/hz3_owner_stash.c` の
`hz3_owner_stash_pop_batch()` 内:
- `// S67-4: Bounded drain ...` の **直前 1箇所**（bounded drain を差し替える場所）に固定する。

ここでのみ行う:
- `atomic_exchange(headp, NULL)` による full drain
- `out[]` 充填 + `spill_array` 充填
- 余りを `spill_overflow[sc]` に保存（stash へ push-back しない）

---

## フラグ（A/B と切り戻し）

`hakozuna/hz3/include/hz3_config_scale.h`:
- `HZ3_S112_FULL_DRAIN_EXCHANGE=0/1`（scale lane 既定: `1`）
- `HZ3_S112_STATS=0/1`（atexit 1行）
- `HZ3_S112_FAILFAST=0/1`（debug 専用）

compile guard（必須）:
- `HZ3_S44_OWNER_STASH=1`
- `HZ3_REMOTE_STASH_SPARSE=1`
- `HZ3_S44_OWNER_STASH_COUNT=0`
- `HZ3_S67_SPILL_ARRAY2=1`（S67-2 の `spill_array/spill_overflow` を前提にする）

注意（互換 / 排他）:
- `HZ3_S99_ALLOC_SLOW_PREPEND_LIST` は、S112 有効時は **自動で 0**（spill_array 内の batch は “linked list” ではないため）。

---

## 実装（S112-0 → S112-1）

### S112-0（観測 / 挙動不変）
目的: bounded drain の retry が出ているか、どの程度かを SSOT で固定する。

- 既存の S67 stats（`HZ3_S67_STATS=1`）を使ってよい:
  - `bounded_drain_cas_retry` が伸びるか確認

### S112-1（full drain exchange 差し替え）
要点:
- bounded drain ブロック（S67-4）を `#if !HZ3_S112_FULL_DRAIN_EXCHANGE` にして無効化
- 代わりに full drain exchange を挿入し、その場で `return got;` する（下流の “bounded overflow 前提” を踏まない）

核心ロジック（概形）:
1) `spill_array/overflow`（既存 S67-2 Step 0）で取れるだけ取る（既存）
2) `atomic_exchange(headp, NULL, acquire)` で全量取得（retry なし）
3) `out[]` に `want` まで詰める
4) 残りを `spill_array` に CAP まで入れる
5) さらに残った分は `spill_overflow[sc]=cur` に保存（stash push-back / tail scan なし）

Fail-Fast（debugのみ）:
- `spill_overflow[sc]` が non-NULL のまま S112 ブロックに到達したら abort（Step0b の不変条件違反）

---

## 注意（リスク / 観測ポイント）

S112 は “stash に残す” 代わりに “TLS spill に保持” するため、次を観測する:
- RSS/メモリが増えないか（remote-free が多く alloc が少ないケース）
- pressure 系の flush が stash だけを見ている場合、spill 側が隠れて影響しないか

まずは `HZ3_S112_STATS=1` で到達率（exchange_calls / overflow_set_calls）を固定してから、必要なら次フェーズで pressure 連携を箱として追加する。

---

## GO/NO-GO（scale lane）

GO:
- `mt_remote_r50_small` / `mt_remote_r90_small` で +1% 以上（または perf で `hz3_owner_stash_pop_batch` の self% が有意に下がる）
- `dist_app` / `dist_uniform` が -1% を超えて退行しない

NO-GO:
- `dist_app` が -1% 超の退行
- RSS の明確な悪化（pressure/idle ワークロードで顕著）

---

## A/B（最低限スイート）

ビルド例（scale lane）:
```sh
make -C hakozuna/hz3 clean all_ldpreload_scale
cp ./libhakozuna_hz3_scale.so /tmp/s112_base.so

make -C hakozuna/hz3 clean all_ldpreload_scale \
  HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_S112_FULL_DRAIN_EXCHANGE=1 -DHZ3_S112_STATS=1'
cp ./libhakozuna_hz3_scale.so /tmp/s112_treat.so
```

スイート（推奨）:
- `mt_remote_r90_small` / `mt_remote_r50_small` / `mt_remote_r0_small`
- `dist_app` / `dist_uniform`
- `mstress` は短時間 1本（timeout cut 可、ただし最終的には通す）

---

## 結果（2026-01-13）

A/B:
- xmalloc-test T=8: **67M → 84M ops/s（+25%）**
- SSOT Small: **109M → 107M（-2%）**
- SSOT Medium: **113M → 113M（0%）**
- SSOT Mixed: **107M → 107M（0%）**

判定:
- **GO**（scale lane 既定: `HZ3_S112_FULL_DRAIN_EXCHANGE=1`）
