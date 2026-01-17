# S110 v2: Free Fast Path最適化（PTAG32強化）

## 目的

hz3_free（14.15% overhead）を最適化し、mixed uniformのgap（-6.4%）を削減する。

**重要**: PTAG32を**置き換えるのではなく強化する**（過去NO-GO教訓）

---

## 現状分析（2026-01-17）

### Perf Profile

| 関数 | Overhead |
|------|----------|
| **hz3_free** | **14.15%** ← 最大 |
| hz3_malloc | 9.28% |
| hz3_sub4k_alloc | 2.21%（S138のみ） |

### Gap測定

| Workload | Perf-First | Mem-First | Gap |
|----------|------------|-----------|-----|
| dist_app | 70.05M | 69.30M | -1.1% |
| mixed uniform | 74.57M | 69.80M | **-6.4%** |

---

## 過去NO-GO教訓

### S113: SegMath（-1.5%、NO-GO）

**失敗理由**:
- Arena base load + math + meta load（2-3 loads + 演算）
- PTAG32（2 loads + bit extract）より**構造的に遅い**

**教訓**: **Arena base loadは重い。PTAG32は極めて速い。**

### S16-2D: FastPath Shape Split（+1.68%だがNO-GO）

**失敗理由**:
- Throughput +1.68%だが**instructions +3.4%増加**
- 関数呼び出しオーバーヘッド > spill削減効果

**教訓**: **関数境界追加は逆効果。Inline展開の方が有効。**

### S16-2C: Lifetime Split（-1.2%、NO-GO）

**教訓**: Lifetime操作だけでは改善しない。

### S140: PTAG32 leaf micro-opt（NO-GO）

**試行**:
- A1: `HZ3_S140_PTAG32_DECODE_FUSED`（`tag32 - 0x00010001` で bin/dst の -1 を融合）
- A2: `HZ3_S140_EXPECT_REMOTE`（local/remote 分岐反転 + `__builtin_expect`）

**結果**:
- r90 で最大 **-14.39%**（remote 比率が高いほど退行）

**失敗理由**:
- A1: 命令数は減ったが、`sub` の結果待ちで extract が直列化し **ILP が崩れて IPC が低下**（依存チェーン化）
- A2: branch-miss が増えるケースがあり狙いと逆（remote-heavyでも予測が悪化）

**教訓**: PTAG32 leaf は “命令数” より **ILP/分岐安定性**が支配しやすい。軽率な decode 融合・ヒント反転は危険。

---

## 設計方針

### コンセプト: "PTAG32を最適化する"

1. ✅ PTAG32 lookupは維持（極小コスト）
2. ✅ Dispatch後の処理を最適化
3. ✅ Leaf化でcallee-saved regs削減
4. ❌ Arena base loadは避ける（重い）
5. ❌ 関数境界追加は避ける（instructions増加）

---

## 最適化案

### A1) PTAG32 TLS Cache（S114延長）

**狙い**: arena_base/ptag32_baseのatomic load削減

**実装**:
```c
// TLSにsnapshot保持
struct Hz3TCache {
    void* arena_base_snapshot;
    uint32_t* ptag32_base_snapshot;
    // ...
};
```

**期待効果**: +1～2%

**リスク**: TLS初期化コスト

**関連**: `HZ3_S114_PTAG32_TLS_CACHE=1`（既存実装あり）

### A2) Tag Layout最適化

**狙い**: bin/dst/kindを1 load + bit操作で取得

⚠️ 注意: S140 の結果より、leaf の “見かけの命令数削減” が ILP 崩壊で負けるケースがある。
bitfield/union による自動最適化は **コンパイラ依存**かつ意図通りの命令列になりにくいので、基本は shift/mask のまま評価する。

**現状（推測）**:
```c
uint8_t kind = (tag >> 24) & 0xFF;  // 個別関数呼び出し
uint16_t bin = (tag >> 10) & 0x3FFF;
uint8_t dst = tag & 0x3F;
```

