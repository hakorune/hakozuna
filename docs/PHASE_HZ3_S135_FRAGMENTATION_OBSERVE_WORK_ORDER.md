# PHASE_HZ3_S135: Fragmentation Observe（partial pages / 断片化の犯人特定）

目的:
- dist_app での RSS gap（hz3 >> tcmalloc/mimalloc）が **large ではなく small/sub4k/medium 側**と判明したため、
  “返せないメモリ（partial pages/断片化）” がどこで発生しているかを **OBSERVE-only** で特定する。
- hot path は触らない（event-only + atexit one-shot）。
- 次の箱（cap/decay/subrelease/粒度変更）を選ぶための「犯人順位」を確定する。

背景（直近の測定）:
- `max_size=32767` にしても RSS が落ちない → 32KiB境界/large は主因ではない。
- `max_size=2048` にしても RSS が高い → small/sub4k/medium の partial pages が濃厚。
- S51（large madvise）/ S55 frozen / S55_3 subrelease は dist_app で RSS を動かせず NO-GO。
  記録: `hakozuna/hz3/docs/PHASE_HZ3_S130_S132_RSS_TIMESERIES_3WAY_RESULTS.md`

---

## S135-0: まずは「既存OBS箱の合成」で通電する（ノーコード）

現状メモ:
- `HZ3_S135_FRAG_OBS` の専用SSOTは **まだ未実装**（ドキュメント先行）。
- したがって、まずは既存の OBSERVE 箱を合成して「partial/空ページの proxy」を取る。
  - small 側: `HZ3_S62_OBSERVE`（small segment の free_bits から free/used pages を数える）
  - segment packing 側: `HZ3_SEG_SCAVENGE_OBSERVE`（pack pool の free_pages / candidate_pages）
  - TLS/segment proxy 側: `HZ3_S55_RETENTION_OBSERVE`（TLS bytes / arena_used_bytes proxy）

出力（既存ログ）:
- `[HZ3_S62_OBSERVE] ...`（small segment の free_pages/used_pages）
- `[HZ3_SEG_SCAVENGE_OBS] ...`（pack pool の free_pages/candidate_pages）
- `[HZ3_RETENTION_OBS] ...`（TLS bytes / arena_used_bytes proxy）

注意:
- これらは RSS の足し上げ内訳ではなく proxy（概算）なので、S130 の RSS timeseries とセットで解釈する。

解釈ルール（重要）:
- RSS/PSS は ground truth、他は allocator 内部の proxy/統計。
- “cand_*” は「返せる可能性がある量」。大きければ S55-3（subrelease）や返済箱の勝ち筋。
- “partial” が支配的なら、subrelease では落ちにくく、**粒度/sizeclass/再利用戦略**の箱が必要。

---

## S135-1: 取得したいメトリクス（優先順位 / 既存ログで代替）

### A) Segment/Page occupancy（本命）

small:
- `HZ3_S62_OBSERVE` の `free_pages/used_pages`（small segment の free_bits proxy）

packing/medium寄り:
- `HZ3_SEG_SCAVENGE_OBS` の `max_free_pages/max_candidate_pages`（contig free proxy）

TLS/segment proxy:
- `HZ3_RETENTION_OBS` の `tls_small_bytes/tls_medium_bytes/arena_used_bytes/pack_pool_free_bytes`

（将来の S135 専用SSOTでやりたいこと）
small/sub4k/medium それぞれで:
- `pages_total`
- `pages_empty`（完全空ページ）
- `pages_decommitted`（DONTNEED 済みページ）
- `pages_partial`（空でも満杯でもないページ）

加えて partial の分布（5bucket）:
- `occ_0`（0%）
- `occ_1_25`
- `occ_25_50`
- `occ_50_75`
- `occ_75_100`

※ “occupancy” の定義は「ページ内の live objects / capacity」。正確さより比較可能性優先（統一が重要）。

### B) “返せる候補量”（次の箱選び）

- `cand_decommit_pages`:
  - segment を保持したまま、空ページを `madvise(MADV_DONTNEED)` できる候補ページ数。
- `cand_unmap_segs`:
  - 完全空 segment で、丸ごと返せる候補（もし設計上可能なら）。

### C) top-N 犯人（5個だけ）

dist_app は size 分布が広いので「一部の class が partial を量産」しがち。
- `top_partial_sc`（partial pages を作っている上位 sc）
- `top_partial_pages`
- （可能なら）`top_tail_waste_bytes`（ページ末尾の余り＝固定廃棄の積み上げ）

---

## S135-2: 実装境界（箱の境界は 1 箇所）

禁止:
- malloc/free hot path に常時分岐や計測を追加しない。

許可（境界）:
- epoch/pressure の event-only 境界で “max tracking”（軽量・budget付き）。
- dump は atexit one-shot。

“budget”:
- segment 全走査は禁止（dist_app のような短いrunでも暴走しないため）。
- 例: `S135_SCAN_SEG_BUDGET=64` など、固定上限を必須。

---

## S135-2: 実行手順（S135 = “OBS合成”）

ビルド（例）:
- `make -C hakozuna/hz3 clean all_ldpreload_scale HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_S55_RETENTION_OBSERVE=1 -DHZ3_SEG_SCAVENGE_OBSERVE=1 -DHZ3_S62_OBSERVE=1'`

実行:
- `measure_mem_timeseries.sh` を回し、stderr の `[HZ3_S62_OBSERVE] / [HZ3_SEG_SCAVENGE_OBS] / [HZ3_RETENTION_OBS]` を保存する。
  - 注意: `[HZ3_S62_OBSERVE]` は short bench だと thread destructor が走らず欠けやすいので、最新版では init 時点で atexit を登録し、exit 時に best-effort snapshot を取る（ログが出ない場合はビルドが古い可能性）。

