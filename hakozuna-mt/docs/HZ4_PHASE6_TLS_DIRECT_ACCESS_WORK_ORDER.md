# HZ4 Phase 6: TLS Direct Access Work Order

目的:
- `hz4_tls_get@plt` の PLT overhead (~6%) を削減
- hz4/hz3 ratio を改善

---

## 0) ルール（Box Theory）

- 変更は **TLS Box** 内で完結
- A/B は compile-time で切替可能（`HZ4_TLS_DIRECT`）

---

## 1) 観測されたボトルネック（perf 分析）

| 項目 | hz4 | hz3 | 差分 |
|------|-----|-----|------|
| TLS 取得 | `hz4_tls_get@plt` | 直接 `%fs` | **~6%** overhead |
| 命令数 | 3.70B | 2.92B | +27% |
| L1 miss | 58.9M | 38.2M | +54% |
| IPC | 1.09 | 1.27 | -14% |

---

## 2) 施策

### A) HZ4_TLS_DIRECT=1

- TLS 変数を extern 宣言してヘッダで inline getter を提供
- slow path (init) は noinline で分離
- `__builtin_expect` で init check を cold path へ

---

## 3) 変更箇所

| ファイル | 変更内容 |
|----------|----------|
| `core/hz4_config.h` | `HZ4_TLS_DIRECT` フラグ追加 |
| `include/hz4_tls_init.h` | extern 宣言 + inline `hz4_tls_get()` + noinline attr |
| `src/hz4_tls_init.c` | 変数名変更 + slow path 関数追加 |

---

## 4) 実装詳細

### include/hz4_tls_init.h

```c
#if HZ4_TLS_DIRECT
extern __thread hz4_tls_t g_hz4_tls;
extern __thread uint8_t g_hz4_tls_inited;

void hz4_tls_init_slow(void) __attribute__((noinline));

static inline hz4_tls_t* hz4_tls_get(void) {
    if (__builtin_expect(!g_hz4_tls_inited, 0)) {
        hz4_tls_init_slow();
    }
    return &g_hz4_tls;
}
#else
hz4_tls_t* hz4_tls_get(void);  // Legacy: PLT 経由
#endif
```

### src/hz4_tls_init.c

```c
#if HZ4_TLS_DIRECT
__thread hz4_tls_t g_hz4_tls;
__thread uint8_t g_hz4_tls_inited = 0;
static _Atomic(uint16_t) g_hz4_tid_next = 1;

void hz4_tls_init_slow(void) {
    uint16_t tid = atomic_fetch_add_explicit(&g_hz4_tid_next, 1, memory_order_relaxed);
    hz4_tls_init(&g_hz4_tls, tid);
    g_hz4_tls_inited = 1;
}
#else
// Legacy 実装
__thread hz4_tls_t tls;
static __thread uint8_t g_hz4_tls_inited = 0;
static _Atomic(uint16_t) g_hz4_tid_next = 1;

hz4_tls_t* hz4_tls_get(void) {
    if (!g_hz4_tls_inited) {
        uint16_t tid = atomic_fetch_add_explicit(&g_hz4_tid_next, 1, memory_order_relaxed);
        hz4_tls_init(&tls, tid);
        g_hz4_tls_inited = 1;
    }
    return &tls;
}
#endif
```

---

## 5) SSOT ベンチ手順

```bash
# Baseline (TLS_DIRECT=0)
make -C hakozuna/hz4 clean all CFLAGS="-O2 -Wall -Wextra -Werror -std=c11 -DHZ4_TLS_DIRECT=0"

# A: TLS_DIRECT=1
make -C hakozuna/hz4 clean all

# Benchmark (R=0, 16-2048, x3 median)
ITERS=20000000 WS=400 MIN=16 MAX=2048
LD_PRELOAD=./hakozuna/hz4/libhakozuna_hz4.so \
  ./hakozuna/out/bench_random_mixed_malloc_args $ITERS $WS $MIN $MAX
```

---

## 6) NO-GO 判定

- **+6% 未満** → NO-GO（default OFF 維持）
- correctness 低下 → 即撤退

---

## 7) 結果（SSOT, 2026-01-23）

### ベンチ結果 (R=0, 16-2048, x3 median)

| Mode | ops/s | vs Baseline | vs hz3 |
|------|-------|-------------|--------|
| Baseline (TLS_DIRECT=0) | ~86.8M | - | 0.79 |
| TLS_DIRECT=1 | ~94.4M | **+8.8%** | 0.86 |
| hz3 | ~110M | - | - |

### 評価

- **+8.8%** で +6% 目標達成 → **GO**
- hz4/hz3 ratio: 0.79 → 0.86 (+9% 改善)
- PLT overhead 削減が効いている

### 判定

**GO** → default ON に切替

### 実装した変更

- `HZ4_TLS_DIRECT=1` (default ON)
- `extern __thread` で TLS 変数を公開
- `hz4_tls_init_slow()` を noinline で分離
- `hz4_tls_get()` を inline 化（direct `%fs` access）

---

## 8) 次の優先案

1. **Page Tag Table** 導入（高 / +9%）
   - magic routing の 3 回比較を 1 回に
2. **TLS 構造統合**（中 / +3%）
   - `hz4_alloc_tls_t` を `hz4_tls_t` に統合
