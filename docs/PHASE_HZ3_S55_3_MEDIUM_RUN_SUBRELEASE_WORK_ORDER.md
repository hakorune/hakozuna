# PHASE_HZ3_S55-3: MediumRunSubreleaseBox（FROZEN: RSS削減 / event-only）

背景:
- S55-1 OBSERVE では `arena_used_bytes`（segment/slot 使用量 proxy）が 97–99% を支配。
- S55-2（OpenDebtGate）/S55-2B（Epoch Repay）は **full-free segment を返す**前提のため、`mstress` では `gen_delta==0` が続き RSS が動かず **NO-GO**。
- つまり、steady-state の RSS を下げるには「segment を丸ごと返す」以外の手段が必要。

結論（方向性）:
- **“返せない segment”を保持したまま、freeになった medium run（4KB–32KB）だけを OS に返す**（tcmalloc の subrelease/purge 相当）。
- hot path は触らず、**epoch境界（event-only）だけ**で実行する。

---

## 0) スコープ（最小）

- 対象: **medium run（4KB–32KB）だけ**
  - small（<=2KB）は intrusive freelist で `madvise` と相性が悪いので触らない。
  - large は S53 系で別箱があるのでここでは触らない。
- 実行境界: `hz3_epoch_force()` のみ（event-only）
- 既定: OFF（opt-in）

---

## 1) 仕組み（箱の境界は1箇所）

### 1.1 入口（boundary）

`hz3_epoch_force()` の末尾（policy/learn の後）に 1回だけ:

1. “medium run demote”（S45 Phase2 と同型）で、central から **小 budget** だけ run を pop して `hz3_segment_free_run()` に戻す
2. その直後に **同じ run 範囲へ `madvise(MADV_DONTNEED)`** を実行して RSS を落とす

重要:
- `madvise` は **central から外した後**に行う（intrusive list の next を壊さない）。
- 返却は “segment単位” ではなく “run単位” なので、断片化で full-free segment が作れなくても RSS は落とせる。

---

## 2) A/B（compile-time）

最低限:
- `HZ3_S55_3_MEDIUM_SUBRELEASE=0/1`（default 0）

ノブ（最初は固定値でOK）:
- `HZ3_S55_3_MEDIUM_SUBRELEASE_BUDGET_PAGES=...`（1 epoch の上限、例: 256 pages = 1MiB）
- `HZ3_S55_3_MEDIUM_SUBRELEASE_INTERVAL=...`（間引き、例: 16 epoch に1回）
- `HZ3_S55_3_MEDIUM_SUBRELEASE_EPOCH_MULT=...`（追加間引き倍率。`HZ3_S55_REPAY_EPOCH_INTERVAL` に掛ける）
  - まずは `1` を推奨（“一度も発火しない”事故を避ける）

Fail-Fast（デバッグ用）:
- `HZ3_S55_3_MEDIUM_SUBRELEASE_FAILFAST=0/1`
  - `madvise` が失敗したら one-shot でログして abort（研究用）

---

## 3) 可視化（one-shot / 統計）

atexit で 1回だけ:

`[HZ3_S55_3_MEDIUM_SUBRELEASE] calls=%u demote_pages=%u madvise_pages=%u madvise_bytes=%zu`

“どれだけ返せたか” を固定し、RSS変化と突き合わせる。

---

## 4) 実装差し込み点（候補）

安全に最小で済ませるため、S45 の既存境界を再利用する:

- `hz3_mem_budget_reclaim_medium_runs()` を “budget版” にする（戻り値で `madvise` 対象範囲を回収できるように）
  - 例: `hz3_mem_budget_reclaim_medium_runs_collect(out_ranges[], cap, pages_budget) -> nranges`
  - `out_ranges[]` は `(addr, len)`（ページアライン）だけ

その結果を S55-3 から `madvise` する（epoch境界のみ）。

---

## 5) 測定（GO/NO-GO）

GO（まずの合格ライン）:
- `mstress` mean RSS が **-10% 以上**（vs baseline）
- 速度退行 **-2% 以内**（SSOT ±2%）

