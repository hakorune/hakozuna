# HZ4 Phase 6: Collect Reduction Work Order

目的:
- T=16/R=90 で支配的な **hz4_collect のメタ仕事**を削減する
- collect が触る対象を減らして **branch-miss / L1D miss / IPC** を改善

前提:
- SegQ O(1) push 実装済み
- perf top3 で `hz4_collect` が最上位 (self ~26%)

---

## 0) ルール（Box Theory）

- 変更は **Collect / SegQ / PageQ / Carry** の箱内で完結
- 境界は 1 箇所（collect 内で完結させる）
- A/B は compile-time (`-D` / `#ifdef`)
- Fail‑Fast 維持

---

## 1) P0: seg 内 pageq を sc bucket 化（DONE）

### 狙い
`collect(sc)` が **無関係ページを触らない**ようにする

### 仕様（最小差分）
```
HZ4_PAGEQ_BUCKET_SHIFT=3  // 8 sc/桶 → 16 buckets (HZ4_SC_MAX=128)
HZ4_PAGEQ_BUCKETS = (HZ4_SC_MAX + ((1u << HZ4_PAGEQ_BUCKET_SHIFT) - 1)) >> HZ4_PAGEQ_BUCKET_SHIFT
bucket = sc >> HZ4_PAGEQ_BUCKET_SHIFT
```

### 変更点
- `hz4_seg.h`: `pageq_head[HZ4_PAGEQ_BUCKETS]` に変更
- `hz4_pageq_notify`: page->sc から bucket を選んで Treiber push
- `hz4_drain_segment`: `bucket = sc>>shift` の head だけ交換して drain
- `hz4_segq_finish`: 全 bucket を走査して pending 判定
- Fail‑Fast: `page->sc` / bucket 範囲外は即 abort

### A/B
```
HZ4_PAGEQ_BUCKET_SHIFT=3 (new)
```

### 期待
- `hz4_collect` の self% が低下
- branch/L1D miss の低下

### 結果（SSOT, 2026-01-23）
- T=16 R=90 (16–2048)
- Throughput: **~42.3M ops/s**（P0 前 35.9M から **+17.8%**）
- `hz4_collect` self: **8.78%**（P0 前 23.11% から **-62%**）
- perf top3（self%）:
  1) `hz4_collect` **8.78%**
  2) `bench_thread` **7.09%**
  3) `__tls_get_addr` **6.72%**
- HZ4 内トップ3（self%）:
  1) `hz4_collect` **8.78%**
  2) `hz4_malloc` **4.78%**
  3) `hz4_remote_flush` **3.13%**

---

## 2) P1: carry を複数スロット化

### 狙い
requeue/pushback を **最後の手段**にする

### 仕様
```c
// hz4_config.h (default=4)
#ifndef HZ4_CARRY_SLOTS
#define HZ4_CARRY_SLOTS 4
#endif
```

### データ構造

```c
// hz4_tls.h
typedef struct {
    void*        head;   // freelist head (NULL = empty)
    hz4_page_t*  page;   // source page (carry 元の page)
} hz4_carry_slot_t;

typedef struct {
    uint8_t          n;                          // 使用中スロット数 (0..HZ4_CARRY_SLOTS)
    hz4_carry_slot_t slot[HZ4_CARRY_SLOTS];      // LIFO stack
} hz4_carry_t;

// tls.carry[sc] で sc 別に保持
```

### 対象ファイル

| ファイル | 変更内容 |
|----------|----------|
| `core/hz4_config.h` | `HZ4_CARRY_SLOTS` 定義追加 (default=4) |
| `core/hz4_tls.h` | `hz4_carry_slot_t`, `hz4_carry_t` 構造体変更 |
| `src/hz4_tls_init.c` | `n=0`, `slot[].head=NULL`, `slot[].page=NULL` で初期化 |
| `core/hz4_collect.inc` | `hz4_carry_consume()`, `hz4_drain_page()` の slot 対応 |

### 変更点（詳細）
- `core/hz4_config.h`: `HZ4_CARRY_SLOTS` のデフォルト定義を追加（default=4）
- `core/hz4_tls.h`:
  - `hz4_carry_t` を **スロット配列**へ変更（`n` 個の {head,page} を保持）
- `src/hz4_tls_init.c`:
  - init で `n=0`、`slot[i].head=NULL`、`slot[i].page=NULL` をクリア
- `core/hz4_collect.inc`:
  - `hz4_carry_consume()` を **slot 消費対応**（`n>0` の限り順次 drain）
  - `hz4_drain_page()` の remainder を **slot に積む**（`n < HZ4_CARRY_SLOTS` のとき）
  - slot が満杯のときだけ requeue/pushback に落とす
  - `carry.n > 0` のときは **segq を触らない**既存最適化を維持

