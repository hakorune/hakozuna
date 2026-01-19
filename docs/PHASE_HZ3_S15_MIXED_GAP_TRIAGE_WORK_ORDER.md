# PHASE_HZ3_S15: mixed（16–32768）gap triage（Work Order）

目的:
- SSOT の `mixed (16–32768)` で残っている tcmalloc 差（~ -16%）を詰める。
- hot path を太らせず、A/B と NO-GO アーカイブ前提で “当たり所” を特定する。

前提（現状の観測）:
- `small` は hz3 が勝っている、`medium` は小差、`mixed` が大差。
- perf では shared path（central/inbox）は非常に低い割合で、hot 側（特に free / PTAG）が支配的。

参照:
- SSOT-all: `hakozuna/scripts/run_bench_ssot_all.sh`
- hz3 SSOT: `hakozuna/hz3/scripts/run_bench_hz3_ssot.sh`

制約:
- allocator挙動の切替は compile-time `-D` のみ（envノブ禁止）
- hot path に per-op 統計を入れない（stats は slow/event のみ）
- 1 commit = 1 purpose

---

## 1) まず “原因を割る” 計測（S15-0）

mixed は `16–32768` の均一乱数なので、以下のどれが支配かを確定する:

1. `sc=0 (4KB)` の比率/枯渇が高く、slow alloc が増えている
2. `malloc/free` の “サイズ分岐の混在” で分岐予測が崩れている
3. `PTAG range check + tag load + decode` の固定費が大きい

### 追加するカウンタ（slow/event only）

- `alloc_slow_calls[sc]`（既存 `refill_calls` で代用可）
- `central_pop_miss[sc]`（既存）
- `segment_new_count`（既存）

この3つを `small/medium/mixed` で比較し、
- `mixed` だけ `sc=0` が突出していないか
- `mixed` だけ `segment_new_count` が増えていないか
を確認する。

（hot統計は禁止のため、free/mallocの経路比率は推測せず “slow側の増え方” で割る）

---

## 2) mixed 改善の最短候補（S15-1）

### 仮説A（最有力）: “4KB class の cache が薄い”

mixed では `2049–4095` が `4KB (sc=0)` に繰り上がるため、`sc=0` の需要が増える。
tcmalloc は 2–4KB 帯でより細かい sizeclass かつ cache bytes が厚いので、
hz3 が `sc=0` 枯渇で slow alloc が増えると負けやすい。

やること（A/B）:
- `sc=0` だけ `bin_target` を増やす（例: 16→32）
- 併せて `HZ3_REFILL_BATCH[0]` は既に 12 なので据え置きでOK（必要なら 16 を試す）

要件:
- `small/medium` に回帰を出さない（±0%）
- mixed が +5% 以上動けば GO

### S15-1 結果（既知）

`sc=0` bin 厚みは **mixed の主因ではなかった**（small/medium は改善するが mixed は動かない）。
詳細: `hakozuna/hz3/docs/PHASE_HZ3_S15_1_BIN_TARGET_TUNING_RESULTS.md`

---

## 3) hot 固定費削減（S15-2）

### 仮説B: PTAG range check が重い（atomic loadが多い）

`hz3_arena_page_index_fast()` は `base` と `end` を atomic load しているが、
`HZ3_ARENA_SIZE` は定数なので `end = base + HZ3_ARENA_SIZE` にできる。

狙い:
- hot 1回あたり atomic load を 1 回減らす（特に free の割合が高い workload で効きやすい）

注意:
- これは “箱を増やす” ではなく “固定費を削る” だけなので安全度が高い。

### 重要メモ（S15-2A: `end = base + 4GB` は逆効果になりうる）

`HZ3_ARENA_SIZE = (1ULL << 32)`（4GB）は、x86-64 では `add r64, imm32` の即値に収まらない（imm32 は sign-extend）ため、
コンパイラが `movabs $0x1_0000_0000, reg; add reg, base` のような **64-bit 即値ロード**を生成しやすい。

その結果:
- もともと `end` を 1 load で済ませていた場合、
- “メモリロード1回” → “即値ロード+加算（命令列が増える）” に置換され、
- mixed のように呼び出し頻度が最大の workload だけ落ちる説明がつく。

確認方法（推測禁止）:
- `objdump -d` で `hz3_arena_page_index_fast` の inlined 生成コードを確認し、`movabs` が増えていないかを見る。
- `perf stat -e cycles,instructions,branch-misses` で mixed の instruction 増加が起きていないかを見る。

### 代替案（S15-2B: “4GB即値を使わない” range check）

atomic load を減らしつつ `movabs` を避けるなら、`end` を作らず **delta の上位32bitを見る**方式が候補:

- `delta = (uintptr_t)ptr - (uintptr_t)base`
- `if ((delta >> 32) != 0) return 0;`（4GBを超えたら即reject）
- `page_idx = (uint32_t)(delta >> 12)`

この形なら 4GB の即値をロードせずに範囲チェックできる。
（ただし実際の命令列は必ず `objdump` で確認し、mixed で A/B する）

### S15-2B 結果（既知）

`delta>>32` 方式は mixed で明確な改善が出ず（±1%内）、small/medium が -1〜-2% 程度落ちたため **NO-GO**。
詳細: `hakozuna/hz3/docs/PHASE_HZ3_S15_2B_ARENA_RANGE_CHECK_RESULTS.md`

---

## 4) perf で “差の正体” を確定（S15-3）

### 結論

mixed の差は branch miss / cache miss ではなく、**命令数差**が支配的。
（= “共有経路” をいじっても詰め切れない可能性が高い。hot の命令列削減が本命。）

詳細（SSOT固定）:
- `hakozuna/hz3/docs/PHASE_HZ3_S15_3_MIXED_INSN_GAP_PERF_RESULTS.md`

次フェーズ（実装ターゲット）:
- `hakozuna/hz3/docs/PHASE_HZ3_S16_MIXED_INSN_REDUCTION_WORK_ORDER.md`

---

## 5) GO/NO-GO

GO（目安）:
- mixed が +5% 以上（tcmalloc gap の芯に当たる改善）
- small/medium は ±0%（回帰なし）

NO-GO:
- mixed が動かない（±1%）
- small/medium が落ちる

NO-GO の場合:
- revert
- `hakozuna/hz3/archive/research/s15_mixed_gap_<topic>/README.md` に SSOTログと結論を固定
- `hakozuna/hz3/docs/NO_GO_SUMMARY.md` に追記