NO-GO:
- RSS が -5% 未満しか動かないのに速度が落ちる
- `madvise` が原因のクラッシュ/データ破壊（即 revert）

---

## 6) 注意（安全）

- `madvise` の対象は “完全に free になった medium run（pages単位）” のみ。
- small の intrusive freelist に `madvise` を当てない（next が消える）。
- `hz3_segment_free_run()` の **my_shard 制約**を守る（S45と同様）。

---

## 7) 予定: Hybrid（Linux/Windows）対応の箱割り

結論:
- “返却の policy（いつ/どれだけ）” と “OS API（どう返す）” を分離する。
- S55-3 は **policy箱**として OS 非依存に保ち、OS差分は **薄いOS箱**に閉じ込める。

### 7.1 OS PageAdviceBox（新設案）

新しい薄い箱を作り、S55-3 はここだけ呼ぶ:

- API（例）:
  - `hz3_os_page_discard(addr, len)`（物理ページを捨ててよい。VAは保持）

実装（例）:
- Linux:
  - `madvise(addr, len, MADV_DONTNEED)`
- Windows:
  - `VirtualFree(addr, len, MEM_DECOMMIT)`（同等の意味）

### 7.2 方針（箱理論）

- S55-3:
  - event-only（epoch境界のみ）
  - budget/interval の制御のみ
  - OS分岐を持たない
- OS PageAdviceBox:
  - OS API の差分のみ
  - 可観測性（one-shotの失敗統計）はここに集約しても良い

### 7.3 TODO（計画）

1. まず Linux で S55-3 を実装・GO/NO-GO判定
2. GO したら OS PageAdviceBox を導入して API 経由に差し替え（Linuxは同挙動）
3. Windows 実装（`VirtualFree(MEM_DECOMMIT)`）を追加し、同じテスト（RSS/速度）を通す

---

## S55-3 測定結果（2026-01-06）

### ビルド条件
- S55-3有効: `HZ3_S55_RETENTION_FROZEN=1 -DHZ3_S55_3_MEDIUM_SUBRELEASE=1`
- 発火頻度: `HZ3_S55_REPAY_EPOCH_INTERVAL=1 -DHZ3_S55_3_MEDIUM_SUBRELEASE_EPOCH_MULT=1`
- ウォーターマーク: SOFT=256MB, HARD=384MB, HYST=64MB

### 1. mstress 10 100 100（連続高負荷：最悪系）

| Metric | OFF | ON | 削減率 |
|--------|-----|-----|--------|
| RSS mean | 853 MB | 805 MB | -5.6% |
| RSS max | 905 MB | 860 MB | -5.0% |
| RSS p95 | 905 MB | 859 MB | -5.1% |
| PSS mean | 755 MB | 715 MB | -5.3% |
| PSS max | 905 MB | 856 MB | -5.4% |
| PSS p95 | 905 MB | 856 MB | -5.4% |

**S55-3統計**:
- calls=870
- demote_pages=200305
- madvise_calls=27840
- madvise_bytes=820449280 (782.7 MB)

**判定**: 目標-10%未達（実績-5.6%）

### 原因分析

mstress は連続高負荷ワークロードで、以下の特性がある:
- madvise で物理メモリを返却しても、すぐに再アロケーションで再利用される
- "burst/idle" パターンではないため、subrelease の効果が限定的
- madvise_bytes=782.7MB に対して RSS削減は48MB程度 → 返却したメモリの大部分がすぐ再利用されている

### 次のステップ

S55-3 の「効く/効かない」を切り分けるため、以下のワークロードで追加測定が必要:

1. **cache-thrash/cache-scratch（mimalloc-bench）**
   - burst/idle パターン
   - メモリ確保→使用→解放のサイクルが明確
   - idle期間中に subrelease が効果を発揮する可能性

2. **larson-sized（mimalloc-bench）**
   - temporal locality が高い
   - 局所的なメモリ使用パターン