### Fail-Fast ガード

```c
// hz4_carry_consume() 冒頭
#if HZ4_FAILFAST
if (sc >= HZ4_SC_MAX) {
    HZ4_FAIL("carry_consume: sc out of range");
}
if (c->n > HZ4_CARRY_SLOTS) {
    HZ4_FAIL("carry_consume: n overflow");
}
#endif

// hz4_drain_page() remainder 積み込み時
#if HZ4_FAILFAST
if (c->n >= HZ4_CARRY_SLOTS) {
    HZ4_FAIL("carry push: slots full but called");
}
#endif
```

### 実装メモ（箱境界）
- carry は **TLS 内に閉じ込め**、collect の箱だけで完結
- sc ごとに carry を持つ（他 sc への混入禁止）
- Fail‑Fast: `carry.n <= HZ4_CARRY_SLOTS` を常時保証

### A/B
```
HZ4_CARRY_SLOTS=4 (new)
```

### 実装ステップ
1. `hz4_config.h`: `#ifndef HZ4_CARRY_SLOTS` を追加（default=4）
2. `hz4_tls.h`:
   - `hz4_carry_slot_t { void* head; hz4_page_t* page; }`
   - `hz4_carry_t { uint8_t n; hz4_carry_slot_t slot[HZ4_CARRY_SLOTS]; }`
3. `hz4_tls_init.c`: `n=0`, `slot[].head=NULL`, `slot[].page=NULL` 初期化
4. `hz4_collect.inc`:
   - `hz4_carry_consume()` を slot 方式に変更（LIFO で OK）
   - `hz4_drain_page()` で remainder 発生時に slot に積む
5. smoke: `make -C hakozuna/hz4 test`
6. SSOT: T=16/R=90 (16–2048) で A/B

### SSOT 検証手順

```bash
# 1. Build
make -C hakozuna/hz4 clean all

# 2. Test (Fail-Fast 有効)
make -C hakozuna/hz4 test

# 3. Benchmark (T=16 R=90)
for i in 1 2 3; do
  LD_PRELOAD=./hakozuna/hz4/libhakozuna_hz4.so \
    ./hakozuna/out/bench_random_mixed_mt_remote_malloc 16 2000000 400 16 2048 90 65536
done

# 4. perf top3
perf record -g -- env LD_PRELOAD=./hakozuna/hz4/libhakozuna_hz4.so \
  ./hakozuna/out/bench_random_mixed_mt_remote_malloc 16 2000000 400 16 2048 90 65536
perf report --stdio -g none --no-children 2>&1 | grep -E "hz4" | head -5
```

### 結果（SSOT, 2026-01-23）
- T=16 R=90 (16–2048)
- Throughput: **~39–40M ops/s**（P0: ~42.3M から **-5〜7%**）
- perf (HZ4 内 top3, self%):
  1) `hz4_collect` **8.67%**
  2) `hz4_malloc` **4.58%**
  3) `hz4_remote_flush` **3.29%**
- 評価: **collect self はほぼ不変**、throughput 微減 → **NO-GO 候補**（slot 管理 overhead の可能性）

### 記録フォーマット

```
P1 (carry slots)
T=16 R=90
hz4: <ops/s>
perf: hz4_collect <X%>, hz4_malloc <Y%>, hz4_remote_flush <Z%>
notes: <採用/NO-GO>
```

---

## 3) P2: pressure mode で budget 増量

### 狙い
collect 回数を減らし **amortize**

### 仕様案
```
if (segq_head != NULL || segs_popped > 1) obj_budget = 256; else 64;
```

### ガード
- bin 保持上限を越える場合は通常 budget に戻す
- overflow を増やさない

### A/B
```
HZ4_COLLECT_BUDGET_PRESSURE=1 (new)
```

---

## 4) P3: inbox message passing レーン（別レーン）

### 狙い
segq/pageq を迂回して **remote を inbox で一括回収**

### 要点
- flush が owner inbox に batch を投げる
- collect は inbox を `exchange` で回収
- small 専用の **別レーン**として隔離

### 注意
- まず P0–P2 で勝ち筋を確認してから着手
- 実装は research box で隔離

---

## 5) ベンチ（SSOT）

```
ITERS=2000000 WS=400 MIN=16 MAX=2048 R=90 RING=65536
LD_PRELOAD=./hakozuna/hz4/libhakozuna_hz4.so \
  ./hakozuna/out/bench_random_mixed_mt_remote_malloc 16 $ITERS $WS $MIN $MAX $R $RING
```

---

## 6) 記録フォーマット

```
<phase>
T=16 R=90
hz4: <ops/s>
hz3: <ops/s>
delta: <hz4/hz3 - 1>
perf: <top3>
notes: <採用/NO-GO>
```
