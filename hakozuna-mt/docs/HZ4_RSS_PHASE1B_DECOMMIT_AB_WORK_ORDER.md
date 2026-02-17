# HZ4 RSS Phase 1B: Decommit A/B 検証（Option A）

目的:
- Phase 1（page meta 分離 + decommit）の **安全性と RSS 効果**を SSOT で確認する。
- `HZ4_PAGE_DECOMMIT` の ON/OFF で **正しい A/B** を取る。

非目的:
- 新しい最適化の追加（チューニングは Phase 2 以降）。
- ベンチ条件の改変（SSOT 条件を固定）。

---

## Box Theory
- **PageMetaBox** と **OS Box** のみで完結（hot path 変更なし）。
- 変更はフラグで **即時に切り戻し可能**（A/B）。
- 可視化は **atexit 1行**の OS stats のみ。

---

## 1) ビルド（A: decommit OFF）

```sh
make -C hakozuna/hz4 clean all \
  HZ4_DEFS_EXTRA='-DHZ4_OS_STATS=1 -DHZ4_PAGE_META_SEPARATE=1 -DHZ4_PAGE_DECOMMIT=0'
```

---

## 2) 実行（A: decommit OFF）

```sh
env LD_PRELOAD=./hakozuna/hz4/libhakozuna_hz4.so \
  /usr/bin/time -v \
  ./hakozuna/out/bench_random_mixed_mt_remote_malloc 16 2000000 400 16 2048 90 65536
```

> 注意: `env LD_PRELOAD=... /usr/bin/time -v <bench>` の順で測る（逆順は無効）。

---

## 3) ビルド（B: decommit ON）

```sh
make -C hakozuna/hz4 clean all \
  HZ4_DEFS_EXTRA='-DHZ4_OS_STATS=1 -DHZ4_PAGE_META_SEPARATE=1 -DHZ4_PAGE_DECOMMIT=1'
```

---

## 4) 実行（B: decommit ON）

```sh
env LD_PRELOAD=./hakozuna/hz4/libhakozuna_hz4.so \
  /usr/bin/time -v \
  ./hakozuna/out/bench_random_mixed_mt_remote_malloc 16 2000000 400 16 2048 90 65536
```

---

## 5) 期待される観測

- `HZ4_OS_STATS` の `page_decommit` が **0 より大きい**。
- `page_commit` も **0 より大きい**（再利用が発生している証拠）。
- RSS が A より **下がる方向**に動く（少なくとも同等）。

---

## 6) 記録フォーマット（例）

```
SSOT (T=16, R=90) RSS A/B
- A: decommit=OFF ops/s=...  RSS=...KB  page_decommit=0 page_commit=0
- B: decommit=ON  ops/s=...  RSS=...KB  page_decommit=... page_commit=...
```

---

## 7) 異常時のチェック

- `page_decommit=0` のまま → RSS が下がらないのは正常（decommitが発火していない）。
  - まずは **分散が少ない短尺**で再現確認（RUNS=3 程度）。
- `HZ4_FAILFAST=1` で decommit access があれば即停止する。

