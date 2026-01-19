# S16-2C: Tag Decode Lifetime Split - NO-GO

## 概要

`hz3_free` の tag decode を分割して `sc/owner` の live range を短くし、レジスタ圧を低減する試み。

## 仮説

現状:
```c
hz3_pagetag_decode_with_kind(tag, &sc, &owner, &kind);
if (kind == PTAG_KIND_V2) { /* use sc, owner */ }
if (kind == PTAG_KIND_V1_MEDIUM) { /* use sc, owner */ }
```

`sc/owner` が kind 判定前に確定し、分岐を跨いで生存 → spill の温床。

対策:
```c
int kind = tag >> 14;
if (kind == PTAG_KIND_V2) {
    int sc = (tag & 0xFF) - 1;
    int owner = ((tag >> 8) & 0x3F) - 1;
    /* use sc, owner */
}
if (kind == PTAG_KIND_V1_MEDIUM) {
    int sc = (tag & 0xFF) - 1;
    int owner = ((tag >> 8) & 0x3F) - 1;
    /* use sc, owner */
}
```

## 結果

### SSOT (RUNS=10, mixed 16-32768)

| 指標 | baseline | S16-2C | 変化 |
|------|----------|--------|------|
| mixed median | 91.3M | 90.2M | **-1.2%** |

### perf stat

| 指標 | baseline | S16-2C | 変化 |
|------|----------|--------|------|
| instructions | 1,408M | 1,447M | **+2.8%** |
| IPC | 1.70 | 1.72 | +1.2% |

### objdump

- hz3_free: 131 instructions
- 4 pushes (r13, r12, rbp, rbx) - baseline と同じ
- decode は分岐の内側に移動したが、レジスタ割り当ては変わらず

## 分析

### なぜ効かなかったか

1. **callee-saved registers**: r12/r13 は callee-saved なので、関数エントリで push される
2. **関数レベルのレジスタ割り当て**: GCC は関数全体を見てレジスタを割り当てる
3. **ソースの lifetime ≠ 生成コードの lifetime**: 分岐の内側に変数を移しても、コンパイラが同じレジスタを使うなら push/pop は変わらない

### 根本的な問題

S16-2 (bit layout 変更) と S16-2C (lifetime 分割) の両方が失敗した理由は同じ:
- 命令数削減はソースレベルの「式」や「スコープ」ではなく、コンパイラのレジスタ割り当てで決まる
- callee-saved registers (r12-r15) を避けるには、変数を減らすかインライン展開を変えるしかない

## 判定

**NO-GO**
- mixed: -1.2% (±1% 内)
- instructions: +2.8% (増加)

## 参照

- Work Order: `hakozuna/hz3/docs/PHASE_HZ3_S16_2C_TAG_DECODE_LIFETIME_WORK_ORDER.md`
- S16-2 (bit layout): `hakozuna/hz3/archive/research/s16_2_ptag_layout_v2/README.md`
