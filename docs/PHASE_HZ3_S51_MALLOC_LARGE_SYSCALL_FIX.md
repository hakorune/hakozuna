# PHASE_HZ3_S51: malloc-large syscall削減（LargeCacheBox: cap + evict + （任意で）madvise）

目的:
- `mimalloc-bench/bench/malloc-large`（5–25MiB, live<=20）で hz3 が負ける原因（`munmap/mmap` 多発）を潰す。
- small/medium（16–32768）の hot path には固定費を入れない（large box 内だけで完結）。

結論:
- **S51: GO**（`munmap` をほぼゼロにして `malloc-large` を大幅改善）。

---

## 背景（症状）

`malloc-large` はサイズが 5–25MiB で多様なため、large cache が刺さらないと `mmap/munmap` が連打される。

- before: hz3 が `munmap` を大量に呼び、syscall/fault が支配。
- after: cap を大きくして eviction を抑え、`munmap` をほぼゼロへ。
  - `madvise(MADV_DONTNEED)` は **任意**（RSS/省メモリ寄り）。speed-first では OFF 推奨。

---

## 変更点（箱の境界）

対象は `hz3_large` のみ:
- `hakozuna/hz3/src/hz3_large.c`

compile-time flags:
- `HZ3_LARGE_CACHE_MAX_BYTES=(8192ULL<<20)`（8GiB）
  - 512MiB cap だと eviction が多すぎるため、`malloc-large` 向けに headroom を確保
- `HZ3_S50_LARGE_SCACHE_EVICT=1`
  - cap 超過時の eviction を有効化
  - S51: **同一 size class を優先して evict**（hit率維持）
- `HZ3_S51_LARGE_MADVISE=0/1`
  - `1` のとき large cache push 時に `madvise(MADV_DONTNEED)`（ページ境界に丸めて適用）
  - RSS/省メモリ寄り。speed-first では `0` 推奨（再利用時の page fault を避けるため）

scale lane では上記（`MAX_BYTES=8GiB`, `EVICT=1`, `MADVISE=0`）を `hakozuna/hz3/Makefile` で既定化。

---

## 計測（観測のみ）

### ベンチ
- `mimalloc-bench/bench/malloc-large/malloc-large`

### syscall支配の確認（strace）
例:
```bash
make -C hakozuna/hz3 clean all_ldpreload_scale
env -u LD_PRELOAD LD_PRELOAD=./libhakozuna_hz3_ldpreload.so \
  strace -f -c -o large_hz3.strace \
  ./mimalloc-bench/bench/malloc-large/malloc-large >/dev/null
```

### CPU/カーネル支配の確認（perf stat）
```bash
env -u LD_PRELOAD LD_PRELOAD=./libhakozuna_hz3_ldpreload.so \
  perf stat -d -r 5 -- \
  ./mimalloc-bench/bench/malloc-large/malloc-large >/dev/null
```

---

## 結果（S51）

### syscall（例）

| 項目 | before | after | 変化 |
|------|--------|-------|------|
| munmap | 979 | 2 | -99.8% |
| mmap | 1061 | 273 | -74% |
| madvise | 0 | 0 | speed-first（`HZ3_S51_LARGE_MADVISE=0`） |

### 実行時間（例）

| Allocator | Time(s) |
|-----------|---------|
| tcmalloc | 1.27 |
| system | 1.99 |
| hz3 (S51) | 2.61 |
| mimalloc | 3.39 |

---

## 追加メモ（tcmalloc との差分）

- `strace -c` では hz3 の syscall 時間は小さいが、tcmalloc の方が総時間は短い。
- `perf` では `memset` が支配的（ベンチ由来）で、hz3 は `memset` 以外のサイクルがまだ重い。
  - `mmap` 回数（hz3 273 vs tcmalloc 36）が page fault/TLB の差になっている可能性がある。

---

## 注意

- `HZ3_LARGE_CACHE_MAX_BYTES=8GB` は「malloc-large 対策の実験レーン」向け。メモリ上限や運用条件に応じて tighten する。
- `HZ3_S51_LARGE_MADVISE=1` は RSS を抑える代わりに、再利用時の page fault が増える可能性がある（speed-first では OFF 推奨）。
