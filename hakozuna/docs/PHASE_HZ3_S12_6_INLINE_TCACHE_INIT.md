# PHASE_HZ3_S12-6: tcache init guard の inline 化（small の固定費削減）

目的:
- small(16–2048) / mixed(16–32768) の `hz3_tcache_ensure_init()` 固定費を削る。
- 箱理論の “人間の境界” は維持しつつ、プログラム境界（call/stack setup）を減らす。

---

## 変更概要

- `hz3_tcache_ensure_init()` を header 側で `static inline` の fast guard にし、
  slow 側を `hz3_tcache_ensure_init_slow()` に分離した。

期待:
- `free/malloc` の hot path で “初期化済みチェック” が call にならず、分岐1回で抜ける。

---

## ベンチ（SSOT-HZ3）

ベンチ:
- `hakozuna/hz3/scripts/run_bench_hz3_ssot.sh`

注意:
- hz3 の Makefile はヘッダ依存の `.d` を生成していないため、ヘッダ変更が “再ビルドされずに取り違え” になりやすい。
- そのため SSOT スクリプトは `HZ3_CLEAN=1`（既定）で `clean all_ldpreload` を行う。

再測定（RUNS=10 median）:
- ログ: `/tmp/hz3_ssot_7b02434b8_20260101_060336`
- small: hz3 `94.33M` / tcmalloc `95.34M`
- medium: hz3 `106.97M` / tcmalloc `116.99M`
- mixed: hz3 `98.75M` / tcmalloc `114.18M`

---

## 変更ファイル

- `hakozuna/hz3/include/hz3_tcache.h`
- `hakozuna/hz3/src/hz3_tcache.c`
- `hakozuna/hz3/scripts/run_bench_hz3_ssot.sh`（`HZ3_CLEAN=1`）

