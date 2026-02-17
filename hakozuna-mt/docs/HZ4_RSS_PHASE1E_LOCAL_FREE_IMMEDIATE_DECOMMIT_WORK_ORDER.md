# HZ4 RSS Phase 1E: Local‑Free Immediate Decommit

目的:
- `used_count==0` を **local free の場で即 decommit** して発火を確認する。
- Phase 1D の「notify だけでは発火しない」問題を解消する。

非目的:
- hot path の恒久チューニング（実験用フラグ）。

---

## Box Theory
- 変更は **TCache Box の owner free 境界**だけ。
- decommit 本体は **OS Box** のまま。
- **フラグで切替可能**（即戻せる）。

---

## 1) ビルド（即時 decommit ON）

```sh
make -C hakozuna/hz4 clean all \
  HZ4_DEFS_EXTRA='-DHZ4_OS_STATS=1 -DHZ4_PAGE_META_SEPARATE=1 -DHZ4_PAGE_DECOMMIT=1 -DHZ4_LOCAL_FREE_DECOMMIT=1'
```

---

## 2) 実行（R=0 / R=50）

```sh
env LD_PRELOAD=./hakozuna/hz4/libhakozuna_hz4.so \
  /usr/bin/time -v \
  ./hakozuna/out/bench_random_mixed_mt_remote_malloc 16 2000000 400 16 2048 0 65536
```

```sh
env LD_PRELOAD=./hakozuna/hz4/libhakozuna_hz4.so \
  /usr/bin/time -v \
  ./hakozuna/out/bench_random_mixed_mt_remote_malloc 16 2000000 400 16 2048 50 65536
```

---

## 3) 期待される観測

- `page_decommit > 0` が **R=0 で出る**。
- `page_commit` も増える（再利用あり）。

---

## 4) 記録フォーマット

```
R=0  ops/s=... RSS=...KB  page_decommit=... page_commit=...
R=50 ops/s=... RSS=...KB  page_decommit=... page_commit=...
```

