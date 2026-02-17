# HZ4 Phase 11: TcacheBox（Bulk Push / Populate 最適化）指示書

目的: R=0（local-only）で観測された `hz4_tcache_pop/push` の overhead を減らす。

※ **本 Phase は NO-GO（2026-01-26）**。実装は本線から削除し、研究箱としてアーカイブ済み。

箱理論（Box Theory）
- **箱**: `TcacheBox`
- **境界1箇所**: `hz4_populate_page()`（refill 境界）だけを変更する
- **戻せる**: 研究箱として隔離（本線は常に OFF）
- **見える化**: SSOT（ops/s）+ perf（`hz4_tcache_push` の self%）
- **Fail-Fast**: `HZ4_FAILFAST=1` 時の既存 invariant を壊さない（構造変更なし）

---

## 背景

R=0 perf で `hz4_tcache_pop/push` が無視できない割合を占めた。

本 Phase は “routing を足す” のではなく、**page populate のループ**で発生する
`bin->head` 更新（毎 iteration の load/store）をまとめて減らす。

狙い:
- `hz4_populate_page()` の `hz4_tcache_push()` 呼び出し回数は維持するが、
  **bin->head の更新を 1 回**にし、count 更新も 1 回にまとめる。

---

## A/B フラグ

ファイル: `hakozuna/hz4/core/hz4_config.h`

- 以前の実装では `HZ4_TCACHE_BULK_PUSH=0/1` を用意して A/B していたが、NO-GO のため **本線から削除済み**。
- 再挑戦する場合は、この指示書を元に “研究箱” として再実装してから A/B する。

---

## 実装（最小）

### Step 1: bulk splice helper を追加（TcacheBox）

ファイル: `hakozuna/hz4/src/hz4_tcache.c`

追加（例）:
- `hz4_tcache_push_list(bin, head, tail, n)`（既存 `hz4_tcache_splice` を流用してもOK）

### Step 2: `hz4_populate_page()` を “list build + splice” に変更（境界1箇所）

ファイル: `hakozuna/hz4/src/hz4_tcache.c`

現状:
- ループで `hz4_tcache_push(bin, obj)` を繰り返し（毎回 `bin->head` を read/write）

変更案:
1) page 内で `obj->next` を **連結**（アドレス昇順で OK）
2) 最後に 1 回だけ `tail->next = bin->head; bin->head = head;`
3) `HZ4_TCACHE_COUNT` は `+= n` を 1 回だけ

注意:
- オブジェクト順序（LIFO/FIFO）に意味は無い前提（tcache は “再利用可能な free リスト”）。
- owner-thread only なのでロック不要。

任意（同じ最適化の適用先）:
- RSS lane の `hz4_page_rebuild_decommitted()`（同じ “page 全走査で push” をしている）

---

## A/B（SSOT）

### Baseline
```sh
cd /mnt/workdisk/public_share/hakmem
make -C hakozuna/hz4 clean all \
  HZ4_DEFS_EXTRA='-DHZ4_PAGE_META_SEPARATE=0 -DHZ4_PAGE_DECOMMIT=0'
env -i PATH="$PATH" HOME="$HOME" LD_PRELOAD="$PWD/hakozuna/hz4/libhakozuna_hz4.so" \
  ./hakozuna/out/bench_random_mixed_mt_remote_malloc 16 2000000 400 16 2048 0 65536
```

### Bulk push ON
```sh
cd /mnt/workdisk/public_share/hakmem
make -C hakozuna/hz4 clean all \
  HZ4_DEFS_EXTRA='-DHZ4_PAGE_META_SEPARATE=0 -DHZ4_PAGE_DECOMMIT=0 -DHZ4_TCACHE_BULK_PUSH=1'
env -i PATH="$PATH" HOME="$HOME" LD_PRELOAD="$PWD/hakozuna/hz4/libhakozuna_hz4.so" \
  ./hakozuna/out/bench_random_mixed_mt_remote_malloc 16 2000000 400 16 2048 0 65536
```
※ 現状は `HZ4_TCACHE_BULK_PUSH` の実装が本線に無いので、そのままではビルドできません（研究箱で復活させた場合のみ）。

判定:
- GO: R=0 **+2% 以上**
- NO-GO: **+1% 未満**なら撤退（複雑度を増やさない）

---

## perf（任意）

```sh
perf record -g -- env -i PATH="$PATH" HOME="$HOME" LD_PRELOAD="$PWD/hakozuna/hz4/libhakozuna_hz4.so" \
  ./hakozuna/out/bench_random_mixed_mt_remote_malloc 16 2000000 400 16 2048 0 65536
perf report --stdio | head -n 60
```

期待:
- `hz4_tcache_push` / `hz4_populate_page` の self% が低下

---

## 結果（2026-01-26）

結論: **NO-GO**

- R=0（runs=10 median）:
  - baseline: 267.32M ops/s
  - Bulk Push ON: 267.33M ops/s（+0.004%）
- R=90 sanity: パス（例: 85M ops/s）

推定理由:
- `hz4_populate_page()` は refill 時のみで、R=0 では呼び出し頻度が低く、全体に波及しない。

アーカイブ:
- `hakozuna/hz4/archive/research/hz4_phase11_tcache_bulk_push/README.md`
