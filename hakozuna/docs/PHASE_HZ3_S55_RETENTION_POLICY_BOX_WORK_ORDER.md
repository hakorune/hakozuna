# PHASE_HZ3_S55: RetentionPolicyBox（FROZEN: RSS削減 + 速度維持）

目的:
- `tcmalloc` と比べて大きい RSS gap を、**large cache（S53）以外**の保持（TLS/central/segment滞留）で詰める。
- hot path は汚さない（event-only の境界だけ）。
- まず **OBSERVE（統計）**で“どこが保持しているか”を確定し、次に **FROZEN（固定ポリシー）**で上限/trim を入れる。

前提:
- S54 OBSERVE では segment/medium の contig free pages の `max_potential_bytes` が ~1MB 級 → **page-scavenge 単体は主力になりにくい**。
- S53 mem-mode は workload により RSS 差が小さい（平均 -0〜-2% 程度）→ RSS 主因は別。

---

## S55-0: 目標（GO/NO-GO）

まずの GO（FROZEN）:
- `mstress` の mean RSS を **-10% 以上**（vs hz3 baseline）に寄せる。
- `mstress` の速度は **-2% 以内**（できれば ±0%）。
- SSOT（small/medium/mixed）は **±2%** 以内（回帰なし）。

NO-GO:
- RSS が -5% 未満しか動かないのに速度が落ちる。
- hot path に固定費が入ってしまう（設計違反）。

---

## S55-1: OBSERVE（統計だけ、madviseなし）

### 目的
「RSS がどの層で保持されているか」を allocator 内部統計で切り分ける。

重要（解釈の前提）:
- `[HZ3_RETENTION_OBS]` の各フィールドは **保持/配置の proxy（概算）**であり、RSS そのものではない。
- **足し上げて RSS に一致させる用途ではない**（同一ページ/同一オブジェクトが複数の“箱”に見える、または観測範囲外がある）。
- したがって S130 の RSS gap を詰めるときは、`[HZ3_RETENTION_OBS]` を「犯人候補の順位付け」に使い、RSS/PSS は別途 timeseries（または `/proc/self/smaps_rollup`）で確認する。

前提（重要）:
- `HZ3_TCACHE_SOA_LOCAL=1` の場合、TLS bin の `count` は hot/slow の両方で整合している必要がある。
  - SoA の slow path が `count` 更新を忘れると underflow して観測が破綻するだけでなく、trim/flush 等の挙動も壊れる。
  - 例: `hz3_small_v2_alloc_slow()`（SoA）で `local_count` 更新漏れがあると、後続の `hz3_binref_pop()` で `count--` が発生して wrap する。

### 追加する one-shot 出力（例）

```
[HZ3_RETENTION_OBS] tls_small_bytes=... tls_medium_bytes=... owner_stash_bytes=... central_medium_pages=... seg_used_slots=...
```

### 候補メトリクス（最小セット）

- TLS:
  - small/local bins: `count * class_size` の概算
  - medium/local bins: `count * pages * 4KB` の概算
- owner stash（S44）:
  - `count` が無い場合は “drainした個数” のカウンタを event-only で取る（近似でOK）
- central:
  - medium: free list の run 数（pages換算）
  - small: まずは後回し（影響が大きい場合のみ）
- segment:
  - `arena_used_slots`（確保済 segment 数）
  - `pack_pool segments/free_pages`（S54の観測を再利用してもよい）

### 収集タイミング（event-only）

優先:
- `hz3_epoch_force()`（cold / 定期、hotには入れない）
- S46 pressure handler（発火時のみ）

出力:
- atexit one-shot（既存パターン踏襲）

注意（短いベンチ / single-thread）:
- epoch/pressure が発火しないと `calls=0` になり得る。現状は atexit dump 時に **best-effort の snapshot** を 1 回取って埋める設計。
- atexit は TLS destructor / 片付けの順序と競ることがあり、**ピークではなく終了時点寄り**の値になる可能性がある（ピーク追跡は別箱）。

### OBSERVE 実測（2026-01-06）

`HZ3_S55_RETENTION_OBSERVE=1`（scale lane）での観測例:

- `mstress`（T=32）
  - `arena_used_bytes` が支配的（約 97.6%）
  - `tls_small/medium` や `central_medium` は “見えるが minor”
- `bench_random_mixed_mt_remote_malloc`（T=32, WS=400, 16–32768, R=50）
  - `arena_used_bytes` が支配的（約 99.0%）
  - `central_medium_bytes` は数百MB規模だが、割合としては 1% 程度

解釈（注意）:
- `arena_used_bytes = used_slots * HZ3_SEG_SIZE` は **arena slot（segment）の使用量**の proxy。
  - RSS（物理）そのものではなく、segment の確保・保持（VA/ページテーブル等を含む）を示す。
  - tcmalloc の RSS 差分が大きい場合、segment の「使い過ぎ（開き過ぎ / 返し不足）」が疑わしい。

