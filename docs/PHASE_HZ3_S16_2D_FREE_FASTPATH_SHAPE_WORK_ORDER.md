# PHASE_HZ3_S16-2D: hz3_free “fast path の形” を変えて spills を消す（Work Order）

目的:
- `mixed (16–32768)` の tcmalloc 差のうち、`hz3_free` hot の固定費（命令数/ロードストア）を削る。
- S16-2 / S16-2C が NO-GO だったため、次は **式の最適化ではなく、生成コードの形（prolog/spills）**を変える。

背景:
- S16-2C: lifetime split は NO-GO（mixed -1.2%、instructions +2.8%）。
- “callee-saved push が入口で固定発生”している場合、式を弄っても改善しづらい。

状況:
- S16-2C 結果は固定済み（`PHASE_HZ3_S16_2C_TAG_DECODE_LIFETIME_RESULTS.md`）
- 次の本命は「生成コードの形」を変える本手順

制約:
- allocator本体のノブは compile-time `-D` のみ
- hot に per-op stats を入れない
- 1 commit = 1 purpose / NO-GO はアーカイブ
- A/B gate: `HZ3_FREE_FASTPATH_SPLIT=0/1`

参照:
- S16: `hakozuna/hz3/docs/PHASE_HZ3_S16_MIXED_INSN_REDUCTION_WORK_ORDER.md`
- S16-2C結果: `hakozuna/hz3/docs/PHASE_HZ3_S16_2C_TAG_DECODE_LIFETIME_RESULTS.md`

---

## 仮説（S16-2D）

mixed が遅い主因は:
- branch/caches ではなく、
- `hz3_free` が **callee-saved regs の save/restore**や **stack spills**を起こしており、
  それが tcmalloc の “超フラット” free と比べて命令数差になっている。

---

## 施策候補（A/Bで1つずつ）

### D1) fast/slow を関数境界で分離（spill を外へ）

狙い:
- `hz3_free` の “最頻ホット” 部分を **小さく**して、callee-saved を使わない形にする。
- 低頻度の分岐先は `noinline` に逃がし、ホット側の live range を減らす。

注意:
- 関数呼び出しが増えるので、A/B必須。
- 目的は「callを増やす」ことではなく「prolog/spillを減らす」こと。

例（方針）:
- `hz3_free()` は `range check + tag load + kind` まで
- `hz3_free_small_v2_impl()` / `hz3_free_medium_impl()` を `__attribute__((noinline))` で分離

### D2) `hz3_free` を “leaf化” する（使うレジスタを絞る）

狙い:
- 可能なら `hz3_free` を leaf（callee-saved不要）に近づける。

やること（例）:
- 余計な一時変数・ポインタを削り、1つの `tag` と最低限の派生値だけで分岐
- `switch(kind)` の形を固定し、コンパイラが保存レジスタを使いにくい形に寄せる

### D3) ビルドの “生成コード” 側を詰める（慎重）

※アーキ変更ではなく “同じソースで命令列を減らす” 試行。

候補（安全な順）:
- `-fomit-frame-pointer`（既にONの可能性あり、確認）
- `-fno-plt`（PLT固定費削減、ただし mixed で効くかは要確認）
- `-flto`（LTOで inlining が進み、prologが減る可能性）

注意:
- これは “hz3のアルゴリズム” ではなく “生成物” に依存するため、SSOTログに CFLAGS を必ず残す。

---

## 計測（必須）

### 1) SSOT（RUNS=10）
- `small/medium/mixed` の median を保存
- 要件:
  - `mixed` +2% 以上（理想 +5%）
  - `small/medium` ±0%（回帰なし）

### 2) perf stat（mixed）
- `instructions,cycles,branches,branch-misses`

### 3) objdump（推測禁止）
- `hz3_free` 周辺の prolog/epilog を確認:
  - callee-saved push/pop が減ったか
  - stack spills（`[rsp+...]`）が減ったか

---

## GO/NO-GO

GO:
- `mixed` +2% 以上
- `instructions` が減る、または同等でも `cycles` が減る
- `small/medium` ±0%

NO-GO:
- `mixed` が ±1% 内
- `instructions` が増える
- `small/medium` が落ちる

NO-GO の場合:
- revert
- `hakozuna/hz3/archive/research/s16_2d_free_fastpath_shape/README.md` に SSOTログ / perf / objdump を固定
- `hakozuna/hz3/docs/NO_GO_SUMMARY.md` に追記

---

## 結果（S16-2D）

ビルド:
- baseline: `HZ3_FREE_FASTPATH_SPLIT=0`
- split: `HZ3_FREE_FASTPATH_SPLIT=1`

SSOT（RUNS=10, hz3 median）:
- baseline logs: `/tmp/hz3_ssot_6cf087fc6_20260101_152106`
- split logs: `/tmp/hz3_ssot_6cf087fc6_20260101_152212`
- small: 100667439.46 → 100083143.72（-0.58%）
- medium: 102875818.92 → 101964668.33（-0.89%）
- mixed: 94342898.98 → 95924636.39（+1.68%）

perf stat（mixed, RUNS=1）:
- baseline: `/tmp/s16_2d_baseline_perf.txt`
- split: `/tmp/s16_2d_split_perf.txt`
- instructions: 1.446B → 1.495B（増加）
- cycles: 820M → 824M（微増）

objdump:
- baseline: `/tmp/s16_2d_baseline_hz3_free.txt`
- split: `/tmp/s16_2d_split_hz3_free.txt`

判定:
- mixed の改善は +1.68% で目標未達。
- instructions が増えており “形変更” は NO-GO。