GO/NO-GO（観測箱なので “ログが出て比較可能” が GO）:
- crash なし / hang なし。
- 3本のログが出る（atexit one-shot が出る）。
- dist_app の max_size を変えても、ログが “全ゼロ” にならない（特に `HZ3_S62_OBSERVE` は dtor で1回は走る）。

---

## S135-3: 次の分岐（観測結果→次の箱）

1) `pages_empty` / `cand_decommit_pages` が大きい:
- **返せる余地がある** → S55-3 系（subrelease）を “効くように” 調整する価値あり。
  - ただし dist_app で NO-GO だったため、「候補が小さい」可能性もある。S135で確定してから。

2) `pages_partial` が支配 + occupancy が 1–25% に偏る:
- **断片化支配** → cap/decay では限界。粒度・sizeclass・segment reuse の設計箱が必要。

3) top_partial_sc が少数に偏る:
- **sc 特化**（その class の run/page 形状、cap、reclaim）で勝ち筋。

4) top_partial_sc が広い:
- **汎用設計**（segment 内再配置/packing/active-set など）を検討。

---

## S135-4: 実測ログ（2026-01-17 / dist_app）

### max_size=32768

- `[HZ3_S62_OBSERVE] dtor_calls=0 scanned_segs=3 candidate_segs=1 total_pages=255 free_pages=9 used_pages=246 purge_potential_bytes=36864`
- `[HZ3_RETENTION_OBS] calls=1 tls_small_bytes=875744 tls_medium_bytes=1474560 arena_used_bytes=3145728 pack_pool_free_bytes=618496`
- `[HZ3_SEG_SCAVENGE_OBS] calls=0 max_candidate_pages=0 ...`

### max_size=2048

- `[HZ3_S62_OBSERVE] dtor_calls=0 scanned_segs=3 candidate_segs=2 total_pages=510 free_pages=254 used_pages=256 purge_potential_bytes=1040384`
- `[HZ3_RETENTION_OBS] calls=1 tls_small_bytes=909344 tls_medium_bytes=45056 arena_used_bytes=3145728 pack_pool_free_bytes=999424`
- `[HZ3_SEG_SCAVENGE_OBS] calls=0 max_candidate_pages=0 ...`

### 解釈（暫定）

- max2048 では free_pages が **~1MB 相当**あり、small 側の “返せる余地” がある。
- max32768 では free_pages がほぼゼロに近く、**partial pages 支配**の可能性が高い。
- `arena_used_bytes` が両方で一定（3.1MB）→ segment 開き過ぎではなく **中身の詰まり方（断片化）**が主因候補。
- `pack_pool_free_bytes` は 0.6–1.0MB 程度 → packing/active-set の効き具合を次で確認。

### 次の分岐（更新）

1) **max2048 の free_pages を “実際に返せるか” をテスト**
   - S62 retire/purge を **single-thread 安全ゲート**で有効化できるか検討。
2) **max32768 の partial 支配を定量化**
   - sizeclass 別の partial 発生（top5）を出せる観測箱（S135-1B）を追加する。

---

## S62-1A: AtExitGate 結果（2026-01-17）

実測（dist_app / max2048）:
- ops/s: +0.9%
- RSS mean: -0.24%（ほぼ差なし）
- RSS max: 0%
- S62 実行ログ: `pages_purged=254 (1MB)` / `madvise_calls=1` を確認

解釈:
- **S62 は atexit で動作しているが、RSS timeseries（実行中のみ）には反映されない**。
- したがって S62-1A は「exit時の掃除」用途に限定され、**実行中のRSS削減には効かない**。

結論:
- S62-1A は debug/clean-up 用として保持（研究箱）。
- RSS 削減の本命は **S135-1B（sizeclass別 partial 観測）** に移行。

---

## S137: SmallBinDecayBox 実測（2026-01-17）

目的:
- top_sc=9..13 の small v2 bin だけ epoch で trim し、partial を減らす。

設定:
- `HZ3_S58_TCACHE_DECAY=1`
- `HZ3_S137_SMALL_DECAY=1`（FIRST=9 / LAST=13 / TARGET=0=refill_batch）

結果（dist_app / MEM_TS）:
- S137 OFF:
  - max=32768: mean_kb=14028.8 p95_kb=16128
  - max=2048: mean_kb=13952  p95_kb=16000
- S137 ON:
  - max=32768: mean_kb=14259.2 p95_kb=16256（増加）
  - max=2048: mean_kb=13747.2 p95_kb=15872（微減）

補足:
- S58 stats は `adjust_calls=0` で動作していない（epoch そのものが発火していない可能性）。
- S137 の効果は **max=32768 で悪化 / max=2048 で微小改善** → 現状は NO-GO（または “動作不確定で保留”）。

次の分岐（提案）:
1) **epoch を確実に動かす検証**（`HZ3_S134_EPOCH_ON_SMALL_SLOW=1`）で S137 を再評価
2) それでも改善しない場合は **S135-1C（full pages / tail waste top5）へ移行**

---

## S69/S135 既知の注意点（2026-01-17）

S69 live_count triage:
- fast path（`hz3_free_try_s113_segmath` / `hz3_free_try_ptag32_leaf`）で **live_count の減算が欠けていた**ため、inc/dec が不一致だった。
- fast path で `hz3_s69_live_count_dec()` を追加し、`inc≈dec` が一致することを確認。

S12 stats の“0表示”について:
- single-thread のベンチでは thread destructor が走らないため、`[S12_V2 stats]` が **見かけ上 0** になり得る。
- S69 の `inc/dec` が一致していれば、free v2 経路は動いていると判断して良い。
