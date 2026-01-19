# S16-2: PTAG Layout V2 (0-based encoding) - NO-GO

## 概要

PageTag encoding を 0-based に変更し、decode 時の `-1` 演算を削除して命令数を削減する試み。

## 変更内容

### hz3_config.h
```c
#ifndef HZ3_PTAG_LAYOUT_V2
#define HZ3_PTAG_LAYOUT_V2 0
#endif
```

### hz3_arena.h (HZ3_PTAG_LAYOUT_V2=1 時)
```c
// 0-based encoding: sc/owner を直接格納
static inline void hz3_pagetag_decode_with_kind(uint16_t tag, int* sc, int* owner, int* kind) {
    *kind = tag >> 14;           // -1 なし
    *owner = (tag >> 8) & 0x3F;  // -1 なし
    *sc = tag & 0xFF;            // -1 なし
}
```

### 期待効果
- decode 時の `-1` 演算 3個削除
- 特に mixed (16-32768) で効果大

## 結果

### SSOT (RUNS=10, mixed 16-32768)

| 指標 | baseline | S16-2 | 変化 |
|------|----------|-------|------|
| mixed median | 91.3M | 91.1M | **-0.2%** |

### perf stat

| 指標 | baseline | S16-2 | 変化 |
|------|----------|-------|------|
| instructions | 1,408M | 1,409M | +0.07% |

### objdump

- hz3_free: baseline 2 pushes → S16-2 4 pushes (r13, r12, rbp, rbx)
- コンパイラがレジスタ割り当てを変更し、callee-saved registers を増やした
- -1 削除の効果がレジスタ save/restore で相殺

## 分析

### なぜ効かなかったか

1. **レジスタ割り当ての変化**: 式を軽くしたが、コンパイラが異なるレジスタ戦略を選択
2. **callee-saved registers 増加**: r12/r13 の使用で push/pop が増加
3. **net effect ゼロ**: -1 削除の効果と push/pop 増加が相殺

### 教訓

- 「式を軽くすれば命令数が減る」という仮説は成り立たない
- レジスタ割り当ては式の複雑さより変数の数や live range で決まる
- コンパイラの生成コードを見ずに最適化を議論するのは無意味

## 判定

**NO-GO**
- mixed: -0.2% (±1% 内)
- 命令数: 同等（+0.07%）
- objdump: レジスタ圧増加

## コード保持

`hz3_arena.h` に `#if HZ3_PTAG_LAYOUT_V2` ブロックとして残してある（default=0）。
将来の最適化で layout 変更と他の変更を組み合わせる場合に再利用可能。

## 参照

- Work Order: `hakozuna/hz3/docs/PHASE_HZ3_S16_MIXED_INSN_REDUCTION_WORK_ORDER.md`
