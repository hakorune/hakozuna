# HZ4 Phase 6: ChatGPT Pro Packet (Design Consult)

目的:
- hz4 の **T=16 R=90** を改善するための設計相談用まとめ
- perf で事実を確定したうえで、次の箱設計を決める

---

## 0) 最新スナップショット（2026-01-23）

### P3 inbox lane（default ON）
- **+60–77% 改善** → GO（T=16/R=90, 16–2048）
- T依存性（R=90, 16–2048）:  
  T=4 **49.0M** vs hz3 **66.3M**（-26%）  
  T=8 **61.4M** vs hz3 **68.3M**（-10%）  
  T=16 **69.0M** vs hz3 **78.7M**（-12%）
- R依存性（T=16, 16–2048）:  
  R=0 **256.7M** vs hz3 **361.5M**（-29%）  
  R=50 **89.4M** vs hz3 **96.4M**（-7%）  
  R=90 **70.2M** vs hz3 **75.4M**（-7%）

### full‑range（16–65536）
- LargeFree 修正で crash 解消
- MT remote (T=16 R=90): hz4 **1.66M** vs hz3 **1.76M**（ratio **0.94**）

### TLS Direct Access（GO）
- `HZ4_TLS_DIRECT=1`（default ON）
- Baseline: **~86.8M** → **~94.4M**（+8.8%）
- hz4/hz3 ratio: **0.79 → 0.86**

### NO‑GO まとめ
- **Page Tag Table**: tag lookup が置換ではなく追加 → 退行
- **Remote Flush Page‑Bucket**: hash 偏り + probe で **-3.4%**
- **TLS Struct Merge**: carry 遠方化 + reg pressure で **-0.7%**
- **P1 carry slots**: throughput 微減で NO‑GO

---

## 1) 直近 SSOT（winning set）

**Winning set 固定**
- `HZ4_TLS_INITIAL_EXEC=1`
- `HZ4_SEG_ZERO_FULL=0`
- `HZ4_REMOTE_FLUSH_BUCKETS=64`
- `HZ4_REMOTE_FLUSH_FASTPATH_MAX=8`

**full‑range**
- MT remote (16–65536, T=16 R=90): **6.335M ops/s**
- ST dist_app (16–65536): **55.47M ops/s**

**small range (T=16 R=90, 16–2048)**
- hz4: **35.9M ops/s**（SegQ O(1) 後）
- hz3: **67–83M ops/s**（delta -50% 付近）

---

## 2) perf top3（T=16 R=90, 16–2048）

- `hz4_collect` **26.45%**
- `hz4_remote_flush` **4.13%**
- `hz4_malloc` **3.61%**

補足:
- `__tls_get_addr` と `memset` は閾値以下まで低下（`hz4_tls_get` ~0.26%）

---

## 3) 直近の改善

- **SegQ O(1) push_list** 実装で +6.5%（collect 33% → 23%）
- TLS 初期化モデルの固定で +2.1%
- segment 全体 memset OFF で +7.3%
- remote_flush パラメータ改善は +0.36%（小幅）

---

## 4) Box Theory 制約

- Box 分離を維持（Small/Mid/Large, Collect, RemoteFlush, SegQ）
- 境界は 1 箇所に固定（route/collect/flush）
- A/B は compile-time（`-D` / `#ifdef`）
- Fail‑Fast を維持

---

## 5) 現在の疑問・相談したいこと

1) **R=0（local path）の -29% をどう詰めるか**
   - hz4 の設計哲学（page header 中心）を崩さずに効く策は？
2) **inbox lane の “最後の数％” を削る案**
   - remote_flush / collect の分岐削減やデータ配置改善はあるか
3) **hz3 に近づける low‑risk 改善**
   - TLS/tcache/metadata の局所性で取りうる設計の工夫は？

---

## 6) 参考ログ

- perf data: `/tmp/hz4_perf.data`
- perf record: `/tmp/hz4_perf_record.out`
- full‑range MT remote: `/tmp/hz4_win_fullrange_mt_remote.out`
- dist_app ST: `/tmp/hz4_win_dist_app_st.out`

---

## 7) Update (2026-01-23)

### 最新 SSOT（small range, T=16 R=90, 16–2048）
- **P0 (pageq sc bucket 化)**: hz4 **~42.3M**、`hz4_collect` **8.78%**（P0 前 23.11%）
- **P1 (carry slots)**: hz4 **~39–40M**、`hz4_collect` **8.67%** → **NO‑GO 候補**

### hz3 vs hz4（T=16 R=90, 16–2048）
- hz4 runs: **41.5M / 41.1M / 40.1M**（median ~41M）
- hz3 runs: **70.1M / 75.8M / 94.4M**（median ~76M）
- ratio: **~0.54**（hz3 が **~1.8x** 速い）

### TLS initial-exec 効果
- IE=0 → IE=1: throughput **~41M → ~43M**
- `__tls_get_addr`: **6.52% → 消滅**
- `hz4_tls_get`: **0.92% → 0.33%**

### perf top（self%）
hz3:
- `bench_thread` 30.01%
- `hz3_owner_stash_pop_batch` 10.62%
- `hz3_free` 6.69%
- `hz3_remote_stash_flush_budget_impl` 6.63%
- `hz3_malloc` 4.67%

hz4 (IE=0):
- `hz4_collect` 8.39%
- `bench_thread` 6.69%
- `__tls_get_addr` 6.52%
- `hz4_malloc` 4.08%
- `hz4_remote_flush` 3.20%

hz4 (IE=1):
- `bench_thread` 11.48%
- `hz4_collect` 9.28%
- `hz4_malloc` 3.67%
- `hz4_remote_flush` 3.54%

### P3 inbox/message‑passing 方針（最小形）
- **owner × sizeclass inbox** に intrusive list を push（message node なし）
- 境界: **remote_flush → inbox publish**（hot path は不変）
- collect: **inbox exchange → out[] へ詰めるだけ**（余りは TLS stash）
- A/B: `HZ4_REMOTE_INBOX=1` compile-time
- Fail‑Fast: page/owner/sc/align 範囲チェック（HZ4_FAILFAST のみ）
- NO‑GO: **+5% 未満**なら撤退、**+10–15%** なら採用検討
