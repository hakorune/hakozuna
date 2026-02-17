# HZ4 RSS Phase 1D: Local‑Free Decommit Trigger

目的:
- local free だけの経路でも **used_count==0 を捕捉**して decommit を発火させる。
- Phase 1C で `page_decommit=0` だった問題を解消する。

非目的:
- hot path の大規模変更（分岐 1 回のみ）。
- チューニング最適化（Phase 2 以降）。

---

## Box Theory
- 変更は **TCache Box の owner free 境界**のみ。
- decommit 自体は **OS Box** に閉じる（既存のまま）。
- A/B 可能（`HZ4_PAGE_DECOMMIT`）。

---

## 1) 変更内容（要約）

owner free で `used_count` を減らした直後に、0 なら pageq へ通知:

```c
if (owner_tid == tls->tid) {
    hz4_page_used_dec(page, 1);
    if (hz4_page_used_count(page) == 0) {
        hz4_pageq_notify(page);
    }
    hz4_tcache_push(...);
}
```

---

## 2) ビルド（decommit ON）

```sh
make -C hakozuna/hz4 clean all \
  HZ4_DEFS_EXTRA='-DHZ4_OS_STATS=1 -DHZ4_PAGE_META_SEPARATE=1 -DHZ4_PAGE_DECOMMIT=1'
```

---

## 3) 実行（R=0 / R=50）

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

## 4) 期待される観測

- `page_decommit > 0` が **R=0 で出る**（local-only でも発火）。
- `page_commit` も増える（再利用あり）。

---

## 5) 記録フォーマット

```
R=0  ops/s=... RSS=...KB  page_decommit=... page_commit=...
R=50 ops/s=... RSS=...KB  page_decommit=... page_commit=...
```