S130 との突き合わせ（推奨手順）:
1) 同じコマンドで `system malloc`（LD_PRELOAD なし）も timeseries を取って、ベンチ/プロセス固定費（stack/lib/bench-side）を差し引けるようにする。
2) hz3/tcmalloc/mimalloc の RSS/PSS と `[HZ3_RETENTION_OBS]` を並べ、`tls_*` が効いているのか／segment proxy が支配か／large/metadata が疑わしいかを判断する。

---

## S55-2: FROZEN（固定ポリシーで trim）

### 目的
“持ち過ぎ”の層から下へ落とし、RSS を減らす（速度回帰は最小）。

### 境界（1箇所）
S55 の trim は必ず **event-only** に固定する:
- `hz3_epoch_force()` 内
  - ここで「一定budgetだけ」実行（暴走防止）

### 最初の対象（推奨順・最新版）

0) **Admission Control（segment “開き過ぎ”抑制）**
- S55-1 で `arena_used_bytes (= used_slots * HZ3_SEG_SIZE)` が支配的だったため、まずは **segment を“開く頻度”を下げる**のが本線。
- 手段: **OpenDebtGate（借金ゲート） + 2段 watermark**。
  - 新規 segment を開いたら `debt++`、segment を返せたら `debt--`。
  - SOFT/HARD watermark で “開く直前だけ” S49/S47/S45 を強化する（hot 0コスト）。
- 指示書: `hakozuna/hz3/docs/PHASE_HZ3_S55_2_RETENTION_FROZEN_OPEN_DEBT_WORK_ORDER.md`

### S55-2 実測（2026-01-06, `mstress`）: RSS目的では NO-GO

- level 遷移（L0→L1→L2）は成立（MBレンジ watermarks で確認）
- ただし `mstress` の mean RSS は概ね **-0〜-1% 程度**で、目標（-10%）に届かない
- “開く頻度を下げる（Admission Control）”だけでは steady-state の RSS が動かない

### S55-2B（Epoch Repay / ReturnPolicy）実測: RSS目的では NO-GO

- `hz3_epoch_force()` 境界で L2 のとき定期返済を試すが、`mstress` では `gen_delta==0` が継続
  - 返却できる full-free segment が作れていない（断片化）
- 結果: RSS はほぼ動かない（NO-GO）

1) **medium central → segment へ demote（補助）**
- central に滞留している medium run を pop して `hz3_segment_free_run()` で segment の free_bits に戻す（S45 Phase2 と同型）。
- OpenDebtGate の “返済” として HARD 時に 1回だけ踏む候補（暴走防止の budget を必須）。

2) **TLS medium の cap を下げて central へ flush**
- TLS が持ち過ぎている場合のみ、epoch でまとめて中央へ返す（hotは触らない）。

3) small は後回し
- small は intrusive freelist で `madvise` と相性が悪い（リスト破壊）。
- まずは medium/tls/central の滞留制御で勝ち筋を見る。

### 典型的な固定ポリシー（例）

- `HZ3_S55_MEDIUM_CENTRAL_FREE_PAGES_SOFT=...`
  - これを超えたら `DEMOTE_BUDGET_PAGES` 分だけ segment に戻す。
- `HZ3_S55_DEMOTE_BUDGET_PAGES=...`（default 1024 pages など）
  - 1回の epoch でやりすぎない（速度保護）。

---

## フラグ（compile-time）

最低限:
- `HZ3_S55_RETENTION_OBSERVE=0/1`（default 0）
- `HZ3_S55_RETENTION_FROZEN=0/1`（default 0）

初期ノブ:
- `HZ3_S55_DEMOTE_BUDGET_PAGES=...`
- `HZ3_S55_MEDIUM_CENTRAL_FREE_PAGES_SOFT=...`

OpenDebtGate（S55-2 本線）:
- `HZ3_S55_OPEN_DEBT_GATE=0/1`（必要なら分割）
- `HZ3_S55_WM_SOFT_BYTES` / `HZ3_S55_WM_HARD_BYTES` / `HZ3_S55_WM_HYST_BYTES`
- `HZ3_S55_DEBT_LIMIT_SOFT` / `HZ3_S55_DEBT_LIMIT_HARD`

---

## ベンチ手順（SSOT）

外部RSSの時系列（mean/p95）:
- `hakozuna/hz3/scripts/measure_mem_timeseries.sh`

比較（例）:
- `mstress`（T=32）
- `bench_random_mixed_mt_remote_malloc`（T=32, 16–32768, R=50）

---

## 実装手順（最小差分）

1) S55-OBS: 統計だけ入れる（atexit one-shot）
2) S55-FROZEN: medium central demote を budget 付きで入れる（epoch_force のみ）
3) A/B: OBSERVE→FROZEN の順で回す（SSOT + RSS mean/p95）

（更新）:
4) S55-FROZEN(OpenDebtGate): segment open 直前の tries boost + 1回返済を入れ、`arena_used_bytes` の伸びを止める。

（更新 2026-01-06）:
5) S55-2/2B は RSS 目的では NO-GO。次は “full-free segment が作れない断片化” を前提に設計を切る（S55-3）。
