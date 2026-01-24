# S38: `bin->count--`（RMW）固定費の低減 — `uint16_t`→`uint32_t` A/B

目的:
- S37 perf で `hz3_malloc` の `bin->count--`（`subw $0x1, 0x8(%rdx)`）が目立つ。
- hot path の層を増やさずに、**同じ動作のまま**コード生成（命令形/依存）を改善できるか A/B する。

仮説:
- `uint16_t` の `subw mem` より、`uint32_t` の `subl mem` の方が CPU/コード生成的に有利な可能性がある。
- `Hz3Bin` は現状でも 16B アライン/パディングがあるため、`count` を `uint32_t` にしても struct サイズ増は起きにくい（= TLS肥大リスクが小さい）。

非目標:
- count を消す／近似化する（正しさを変える）のはこのフェーズではやらない。

参照:
- S37: `hakozuna/hz3/docs/PHASE_HZ3_S37_POST_S36_GAP_REFRESH_WORK_ORDER.md`
- SSOT index: `hakozuna/hz3/docs/BUILD_FLAGS_INDEX.md`

---

## Step 1: A/B ゲートを入れる（実装）

対象:
- `hakozuna/hz3/include/hz3_config.h`
- `hakozuna/hz3/include/hz3_types.h`（`Hz3Bin.count` の型）
- `hakozuna/hz3/include/hz3_tcache.h`（push/pop の `count++/--`）
- `hakozuna/hz3/src/*`（`bin->count` を扱う箇所の型整合）

設計（例）:

```c
// hz3_config.h
// New preferred entrypoint:
#ifndef HZ3_BIN_COUNT_POLICY
#define HZ3_BIN_COUNT_POLICY 0
#endif

// Legacy (still supported, normalized by hz3_config.h):
#ifndef HZ3_BIN_COUNT_U32
#define HZ3_BIN_COUNT_U32 0
#endif

// hz3_types.h
#if HZ3_BIN_COUNT_U32
typedef uint32_t Hz3BinCount;
#else
typedef uint16_t Hz3BinCount;
#endif

typedef struct {
    void*       head;
    Hz3BinCount count;
} Hz3Bin;
```

注意:
- `Hz3Bin` のサイズ/アラインが変わらないこと（`static_assert(sizeof(Hz3Bin) == 16)` を入れても良い）
- `bin->count` を `uint32_t` で扱う経路で、`-Wsign-conversion` 的な問題が出ないようにキャストを整理

---

## Step 2: SSOT（RUNS=30, seed固定）で A/B

### baseline（U32=0）

```bash
make -C hakozuna bench_malloc_dist
RUNS=30 ITERS=20000000 WS=400 \
  HZ3_LDPRELOAD_DEFS="$(make -C hakozuna/hz3 -s print-ldpreload-defs 2>/dev/null || true)" \
  ./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh
```

※上は概念。実際は `hakozuna/hz3/Makefile` の既定 `HZ3_LDPRELOAD_DEFS` を使って良い。
このフェーズでは **`-DHZ3_BIN_COUNT_POLICY=0/1` だけ差し替える**（推奨）。

### treatment（U32=1）

```bash
RUNS=30 ITERS=20000000 WS=400 \
  HZ3_LDPRELOAD_DEFS_EXTRA="-DHZ3_BIN_COUNT_POLICY=1" \
  ./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh
```

測るもの（S37と同じ）:
- dist=app（`bench_random_mixed_malloc_dist`, `BENCH_EXTRA_ARGS="0x12345678 app"`）
- uniform（`bench_random_mixed_malloc_args`）
- tiny-only（`trimix 16 256 100 ...`）

---

## Step 3: objdump 確認（必須）

狙い:
- `hz3_bin_pop()` 相当の命令が `subw` → `subl` などに変わっているか

例:
```bash
objdump -d --no-show-raw-insn ./libhakozuna_hz3_ldpreload.so | rg -n "hz3_bin_pop|subw|subl"
```

---

## GO/NO-GO

GO（目安）:
- tiny-only（RUNS=30）で +1% 以上（目安）
- dist=app / uniform は退行なし（±1%）

NO-GO:
- tiny-only が動かない（±1%）または退行
- dist=app/uniform が -1% 以上退行

NO-GO の場合:
- `hakozuna/hz3/archive/research/s38_bin_count_u32/README.md` にログと結論を固定
- `hakozuna/hz3/docs/NO_GO_SUMMARY.md` に追記

---

## 実測（2026-01-03）

結論（当時）: **Conditional KEEP**（既定は `HZ3_BIN_COUNT_U32=0` のまま）

理由:
- uniform SSOT（args）は medium/mixed が改善したが、
- dist=app（実アプリ寄り）で hz3 が微減し、tiny-only で退行したため。

詳細ログと表:
- `hakozuna/hz3/archive/research/s38_2_bin_count_u32/README.md`

追記（2026-01-03）:
- S38-4 で tiny-only の退行は再現しなかった（`hakozuna/hz3/archive/research/s38_4_u32_tiny_regression_rootcause/README.md`）。
- 現在の hz3 lane 既定は `HZ3_BIN_COUNT_POLICY=1`（Makefile）で運用。
