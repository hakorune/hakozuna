# S32: dist=app gap（-7〜8%）を “固定費2本” で削る（TLS init check / dst compare）

Status: COMPLETED（S32-1 GO / S32-2 NO-GO）

目的:
- S31 perf で dist=app gap の主因が確定:
  - `hz3_malloc` の **TLS init check** が hot で毎回走り、malloc の ~20% を消費
  - `hz3_free` の **dst == my_shard compare** が毎回走り、free の ~8% を消費
- tail は主犯ではない（S30）。
- したがって S32 では、箱理論を守りつつ **hot path から固定費を消す**。

参照:
- S29: `hakozuna/hz3/docs/PHASE_HZ3_S29_DISTAPP_GAP_REFRESH_PERF_WORK_ORDER.md`
- S30: `hakozuna/hz3/docs/PHASE_HZ3_S30_RESULTS.md`
- S31: `hakozuna/hz3/docs/PHASE_HZ3_S31_PERF_HOTSPOT_RESULTS.md`
- S32 結果: `hakozuna/hz3/docs/PHASE_HZ3_S32_RESULTS.md`

---

## 制約（SSOT/箱理論）

- allocator挙動の切替は **compile-time `-D`** のみ（envノブ禁止）。
- hot path に **新しい分岐/新しい共有load** を増やさない（増やすなら A/B で証明）。
- 失敗/試行は `hakozuna/hz3/archive/research/<topic>/README.md` に固定し、
  `hakozuna/hz3/docs/NO_GO_SUMMARY.md` へ索引化する。
- GO/NO-GO の主判定は **RUNS=30**。

---

## S32-1（推奨・先にやる）: TLS init check を “miss/slow 入口” に押し込む

狙い:
- `hz3_malloc` hot hit から `hz3_tcache_ensure_init()`（cmp/jcc）を排除する。
- “TLSはゼロ初期化で存在する” を利用して、init前は bin を空扱いできる形を維持する。

### アイデア（1行）

hot hit では init を見ない。**miss/slow の入口でだけ** `hz3_tcache_ensure_init_slow()` を呼ぶ。

### 実装方針

1) `hz3_malloc`:
- 現状: hot の早い段階で `hz3_tcache_ensure_init()` を呼んでいる箇所がある。
- 変更: **bin pop → miss 判定**までは init を呼ばない。
- miss に落ちる入口（`hz3_alloc_slow` / refill / central / segment に触る直前）で `hz3_tcache_ensure_init_slow()`。

2) `hz3_free`:
- free hot はすでに PTAG32 で dst/bin を取って push するだけの形に寄っている。
- “初回 free が先に来る”ケースがありうるので、S32-1 では **free に init を戻さない**（hot を汚さない）。
  - init前の `my_shard` 未設定が free の固定費/不正動作を招かないよう、S32-2（row_off）とセットで進める前提にする。

3) `hz3_realloc` / `hz3_usable_size`:
- これらは hot より正しさ/安全を優先してよい（ただし余計な per-op 判定は避ける）。
- “in arena but tag==0” は S21-2 の方針どおり slow fallback（segmap/seg_hdr）へ。

### A/B フラグ案

- `HZ3_TCACHE_INIT_ON_MISS=0/1`（NEW）
  - 0: 現状（hot で ensure_init）
  - 1: miss/slow 入口でのみ ensure_init_slow

### 変更ファイル（見込み）

- `hakozuna/hz3/src/hz3_hot.c`
- `hakozuna/hz3/include/hz3_tcache.h`
- `hakozuna/hz3/src/hz3_tcache.c`（必要なら初期値整備のみ）

### GO/NO-GO

GO:
- dist=app（RUNS=30）で +2% 以上（目安）
- uniform / tiny-only（RUNS=30）で退行なし（±1%）
- `objdump`/`perf annotate` で `hz3_malloc` hot hit の `cmpb/jcc` が消えている（必要条件）

NO-GO:
- dist=app が動かない（±1%） or 退行
- tiny/uniform に明確な退行

