# HZ4 Design SSOT (T=16/R=90 Focus)

目的:
- T=16/R=90 で mimalloc と競る（勝ち筋を固定化）
- scan を主役にしない（event-driven collect）
- hot path を汚さない（boundary 2 箇所）

非目的:
- hz3 本線の既定経路を置き換えない
- S121 PageQ を復活させない

関連:
- GO台帳: `hakozuna/hz4/docs/HZ4_GO_BOXES.md`
- NO-GO台帳: `hakozuna/hz4/docs/HZ4_ARCHIVED_BOXES.md`
- Debugカウンター運用: `hakozuna/hz4/docs/HZ4_DEBUG_COUNTER_POLICY.md`

---

## 1) Box 構成（SSOT）

## Lanes（SSOT: 取り違え防止）

- stable lane（既定・テスト込み）: `make -C hakozuna/hz4 all_stable`
- perf lane（研究用・libのみ）: `make -C hakozuna/hz4 all_perf`
- larson-stable lane（研究用・libのみ）: `make -C hakozuna/hz4 all_rss_larson_stable_lib`
- Stage5 実験（R=90専用）: Remote-lite v1a2 は **既定laneに入れず**、R=90専用 build/lane でのみ扱う（指示書: `HZ4_STAGE5_REMOTE_LITE_V1A_R90_LANE_POLICY.md`）

bench lane（`run_bench_suite_compare.sh` の公式入口）:
- `HZ4_LANE=mt_default`（MT 既定）
- `HZ4_LANE=st_base`（ST 基準）
- `HZ4_LANE=st_fast`（ST 高速 / Phase16）
- 一括実行: `scripts/run_hz4_three_lanes_dev.sh`

注意:
- `all_perf` は `hz4_cph_disjoint_test` の固定定義と衝突するため **lib-only** 運用に固定する。
- `all_rss` / `all_rss_larson*` は互換のため残している deprecated ターゲット（新規運用では使わない）。

RSS lane の既定パラメータ（Phase 15B 確定）:
- `HZ4_RSSRETURN_THRESH_PAGES=32`
- `HZ4_RSSRETURN_BATCH_PAGES=8`
- `HZ4_RSSRETURN_SAFETY_PERIOD=256`

```
FREE fast
  ├─ local free → page.local_free (owner only)
  └─ remote free → TLS remote buffer (array)
                     └─ RemoteFlushBox (boundary)
                         ├─ page.remote_head[shard] (MPSC)
                         └─ PendingSegQueueBox (notify owner)

MALLOC fast
  └─ tcache pop → empty?
                   └─ RefillBox (boundary)
                       └─ CollectBox (bounded)
                           ├─ pop pending segments
                           ├─ drain hot pages only
                       └─ FallbackBox (central/pageheap)
```

境界（2 箇所固定）:
- RemoteFlushBox: `hz4_remote_flush()`  
- CollectBox: `hz4_collect()` (refill からのみ)

---

## 2) API（境界のみ）

### Remote push path (free 側)
```
static inline void hz4_remote_free_enqueue(hz4_tls_t* tls,
                                           hz4_page_t* page,
                                           void* obj);
void hz4_remote_flush(hz4_tls_t* tls);  // boundary
```

### Collect path (malloc 側)
```
uint32_t hz4_collect(hz4_tls_t* tls,
                     hz4_sc_t sc,
                     uint32_t obj_budget,
                     uint32_t seg_budget);
```

### Fallback path
```
void* hz4_fallback_alloc(hz4_tls_t* tls, hz4_sc_t sc);
void  hz4_fallback_free (hz4_tls_t* tls, hz4_page_t* page, void* obj);
```

---

## 3) データ構造（最小）

### Page header（page-local）
```
typedef struct hz4_page {
  void* free;              // owner only
  void* local_free;        // owner only
  _Atomic(void*) remote_head[HZ4_REMOTE_SHARDS]; // MPSC
  _Atomic(uint32_t) remote_state;                // hint/state
  uint16_t owner_tid;
  uint16_t sc;
} hz4_page_t;
```

推奨:
- HZ4_REMOTE_SHARDS = 4（T=16 の shared-line を割る）
- remote_head[] は cacheline を跨がない配置

### Segment header（pending queue）
```
typedef struct hz4_seg {
  _Atomic(struct hz4_seg*) pending_next;   // MPSC
  _Atomic(uint64_t) pending_page_bits[HZ4_PAGEWORDS];
  _Atomic(uint32_t) queued;                // 0/1
} hz4_seg_t;
```

### TLS（remote buffer）
```
typedef struct hz4_remote_ent {
  hz4_page_t* page;
  void* obj;
} hz4_remote_ent_t;

typedef struct hz4_tls {
  hz4_remote_ent_t rbuf[HZ4_RBUF_CAP]; // fixed array
  uint16_t rlen;
} hz4_tls_t;
```