3. **malloc-large（mimalloc-bench）**
   - 大きなアロケーションパターン

### 作業記録

測定環境:
- ベンチマーク: `/mnt/workdisk/public_share/hakmem/mimalloc-bench/out/bench/mstress`
- 測定スクリプト: `/mnt/workdisk/public_share/hakmem/hakozuna/hz3/scripts/measure_mem_timeseries.sh`
- PSS測定間隔: 20回に1回（PSS_EVERY=20）
- 結果: `/tmp/s55_3_mstress_off.csv`, `/tmp/s55_3_mstress_on.csv`
- ログ: `/tmp/s55_3_mstress_off.log`, `/tmp/s55_3_mstress_on.log`

---

## 追加測定結果（2026-01-06）

### 1. cache-thrash（burst/idle系）

実行:
```bash
# OFF
OUT=/tmp/s55_3_cache_off.csv PSS_EVERY=20 \
  LD_PRELOAD=./hakmem/libhakozuna_hz3_scale.so \
  hakmem/hakozuna/hz3/scripts/measure_mem_timeseries.sh \
  mimalloc-bench/out/bench/cache-thrash 1 1000 1 2000000 8

# ON (S55-3 enabled)
OUT=/tmp/s55_3_cache_on.csv PSS_EVERY=20 \
  LD_PRELOAD=./hakmem/libhakozuna_hz3_scale.so \
  hakmem/hakozuna/hz3/scripts/measure_mem_timeseries.sh \
  mimalloc-bench/out/bench/cache-thrash 1 1000 1 2000000 8
```

結果:
- OFF: RSS mean=9.8MB, max=9.8MB (実行時間: 1秒)
- ON:  RSS mean=9.8MB, max=9.8MB (実行時間: 1秒)

判定: **測定不可**
- 実行時間が短すぎて（約1秒）、S55-3の効果を測定できない
- RSS が安定して 9.8MB で変化なし

### 2. larson-sized（temporal locality高い）

実行:
```bash
# OFF
OUT=/tmp/s55_3_larson_off.csv PSS_EVERY=20 \
  LD_PRELOAD=./hakmem/libhakozuna_hz3_scale.so \
  hakmem/hakozuna/hz3/scripts/measure_mem_timeseries.sh \
  mimalloc-bench/out/bench/larson-sized 5 8 1000 5000 100 4141 8

# ON (S55-3 enabled)
OUT=/tmp/s55_3_larson_on.csv PSS_EVERY=20 \
  LD_PRELOAD=./hakmem/libhakozuna_hz3_scale.so \
  hakmem/hakozuna/hz3/scripts/measure_mem_timeseries.sh \
  mimalloc-bench/out/bench/larson-sized 5 8 1000 5000 100 4141 8
```

| Metric | OFF | ON | 削減率 |
|--------|-----|-----|--------|
| RSS mean | 65.4 MB | 65.4 MB | 0.00% |
| RSS max | 66.8 MB | 66.9 MB | -0.19% |
| 実行時間 | 10.567s | 10.299s | - |

判定: **効果なし**
- RSS削減はほぼゼロ（-0.19% は測定誤差）
- mstress と同様、連続使用パターンでは効果が限定的

---

## 総合判定

3つのワークロード結果:
1. **mstress（連続高負荷）**: RSS mean -5.6%（目標-10%未達）
2. **cache-thrash（burst/idle）**: 測定不可（実行時間短すぎ）
3. **larson-sized（temporal locality）**: RSS 0.00%（効果なし）

結論: **S55-3（A案）は効果不十分 → B案（遅延subrelease）に進む**

### 課題と次のステップ

現在の即時subreleaseは以下の理由で効果が限定的:
- madvise で返却したメモリがすぐ再利用される
- burst/idle パターンでも効果が見られない
- 連続高負荷ワークロードでは特に効果が薄い

次のアプローチ（B案: 遅延subrelease）:
- run から返却された segment を即座に madvise せず、一定期間保持
- idle期間中に集約して返却する
- 返却タイミングの最適化（epoch-based or adaptive）