結果（S32-1）:
- **GO**（dist=app +2.55%）
- hz3 lane 既定（`hakozuna/hz3/Makefile:HZ3_LDPRELOAD_DEFS`）では `HZ3_TCACHE_INIT_ON_MISS=1` を採用

---

## S32-2（推奨）: dst compare を消す（row_off 方式）

狙い:
- S28-2C（local bins split）は維持したい。
- しかし `if (dst == my_shard)` が free hot の固定費になっている。
- 分岐を消し、push先を **offset（定数）**で選ぶ。

### 方式（概要）

TLS内に `row_off[HZ3_NUM_SHARDS]`（`&t_hz3_cache` からのオフセット）を持つ:
- 初期状態（未init）: **全dstが remote bank row** を指す
- init 後: **my_shard の1要素だけ** `local_row` を指すように patch

free hot は:
- `Hz3Bin* b = (Hz3Bin*)((uintptr_t)&t_hz3_cache + row_off[dst] + bin*sizeof(Hz3Bin));`
- `hz3_bin_push(b, ptr);`
で終了（compareなし）。

重要（未init安全）:
- `row_off[]` は **定数の `offsetof(...)`** を使う（pointerテーブルはTLS初期化が難しい）。
- init前は `row_off[*]` がすべて `bank[dst][0]` を指すので、local free も一旦 bank に積まれるだけで安全に完走できる。
  - flush は event-only（refill/epoch/destructor）で動くため、initは結局 slow 入口で必ず行われる想定。

### A/B フラグ案

- `HZ3_LOCAL_BINS_SPLIT_ROW_OFF=0/1`（当時）
  - 0: 現状（dst compare）
  - 1: row_off（compareなし）

### 変更ファイル（見込み）

- `hakozuna/hz3/include/hz3_types.h`（tcache struct 追加）
- `hakozuna/hz3/src/hz3_tcache.c`（init時 patch）
- `hakozuna/hz3/src/hz3_hot.c`（push先選択を差し替え）

### GO/NO-GO

GO:
- dist=app（RUNS=30）で +1% 以上（目安）
- uniform / tiny-only（RUNS=30）で退行なし（±1%）
- `perf annotate` で `dst == my_shard` compare の重い行が消える/縮む（必要条件）

NO-GO:
- mixed系で退行
- init前/未init状態で不正動作（failfast/segfault）

結果（S32-2）:
- **NO-GO**（dist=app -2.48%）
- アーカイブ: `hakozuna/hz3/archive/research/s32_2_row_off/README.md`

補足（アーカイブ方針）:
- S32-2 は NO-GO のため、本線（`hakozuna/hz3/src`）からコードを撤去し、研究箱としてアーカイブした。
- 以降 `HZ3_LOCAL_BINS_SPLIT_ROW_OFF` は mainline では提供しない（README参照）。

---

## 測定セット（最小）

必須（RUNS=30）:
1) dist=app（`bench_random_mixed_malloc_dist ... app`）
2) uniform（`run_bench_hz3_ssot.sh` 既定）
3) tiny-only（`trimix 16 256 100 ...`）

追加（任意）:
- 257–2048 only（midの形状確認、退行検知）

perf（dist=app, RUNS=1）:
- `instructions,cycles,branches,branch-misses`

---

## 失敗時のアーカイブ

- `hakozuna/hz3/archive/research/s32_1_tcache_init_on_miss/README.md`
- `hakozuna/hz3/archive/research/s32_2_row_off_no_dst_compare/README.md`

---

## 参考（重い代替案 / 原則は最後）

S32-1/2 が思ったより効かない、または “thread exit cleanup を必ずスレッド開始時に用意したい” 場合のみ検討:

- `HZ3_WRAP_PTHREAD_CREATE=0/1`（案）
  - `pthread_create` を interpose して start routine の最初に `hz3_tcache_ensure_init_slow()` を呼ぶ。
  - hot 0命令だが、interpose の再入・互換・保守コストが高いので最後の手段。
