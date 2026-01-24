# HZ4 Phase 6: NO-GO Summary

Phase 6 で試行した 3 つの最適化はすべて NO-GO となった。
本ドキュメントはその知見を記録する。

---

## Overview

| 最適化 | 目標 | 結果 | 変化 | 日付 |
|--------|------|------|------|------|
| PAGE_TAG_TABLE | +9% | NO-GO | **-3.1%** | 2026-01-23 |
| REMOTE_FLUSH_PAGE_BUCKET | +5% | NO-GO | **-3.4%** | 2026-01-23 |
| TLS_STRUCT_MERGE | +3% | NO-GO | **-0.7%** | 2026-01-23 |

ベースライン: T=16, R=0, ~305M ops/s

---

## 1. PAGE_TAG_TABLE (-3.1%)

### 仮説
- ptr → page header の magic load を tag table lookup に置換
- 1 回の memory load で kind/sc/owner_tid を取得
- hz3 の PTAG32 成功体験から移植

### 実装
```c
#if HZ4_PAGE_TAG_TABLE
uint32_t tag;
if (hz4_pagetag_lookup(ptr, &tag)) {
    uint8_t kind = hz4_pagetag_kind(tag);
    // route by kind
}
#endif
```

### 失敗原因
| 要因 | 影響 |
|------|------|
| **アーキテクチャ不一致** | 致命的 |
| tag table lookup overhead | +18-22 cycles |
| page header は依然必要 | 二重コスト |

**根本原因**: hz3 は tag に bin+dst を encode して完全ルーティング可能。
hz4 は tag から kind しか取れず、page header アクセスが必須。
→ overhead 追加のみで削減効果なし。

### 教訓
- 他プロジェクトの成功パターンを移植する際はアーキテクチャ適合性を確認
- "何を省略できるか" が重要（hz4 では page header を省略できない）

---

## 2. REMOTE_FLUSH_PAGE_BUCKET (-3.4%)

### 仮説
- remote flush の bucketing を (owner, sc) → page-based に変更
- 同一 page のオブジェクトを集約して CAS 回数削減
- inbox push_list の効率向上

### 実装
```c
#if HZ4_REMOTE_FLUSH_PAGE_BUCKET
// Hash: XOR mix
uint32_t idx = (uint32_t)(((paddr >> HZ4_PAGE_SHIFT) ^ (paddr >> 12))
               & (HZ4_REMOTE_FLUSH_BUCKETS - 1));
// Linear probe for collision
for (uint32_t probe = 0; probe < HZ4_REMOTE_FLUSH_PROBE; probe++) { ... }
#endif
```

### 失敗原因
| 要因 | 影響 |
|------|------|
| Linear probe overhead | -1.5% |
| Hash collision rate | -1.0% |
| Page header cache eviction | -0.8% |
| Memory pressure | -0.2% |
| **合計** | **-3.5%** |

**根本原因**: 既存の (owner, sc) bucketing は collision が少なく near-optimal。
page-based は probe 回数増加と page header 再読み込みでコスト増。

### 教訓
- "理論的に正しい" 最適化も実測で検証必須
- 既存実装が "偶然" 最適に近い場合がある
- hash collision の実測が重要

---

## 3. TLS_STRUCT_MERGE (-0.7%)

### 仮説
- `hz4_alloc_tls_t` を `hz4_tls_t` に統合
- TLS indirection (2回 → 1回) で固定費削減
- local path の高速化

### 実装
```c
struct hz4_tls {
#if HZ4_TLS_MERGE
    hz4_tcache_bin_t bins[HZ4_SC_MAX];  // 2KB at front
    hz4_seg_t* cur_seg;
    uint32_t next_page_idx;
    uint8_t owner;
#endif
    // ... existing fields ...
};
```

### 失敗原因
| 要因 | 影響 |
|------|------|
| **carry[] cache line 遠方化** (CL 16→48) | **-0.4~0.5%** |
| TLS parameter register pressure | -0.2~0.3% |
| Addressing offset 拡大 | -0.1% |
| hz4_alloc_tls_get() 削減効果 | +0.0~0.1% |
| **合計** | **-0.7%** |

**根本原因**:
1. `hz4_alloc_tls_get()` は `__builtin_expect` で既に最適化済み（overhead ≈ 0）
2. bins[128] (2KB) を先頭に配置 → carry[] が 3倍遠方化
3. static `__thread` は kernel/CPU が L1 prefetch するため分離が有利

### 教訓
- 冗長に見える indirection もコンパイラ最適化の対象
- static `__thread` の特性（prefetch）を理解する
- 2KB+ の struct merge は cache line hierarchy を検討必須
- "命令数削減" ≠ "性能向上"

---

## 共通の教訓

### 1. 測定なき最適化は最適化にあらず
3 つとも理論的には正しそうだったが、実測で退行。

### 2. アーキテクチャ適合性
他プロジェクト（hz3）の成功パターンは hz4 に直接移植できない。
データ構造の違いを理解してから適用する。

### 3. "既存が最適" の可能性
- (owner, sc) bucketing は偶然 near-optimal
- 分離 TLS は CPU prefetch の恩恵を受けていた

### 4. Cache が支配する
- PAGE_TAG_TABLE: +18-22 cycles overhead
- PAGE_BUCKET: page header eviction
- TLS_MERGE: carry[] 遠方化

すべて cache/memory hierarchy が原因。

### 5. NO-GO は価値ある知見
- "やらない理由" が明確になる
- 将来の無駄な試行を防ぐ
- 設計判断の根拠になる

---

## 今後の方向性

Phase 6 の結果から、hz4 の現状は：
- local path は既に高度に最適化されている
- macro-level の設計変更なしに +3% 以上は困難
- hz4/hz3 ratio 0.85 は妥当な到達点の可能性

次のアプローチ候補：
1. **remote path の改善** - R=90 での性能向上
2. **安定化・品質向上** - correctness, edge cases
3. **別ワークロードでの評価** - mimalloc-bench subset

---

## References

- `HZ4_PHASE6_PAGE_TAG_TABLE_WORK_ORDER.md`
- `HZ4_PHASE6_REMOTE_FLUSH_PAGE_BUCKET_WORK_ORDER.md`
- `HZ4_PHASE6_TLS_STRUCT_MERGE_WORK_ORDER.md`
