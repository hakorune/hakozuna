# HZ4 RSS Phase 1C: Decommit Trigger Probe

目的:
- decommit が**本当に発火する条件**を特定し、RSS 低下の起点を掴む。
- Phase 1B の「page_decommit=0」状態を解消する。

非目的:
- チューニング最適化（Phase 2 以降）。
- hot path への新規ログ追加。

---

## Box Theory
- 変更なし（**実験条件のみ**変える）。
- 可視化は `HZ4_OS_STATS` の atexit 1行のみ。

---

## 1) ビルド（decommit ON 固定）

```sh
make -C hakozuna/hz4 clean all \
  HZ4_DEFS_EXTRA='-DHZ4_OS_STATS=1 -DHZ4_PAGE_META_SEPARATE=1 -DHZ4_PAGE_DECOMMIT=1'
```

---

## 2) 実行（R=0: local-only）

```sh
env LD_PRELOAD=./hakozuna/hz4/libhakozuna_hz4.so \
  /usr/bin/time -v \
  ./hakozuna/out/bench_random_mixed_mt_remote_malloc 16 2000000 400 16 2048 0 65536
```

期待:
- `page_decommit > 0` が出るなら **decommit 経路は生きている**。

---

## 3) 実行（R=50: balanced）

```sh
env LD_PRELOAD=./hakozuna/hz4/libhakozuna_hz4.so \
  /usr/bin/time -v \
  ./hakozuna/out/bench_random_mixed_mt_remote_malloc 16 2000000 400 16 2048 50 65536
```

期待:
- `page_decommit` と RSS の相関を確認（R=0 より少なくても OK）。

---

## 4) 記録フォーマット

```
R=0  ops/s=... RSS=...KB  page_decommit=... page_commit=...
R=50 ops/s=... RSS=...KB  page_decommit=... page_commit=...
```

---

## 5) もし page_decommit が 0 のままなら

- 「used_count が 0 にならない」可能性が高い。
- 次の一手（Phase 1D 候補）:
  - 空ページ判定の条件を **観測用に**追加（ワンショットのみ）。
  - `used_count` の整合性を最小テストで確認。