要点:
- rbuf は固定長配列（n==1 でも next を信用しない）
- flush で page ごとにまとめて publish

---

## 4) PendingSegQueueBox（設計の核）

目的:
- scan を排除して「pending のある seg だけ」collect する

規則:
- `queued` は 0→1 遷移時のみ queue に入れる
- consumer は drain 後に `pending_page_bits` を再チェック
  - 残りがあれば再キュー
  - 無ければ queued=0

再キューの正規手順:
```
if (pending_bits_nonzero)
  queued=1, enqueue(seg)
else
  queued=0
```

---

## 5) CollectBox（bounded）

必須:
- `obj_budget`, `seg_budget` で上限を持つ
- scan fallback は最後の手段（queue empty のときのみ）

狙い:
- pop が支配しないように「軽く回す」

---

## 6) A/B と安全性

ルール:
- hz3 本線と完全分離（hz4 は別ディレクトリ）
- ビルド/LD_PRELOAD で lane 分離
- failfast は境界のみ（one-shot）

---

## 7) 早期勝ち筋（優先順位）

P0: Pending seg queue の正しさ（取りこぼし無し）
P1: Remote flush の batch 化（1 CAS / page）
P2: Shared-line 回避（remote_head[], pending_bits）
P3: Budget 固定（collect が支配しない）

---

## 8) 参考（設計観点）

一般知見（外部）:
- SMT 環境では shared cache line 競合が顕在化しやすい
- page-local + delayed collect がスケールに効く

（外部リンクは本ファイルには残さず、必要なら別資料にまとめる）

---

## 9) 2026-01-22 進捗（Step 5 完了）

テスト:
- test_segq: 7 tests (MT stress: 4 threads x 100 segments)
- test_basic: 6 tests (types/addr/shard/tls/pending bitmap)

確認済み構成:
- HZ4_NUM_SHARDS=64 (power of 2)
- HZ4_REMOTE_SHARDS=4
- HZ4_PAGES_PER_SEG=16 (1MB seg / 64KB page)

実装ルール（確定）:
- qnext は non-atomic (producer writes own node.next only)
- qstate != IDLE の間は retire/evict 禁止
- finish 時に pending_bits が残っていれば再キュー
- n 本固定ループ (while(next) 禁止)
- CAS retry は毎回 tail.next を再設定

---

## 10) Page0 予約 (2026-01-22)

### 設計決定
- page_idx=0 は segment header 専用（予約領域）
- alloc/free/remote/pending の対象外
- usable pages = HZ4_PAGES_PER_SEG - 1 (= 15)

### page_idx の範囲
- 無効: 0
- 有効: 1 .. HZ4_PAGES_PER_SEG-1 (= 1..15)

---

## 11) RSS / Decommit (2026-01-25)

Phase 1 で分かったこと（要点）:
- `madvise(DONTNEED)` は **tcache の intrusive list (`obj->next`) を破壊**するため、
  「空ページ検出 → 即 decommit」は unsafe になり得る。
- `used_count==0` は「アプリ live が 0」を表すべきで、**tcache 上の free obj の参照が残っている限り decommit してはいけない**。

実装方針（Box）:
- PageMetaBox（page meta 分離）で「decommit しても復元できる」前提を作る。
- Decommit 前に **tcache purge/unlink** で参照を断つ（安全化）。
- さらに **delay/hysteresis + budget**（DecommitQueueBox）でスラッシングを抑止する。

入口（作業指示）:
- Phase 1 results: `docs/benchmarks/2026-01-25_HZ4_RSS_PHASE1_RESULTS.md`

### QuarantineBox + PressureGateBox（2026-02-01）
- QuarantineBox は empty page を短時間だけ隔離し、通常ワークロードでも decommit の “候補成熟” を作る（default OFF / opt-in）。
- PressureGateBox は QuarantineBox を **圧がある時だけ**有効化して、`seg_acq` と `ru_maxrss` の分散を抑える（default OFF / opt-in）。
- GO 判定と A/B ログの正: `hakozuna/hz4/results/PRESSUREGATEBOX_RESULTS.md`

### メモリレイアウト
```
Segment (1MB):
+----------------+  offset 0
|   hz4_seg_t    |  (segment header, ~128 bytes)
+----------------+
|   (padding)    |  page0 の残り (~64KB - 128B)
+----------------+  offset 64KB
|   page 1       |  最初の usable page
+----------------+  offset 128KB
|   page 2       |
+----------------+
|   ...          |
+----------------+  offset 960KB
|   page 15      |  最後の usable page
+----------------+
```

### コード影響箇所
- `hz4_pending_set`: page_idx==0 は無視
- `hz4_pending_clear`: page_idx==0 は無視
- `hz4_drain_segment`: bit0 をマスクしてスキップ
- `hz4_page_from_seg(seg, 0)`: segment header を返す（page としては使用禁止）
