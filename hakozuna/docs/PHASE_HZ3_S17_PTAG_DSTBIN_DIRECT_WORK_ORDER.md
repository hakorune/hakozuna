# PHASE_HZ3_S17: PTAG dst/bin direct（hot free 最短化）

目的:
- `hz3_free` の命令数を削減し、mixed の gap を詰める。
- `kind/owner` 判定を消し、**range check + tag load + push** のみで hot を完結させる。

方針（箱理論）:
- Hot Box: free は “dst/bin へ push” のみ。
- Event Box: remote bank を inbox へ flush（drain-first/epoch のみ）。
- 既存の small/medium アルゴリズムは温存（分類と配達だけを変える）。

---

## 0) コンパイルゲート

`hz3/include/hz3_config.h`

- `HZ3_PTAG_DSTBIN_ENABLE`（default 0）
- `HZ3_PTAG_DSTBIN_STATS`（default 0, slow/event only）

---

## 1) タグ形式（32bit 推奨）

理由:
- bin 数が `HZ3_SMALL_NUM_SC + HZ3_NUM_SC` で 256 超の可能性がある。
- 32bit tag なら拡張余地が十分（1M pages = 4MB）。

レイアウト（例）:
- bit 0..15: `bin+1`（0 は “not ours”）
- bit 16..19: `dst+1`（shard id）
- それ以外: 予約

ヘルパ:
```c
static inline uint16_t hz3_tag_bin(uint32_t tag) { return (uint16_t)(tag & 0xFFFFu) - 1u; }
static inline uint8_t  hz3_tag_dst(uint32_t tag) { return (uint8_t)((tag >> 16) & 0x0Fu) - 1u; }
```

---

## 2) Bin index の統一

**bin index は全帯域で連番**にする（kind を消す）。

```
bin_small = sc_small;                         // 0 .. HZ3_SMALL_NUM_SC-1
bin_med   = HZ3_SMALL_NUM_SC + sc_medium;     // 0 .. HZ3_NUM_SC-1 の後ろ
HZ3_BIN_TOTAL = HZ3_SMALL_NUM_SC + HZ3_NUM_SC
```

---

## 3) TLS bank（dst×bin）

`hz3/include/hz3_types.h`（gate下）

```
Hz3Bin bank[HZ3_NUM_SHARDS][HZ3_BIN_TOTAL];
```

用途:
- alloc: `bank[my_shard][bin]` から pop
- free:  `bank[dst][bin]` に push（owner 判定不要）

---

## 4) Hot free（分岐削減）

`hz3/src/hz3_hot.c`（gate下）

```
if (!hz3_arena_page_index_fast(ptr, &page_idx)) fallback;
tag = hz3_pagetag_load32(page_idx);
if (tag == 0) fallback;
bin = hz3_tag_bin(tag);
dst = hz3_tag_dst(tag);
hz3_bin_push(&t_hz3_cache.bank[dst][bin], ptr);
return;
```

期待効果:
- `kind/owner` decode なし
- local/remote 分岐なし（push は共通）

---

## 5) Event flush（remote だけ吐く）

flush の入口:
- `hz3_alloc_slow()` 冒頭
- `hz3_epoch_force()` 内
- destructor

方針:
- `dst != my_shard` の bank を inbox へ “少量ずつ” flush（debt方式）
- hot には置かない

---

## 6) tag set/clear 境界

set:
- small v2: page を “v2用途で使い始める瞬間”
- medium: page を “medium用途で使い始める瞬間”
  - それぞれ `bin` と `dst` を encode

clear:
- segment recycle/unmap 時（既存の clear に統合）

**tag==0 は常に fallback**（false negative OK / false positive NG）。

---

## 7) 実装ステップ（A/B）

S17-0: フラグ + 32bit tag 配列（gated OFF）
- 既存16bit PTAGは維持、DSTBIN専用の `g_hz3_page_tag32` を追加

S17-1: tag set/clear を 32bit 版にも追加

S17-2: hot free を dst/bin direct に切替（gate）

S17-3: event flush 実装（remote bank → inbox）

---

## 8) GO/NO-GO

GO:
- mixed +2% 以上
- small/medium ±0%
- 命令数（perf stat instructions）が減る

NO-GO:
- mixed ±1% 内
- small/medium 回帰

NO-GO の場合:
- revert
- `hakozuna/hz3/archive/research/s17_ptag_dstbin_direct/README.md` に SSOT/perf を固定
- `hakozuna/hz3/docs/NO_GO_SUMMARY.md` に追記

---

## 9) SSOT 実行

```
make -C hakozuna/hz3 clean
make -C hakozuna/hz3 all_ldpreload \
  HZ3_LDPRELOAD_DEFS='-DHZ3_ENABLE=1 -DHZ3_SHIM_FORWARD_ONLY=0 \
  -DHZ3_SMALL_V2_ENABLE=1 -DHZ3_SEG_SELF_DESC_ENABLE=1 \
  -DHZ3_SMALL_V2_PTAG_ENABLE=1 -DHZ3_PTAG_V1_ENABLE=1 \
  -DHZ3_PTAG_DSTBIN_ENABLE=1'

SKIP_BUILD=1 RUNS=10 ./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh
```

perf（mixed）:
```
perf stat -e instructions,cycles,branches,branch-misses \
  env LD_PRELOAD=./libhakozuna_hz3_ldpreload.so \
  ./hakozuna/out/bench_random_mixed_malloc_args 20000000 400 16 32768 305419896
```

---

## 結果（S17-0）

SSOT（RUNS=10, hz3 median）:
- baseline logs: `/tmp/hz3_ssot_6cf087fc6_20260101_160855`
  - small: 95,462,553.14
  - medium: 97,600,034.27
  - mixed: 93,347,727.90
- dst/bin direct logs: `/tmp/hz3_ssot_6cf087fc6_20260101_160954`
  - small: 100,368,576.75 (+5.14%)
  - medium: 103,999,761.97 (+6.56%)
  - mixed: 97,646,359.10 (+4.60%)

判定:
- mixed +4.60% で GO（small/medium も改善）。

---

## perf（mixed, RUNS=1, seed固定）

ログ:
- baseline: `/tmp/hz3_s17_baseline_mixed_perf.txt`
- dstbin: `/tmp/hz3_s17_dstbin_mixed_perf.txt`

要点（dstbin vs baseline）:
- `instructions`: 1.446B → 1.428B（-1.27%）
- `cycles`: 830M → 798M（-3.90%）
- `branches`: 291M → 252M（-13.45%）
- `branch-misses`: 11.54M → 10.91M（-5.45%）

解釈:
- hot free の “分類/分岐/配達” が削れ、mixed の throughput 改善と整合する。

---

## S17-1: TLS snapshot（NO-GO）

狙い:
- arena base / PTAG32 base を TLS にスナップショットし、hot path の atomic load を避ける。

結果:
- mixed は伸びるが small が大きく落ちたため NO-GO。
- アーカイブ: `hakozuna/hz3/archive/research/s17_ptag_dstbin_tls/README.md`
