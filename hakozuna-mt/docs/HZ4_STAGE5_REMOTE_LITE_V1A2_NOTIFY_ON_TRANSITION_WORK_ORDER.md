# HZ4 Stage5-1A2: Remote-lite v1a2（notifyを空→非空遷移のみに絞る）Work Order

目的:
- Remote-lite v1a2（transition-only notify）で、notify 固定費を削減する。
- **notify を “first remote” のときだけ**に絞り、R=50 退行を抑える。

前提:
- `HZ4_REMOTE_LITE=1` は **既定OFF**（R=90専用 lane でのみ検証・運用）。
- `HZ4_PAGE_META_SEPARATE=1` 必須（remote-lite は compile-time で fail-fast）。
- CPH（CentralPageHeap）在籍ページは PageQ に入れない（disjoint を維持）。

---

## 現状（v1a2の挙動）

実装箇所:
- `hakozuna/hz4/core/hz4_remote_flush.inc`
  - `hz4_remote_free_keyed()` / `hz4_remote_free()` が remote-lite 時に
    - `hz4_page_push_remote_one_tid(..., tid)`
    - `hz4_pageq_notify(page)`（毎回）
    - return

問題:
- PageQ は `pageq_notify()` 内で `atomic_exchange(meta->queued, 1)` を行う。
- 通知が多いと、**queued=1でも毎回 exchange** が走り固定費が増える。

---

## 変更方針（v1a2）

Remote-lite は “push” 自体は継続し、**notify の頻度だけ**を下げる。

狙う条件:
- **空→非空の遷移**（first remote）でのみ notify
- それ以外（すでに queued / 既に remote が溜まっている）は notify しない

実装（CAS成功時の `old==NULL` のときだけ notify / mimalloc型）

利点:
- `queued` への load/exchange を増やさない（notify呼び出し回数のみ減る）
- “first remote” の概念に近い

やること:
1. Remote-lite パス専用に、transition を返す push を用意する
   - 例: `hz4_page_push_remote_one_tid_transitioned(page, obj, tid) -> bool`
   - 実装は `hakozuna/hz4/core/hz4_seg.h`（`HZ4_PAGE_META_SEPARATE` 側）で行う
     - CAS loop の **成功時に使われた old が NULL**なら transitioned=true
2. `hz4_remote_lite_try_push()`（`hz4_remote_flush.inc`）で
   - `transitioned = ...transitioned(...)`
   - `if (transitioned) hz4_pageq_notify(page);`
   - return true
3. CPH disjoint は既存の `meta->cph_queued` チェックを維持（queuedなら legacyへフォールバック）

注意:
- shard 単位の空→非空（page全体で既に別 shard に remote があっても、別 shard の first で transitioned し得る）
  - ただし `pageq_notify()` 自体が `meta->queued` で 0→1 の時のみ enqueue するため、重複 enqueue はしない
  - それでも notify 呼び出し回数が増える可能性はある（最大 HZ4_REMOTE_SHARDS 回/epoch）

---

## 変更ファイル（予定）

- `hakozuna/hz4/core/hz4_seg.h`
  - `hz4_page_push_remote_one_tid_transitioned()` を追加
- `hakozuna/hz4/core/hz4_remote_flush.inc`
  - `hz4_remote_lite_try_push()` を v1a2 に更新（notify を条件付きに）
- （任意）`hakozuna/hz4/src/hz4_os.c`
  - OS_STATS が欲しければ `remote_lite_notify_calls` を追加（まずは不要。SSOTで必要なら後から）

---

## A/B（SSOT）手順

Stage5 テンプレを使う（PreloadVerifyBox + sha256 guard + timeout 付き）。

```sh
cd /mnt/workdisk/public_share/hakmem

RUNS=21 ITERS=1000000 TIMEOUT_SEC=60 \
  A_NAME=base \
  A_DEFS_EXTRA='-O2 -g -DHZ4_RSSRETURN=0 -DHZ4_PAGE_META_SEPARATE=1 -DHZ4_REMOTE_LITE=0 -DHZ4_REMOTE_FLUSH_THRESHOLD=64 -DHZ4_TCACHE_PREFETCH_LOCALITY=2' \
  B_NAME=remote_lite_v1a2 \
  B_DEFS_EXTRA='-O2 -g -DHZ4_RSSRETURN=0 -DHZ4_PAGE_META_SEPARATE=1 -DHZ4_REMOTE_LITE=1 -DHZ4_REMOTE_FLUSH_THRESHOLD=64 -DHZ4_TCACHE_PREFETCH_LOCALITY=2' \
  ./hakozuna/archive/research/hz4_mi_chase_lane/run_mi_chase_stage5_ab_template.sh
```

出力:
- `/tmp/hz4_mi_chase_stage5_ab_<git>_<ts>/SUMMARY.tsv`

判定（Stage5暫定）:
- GO: R=50/R=90 両方で -0.5%以内、片方 +1%以上
- 最優先: **R=50 を落とさない**（remote-heavy以外で壊れると lane 化しても運用が難しい）

---

## Fail-Fast（壊れやすい点）

- CPH/PageQ disjoint:
  - `meta->cph_queued!=0` の場合は remote-lite を使わず legacy に落とす（既存の通り）
- 取り違え防止:
  - Stage5 テンプレは A/B `.so` が同一なら停止（sha256）
  - `TIMEOUT_SEC` で hang を fail-fast
