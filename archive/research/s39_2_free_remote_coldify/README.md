# S39-2: hz3_free remote 経路 cold 化 - NO-GO

## 概要

`hz3_free` の remote 経路（`dst != my_shard`）を `__attribute__((noinline, cold))` helper に隔離し、local hot path から bank lookup 計算を追放する試み。

## 結果: NO-GO

全カテゴリで退行が観測された。

## A/B 結果（RUNS=30 median）

| Category | Baseline (COLDIFY=0) | Treatment (COLDIFY=1) | Delta |
|----------|---------------------|----------------------|-------|
| dist=app | 32.35M | 31.54M | **-2.48%** |
| tiny | 101.26M | 98.57M | **-2.66%** |
| medium | 11.81M | 11.71M | **-0.85%** |
| medium_tiny | 100.39M | 101.14M | +0.75% |
| mixed | 13.65M | 13.50M | **-1.10%** |
| mixed_tiny | 97.28M | 97.84M | +0.58% |

## GO/NO-GO 判定

- **GO 基準**: mixed/medium +1% 以上、dist=app 退行なし
- **結果**: NO-GO
  - mixed: -1.10% < +1%
  - medium: -0.85% < +1%
  - dist=app: -2.48% (退行)

## 仮説と現実

### 仮説
- compiler が `dst==my_shard` の分岐に対して bank 側のアドレス計算を hot 側に残している
- remote 経路を noinline+cold helper に隔離すれば local hot から bank lookup 計算を追放できる

### objdump 確認結果（分岐は成功）
```
hz3_free hot path:
  2d5a: cmp %bl,%fs:0x4c85(%r12)   # my_shard comparison
  2d63: jne 2076 <hz3_free.cold>   # remote → cold section

hz3_free_remote_push (cold):
  2046: imul   $0x88,%rdi,%rdi    # bank offset calculation
  204d: lea    0x88(%rsi,%rdi,1),%rax
```

bank 計算は cold section に移動した。

### 退行の原因（推測）
1. **call overhead**: noinline helper への call/ret が ST でも発生するケースがあった可能性
2. **branch misprediction**: `__builtin_expect` だけでは分岐予測が改善せず
3. **register pressure**: hot path での追加の引数保存が発生した可能性
4. **compiler optimization interference**: cold 属性が他の最適化を阻害した可能性

## 実装内容

### hz3_config.h (追加)
```c
// S39-2: hz3_free の remote 経路を cold 化
#ifndef HZ3_FREE_REMOTE_COLDIFY
#define HZ3_FREE_REMOTE_COLDIFY 0
#endif
```

### hz3_hot.c (追加)
```c
#if HZ3_FREE_REMOTE_COLDIFY && HZ3_LOCAL_BINS_SPLIT
__attribute__((noinline, cold))
static void hz3_free_remote_push(uint8_t dst, uint32_t bin, void* ptr) {
    hz3_bin_push(hz3_tcache_get_bank_bin(dst, bin), ptr);
}
#endif
```

### hz3_free() 内の変更（4箇所）
```c
#if HZ3_LOCAL_BINS_SPLIT
    if (__builtin_expect(dst == t_hz3_cache.my_shard, 1)) {
        hz3_bin_push(hz3_tcache_get_local_bin_from_bin_index(bin), ptr);
    } else {
#if HZ3_FREE_REMOTE_COLDIFY
        hz3_free_remote_push(dst, bin, ptr);
#else
        hz3_bin_push(hz3_tcache_get_bank_bin(dst, bin), ptr);
#endif
    }
#else
    hz3_bin_push(hz3_tcache_get_bank_bin(dst, bin), ptr);
#endif
```

## ログ

- Baseline: `/tmp/hz3_s39_2_baseline_runs30.txt`
- Treatment: `/tmp/hz3_s39_2_treatment_runs30.txt`
- Log directory: `/tmp/hz3_ssot_90219bb16_20260103_093144` (baseline), `/tmp/hz3_ssot_90219bb16_20260103_093410` (treatment)

## Rollback

`HZ3_FREE_REMOTE_COLDIFY=0`（default）のまま。実装コードは残すが、有効化しない。

## 日付

2026-01-03