**最適化案**:
```c
// 1 loadで全部取得（union/bitfield）
union Tag32 {
    uint32_t raw;
    struct {
        uint8_t dst : 6;
        uint16_t bin : 14;
        uint8_t kind : 8;
        // ...
    } fields;
};
```

**期待効果**: +1～2%（bit操作削減）

**リスク**: コンパイラ依存

### A3) Remote Check早期化 ← **推奨1位**

**狙い**: local fast pathをleaf化

**現状（推測）**:
```c
void hz3_free(void* ptr) {
    uint32_t tag = load_ptag32(ptr);
    uint8_t kind = tag_kind(tag);  // 分岐
    switch(kind) {
        case SMALL_V2:
            uint8_t dst = tag_dst(tag);
            if (dst != my_shard) {  // 深い
                remote_free(...);
            } else {
                local_push(...);
            }
    }
}
```

**最適化案**:
```c
void hz3_free(void* ptr) {
    uint32_t tag = load_ptag32_fast(ptr);

    // Early remote check
    uint8_t dst = tag & 0x3F;
    if (unlikely(dst != my_shard)) {
        hz3_free_remote_cold(ptr, tag);
        return;  // ← leaf化可能
    }

    // Local fast path（最小、leaf化）
    uint16_t bin = (tag >> 6) & 0x3FFF;
    hz3_bin_push_local_inline(bin, ptr);
}
```

**期待効果**: +2～3%（local pathをleaf化）

**利点**:
- Callee-saved regs不要（leaf関数）
- Prolog/epilog削減
- Local pathが極小

**リスク**: Remote頻度が高いと逆効果

### A4) Bin Push Inline化

**狙い**: 関数呼び出しオーバーヘッド削減

**現状**:
```c
hz3_bin_push_local(bin, ptr);  // 関数呼び出し
```

**最適化案**:
```c
static inline void hz3_bin_push_local_inline(uint16_t bin, void* ptr) {
    Hz3TCache* tc = hz3_tcache_get();
    Hz3Bin* b = &tc->bins[bin];
    b->head = ptr;
    *(void**)ptr = b->next;
    b->next = ptr;
    b->count++;
}
```

**期待効果**: +1～2%

**リスク**: コード肥大化

---

## 実装フェーズ

### Phase 1: 観測（S110-0）

**目的**: 現状のコスト定量化

**実装**:
- PTAG32 load回数
- Arena base atomic load回数
- Tag bit操作回数

**フラグ**: `HZ3_S110_STATS=1`

**判断**: これらのコストが3%以上なら最適化価値あり

### Phase 2: Low-hanging Fruit

**優先順位**:
1. **A3: Remote Check早期化**（リスク低、効果大）
2. **A2: Tag Layout最適化**（ビット操作削減）
3. **A4: Bin Push Inline化**（call削減）

**実装**:
- 各1本ずつA/B測定
- RUNS=10 median比較
- Instructions確認（perf stat）

### Phase 3: 高リスク施策（条件付き）

**条件**: Phase 2で+3%未達成の場合のみ

**施策**: A1: PTAG32 TLS Cache（S114実装あり）

---

## フラグ設計

```c
// hakozuna/hz3/include/hz3_config.h

// Phase 1: 観測
#ifndef HZ3_S110_STATS
#define HZ3_S110_STATS 0  // atexit 1-shot stats
#endif

// Phase 2: 最適化
#ifndef HZ3_S110_REMOTE_CHECK_EARLY
#define HZ3_S110_REMOTE_CHECK_EARLY 0  // A3: early remote check
#endif

#ifndef HZ3_S110_TAG_LAYOUT_V2
#define HZ3_S110_TAG_LAYOUT_V2 0  // A2: union/bitfield
#endif

#ifndef HZ3_S110_BIN_PUSH_INLINE
#define HZ3_S110_BIN_PUSH_INLINE 0  // A4: inline bin push
#endif

// Phase 3: 高リスク
// (A1はS114として既存)
```

