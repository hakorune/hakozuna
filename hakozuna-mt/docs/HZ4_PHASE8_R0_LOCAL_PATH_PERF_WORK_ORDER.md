# HZ4 Phase 8: R=0 Local Path Perf（RouteBox / PopulateBox / TLS Hot-Cold）

目的: `bench_random_mixed_mt_remote_malloc` の **R=0**（local-only）で hz4 の throughput を押し上げる。

前提（SSOT）
- repo: `/mnt/workdisk/public_share/hakmem`
- bench: `hakozuna/out/bench_random_mixed_mt_remote_malloc`
- 基本条件: `T=16 iters=2000000 ws=400 size=16..2048 R=0 ring=65536`

方針（箱理論）
- **境界1箇所**: local-only の変更は `TcacheBox/RefillBox` と “page refill/populate” 境界に閉じる。
- **戻せる**: すべて `#ifndef` フラグで opt-in（A/B 即切替）。
- **見える化**: perf / 1行サマリ（常時ログ禁止）。
- **Fail-Fast**: invariants を壊したら即 abort（release 既定では無効、debug で有効）。

---

## 0) 現状（2026-01-26 時点の観測メモ）

- `HZ4_TLS_MERGE=1` は R=0 で +1.9%（GO → default ON 反映済み）。
- `HZ4_TCACHE_COUNT=0` + `HZ4_LOCAL_MAGIC_CHECK=0` は追加 +0.6%（marginal）。
- `HZ4_PAGE_TAG_TABLE=1` は -9.3%（NO-GO）。
- PTAG32-lite（owner/sc を “1 load” 化）は効果ほぼ無し（-0.3%）。
- R=90 は inbox lane などで強いが、**本 Phase は R=0 に集中**（remote/inbox は触らない）。

---

## 1) Phase 8A: RouteBox（最小・低リスク）

狙い: free 側の routing を “1回計算 + 再利用” で短縮し、不要な TLS 取得や重い分岐を減らす。

### 8A-1: TLS 1回取得の free fast-path
- `hz4_free()` が `hz4_tls_get()` を 1回だけ呼び、`hz4_small_free_with_page_tls(tls, ...)` に渡す。
- `hz4_small_free_with_page` 内で `hz4_tls_get()` をしない。

判定:
- GO: R=0 +2% 以上、または perf 上の `hz4_free` self が目に見えて低下
- NO-GO: +1% 未満で終了（この箱単体の伸びしろは小さい）

---

## 2) Phase 8B: PopulateBox（本命：slice refill）

狙い: R=0 の “新規 page” コストを下げる。

背景:
- 現状の `hz4_populate_page()` は page 全体をループして全 obj の next を書く（64KiB を全面タッチ）。
- R=0 は remote が無いので、勝負はほぼ local refill の初期化コスト。

### 8B-1: Slice refill（段階生成）
本アイデアは **NO-GO** としてアーカイブ済み。

- アーカイブ: `hakozuna/hz4/archive/research/hz4_phase8b_slice_refill/README.md`

（Phase 8B はアーカイブ済みのため、以降は Phase 8C へ進む）

---

## 3) Phase 8C: TLS Hot/Cold Split

本アイデアは **NO-GO** としてアーカイブ済み。

- アーカイブ: `hakozuna/hz4/archive/research/hz4_phase8c_tls_hot_cold_split/README.md`

---

## 4) 測定（SSOT）

ベースライン（perf lane）
```sh
make -C hakozuna/hz4 clean all \
  HZ4_DEFS_EXTRA='-DHZ4_PAGE_META_SEPARATE=0 -DHZ4_PAGE_DECOMMIT=0'
env -i PATH="$PATH" HOME="$HOME" LD_PRELOAD="$PWD/hakozuna/hz4/libhakozuna_hz4.so" \
  ./hakozuna/out/bench_random_mixed_mt_remote_malloc 16 2000000 400 16 2048 0 65536
```

perf（R=0）
```sh
perf record -g -- env -i PATH="$PATH" HOME="$HOME" LD_PRELOAD="$PWD/hakozuna/hz4/libhakozuna_hz4.so" \
  ./hakozuna/out/bench_random_mixed_mt_remote_malloc 16 2000000 400 16 2048 0 65536
perf report --stdio | head -n 40
```

---

## 5) TODO（順番）

1. 既存の NO-GO 箱を増やさない（Phase 8B/8C はアーカイブ済み）
2. Phase 9（RouteBox head64）も NO-GO（アーカイブ済み）:
   - `hakozuna/hz4/archive/research/hz4_phase9_routebox_head64/README.md`
3. 次は “route” を増やさず、`hz4_small_malloc`/`refill` の差分を perf で切り出す
   - 目標: hz3(p5b) に対して R=0 の差分（~20%台）を “どの箱が払っているか” を確定する