---

## GO/NO-GO基準

### GO条件（すべて満たす）

1. **Throughput +3%以上**（mixed uniform、RUNS=10 median）
2. **Instructions減少または同等**（perf stat）
3. **RSS退行 ≤+5%**

### NO-GO条件（いずれか）

1. **Throughput +1%未満**（測定誤差レベル）
2. **Instructions +2%以上増加**
3. **Maintainability大幅低下**

---

## 測定プロトコル

### SSOT（RUNS=10）

```bash
# Baseline
LD_PRELOAD=./libhakozuna_hz3_scale.so \
  ./hakozuna/out/bench_random_mixed_malloc_dist 20000000 400 16 32768 0x12345678 uniform

# Treat（例: A3）
make -C hakozuna/hz3 clean all_ldpreload_scale \
  HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_S110_REMOTE_CHECK_EARLY=1'

LD_PRELOAD=./libhakozuna_hz3_scale.so \
  ./hakozuna/out/bench_random_mixed_malloc_dist 20000000 400 16 32768 0x12345678 uniform
```

### Instructions確認

```bash
perf stat -e instructions,cycles,branches,branch-misses \
  LD_PRELOAD=./libhakozuna_hz3_scale.so \
  ./hakozuna/out/bench_random_mixed_malloc_dist 20000000 400 16 32768 0x12345678 uniform
```

### Objdump確認

```bash
objdump -d libhakozuna_hz3_scale.so | grep -A 50 "<hz3_free>:" > /tmp/hz3_free_asm.txt
# Check: callee-saved push/pop, stack spills
```

---

## 実装ファイル

### 変更対象

1. **`hakozuna/hz3/src/hz3_hot.c`**（line ~100-200）
   - `hz3_free()` 本体
   - 境界: 1箇所のみ

2. **`hakozuna/hz3/include/hz3_tag.h`**（A2実装時）
   - Tag layout変更

3. **`hakozuna/hz3/include/hz3_bin.h`**（A4実装時）
   - Bin push inline定義

### 新規ファイル

- なし（既存ファイルのみ変更）

---

## リスク分析

### R1: Remote頻度が高い場合

**リスク**: A3（early remote check）でlocal pathが小さくなりすぎて逆効果

**緩和策**:
- SSOT測定でremote/local比率を確認
- Remote頻度 >30%なら別アプローチ

### R2: Compiler依存性

**リスク**: A2（union/bitfield）がコンパイラで挙動が変わる

**緩和策**:
- `_Static_assert` で layout検証
- objdump確認

### R3: Inline肥大化

**リスク**: A4（bin push inline）でコード肥大化

**緩和策**:
- I-cache miss測定（perf stat）
- 肥大化が2KB超えたらNO-GO

---

## 成功メトリクス

**定量的**:
- ✅ Mixed uniform +3%以上
- ✅ Instructions減少または同等
- ✅ RSS ≤+5%

**定性的**:
- ✅ Local fast pathがleaf化
- ✅ Callee-saved regs削減
- ✅ Maintainability維持

---

## References

- S113 NO-GO: `hakozuna/hz3/docs/PHASE_HZ3_S113_FREE_SEGMATH_META_FASTDISPATCH_WORK_ORDER.md`
- S16-2D NO-GO: `hakozuna/hz3/docs/PHASE_HZ3_S16_2D_FREE_FASTPATH_SHAPE_WORK_ORDER.md`
- S114 TLS Cache: `hakozuna/hz3/docs/BUILD_FLAGS_INDEX.md` (line 100+)
- Gap測定: `/tmp/s138_next_design_candidates.md`

---

**Date**: 2026-01-17
**Status**: 設計完了、実装待ち
**Phase**: S110 v2（PTAG32強化）
**Key Insight**: Arena base loadは重い、PTAG32は速い、関数境界は逆効果
**推奨実装順**: A3（early remote check）→ A2（tag layout）→ A4（inline）
