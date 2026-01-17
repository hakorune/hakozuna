# PHASE HZ3 S123: CountlessBinsBoundedOpsBox（hot から count RMW を消す）— Work Order

目的（ゴール）:
- hot path（`hz3_binref_pop/push` など）から `(*ref.count)--/++` の **RMW（Read-Modify-Write）依存チェーン**を排除する。
- 代わりに event/slow path 側は「full walk 禁止」＋「上限付き（bounded）」でコストを制御し、LAZY_COUNT が負ける原因を潰す。

背景（敗因の整理）:
- `HZ3_BIN_LAZY_COUNT=1`（= hot から count 更新を消す）は方向性として正しいが、現状は
  - `hz3_inbox_drain()` で `hz3_bin_count_walk(..., HZ3_BIN_HARD_CAP)` が高頻度
  - epoch の trim が detached の tail/n を **全長 walk** しがち
  - S58 decay が “len を正確に読む” 前提で、walk が増える
 となり、イベント側の walk が帳消しになる。

箱の境界（1箇所）:
- 本箱は「count を hot から消す」＝ `HZ3_BIN_LAZY_COUNT=1` を成立させるための、
  **event/slow 側の境界だけ**を最小差分で整える箱。

---

## フラグ（A/B）

**本箱は既定 OFF**（A/B で勝ち筋が取れたら lane 化して隔離する）。

推奨 A/B の基本:
- `HZ3_BIN_COUNT_POLICY=2`（= `HZ3_BIN_LAZY_COUNT=1`）
- 追加の bounded 化は S123 の sub-flag で段階投入（下記）。

（注）実装の都合で sub-flag 名は後から決めてもよい。まずは “段階投入” が重要。

S123 sub-flags（段階投入）:
- `HZ3_S123_INBOX_DRAIN_ASSUME_EMPTY=0/1`
- `HZ3_S123_INBOX_DRAIN_FAILFAST=0/1`（debug 推奨）
- `HZ3_S123_TRIM_BOUNDED=0/1`
- `HZ3_S123_TRIM_BATCH=...`（例: 64）
- `HZ3_S123_S58_OVERAGE_BOUNDED=0/1`

---

## 実装ステップ（段階投入）

### S123-0: 前提確認（no-variant vs variant を先に取る）

- 主戦場: `mixed` / `dist_app`（xmalloc-testN は参考/回帰）
- A/B は必ず `no-variant` vs `variant` を最初に取る（“退行している実装の最適化” を防ぐ）

### S123-1: inbox drain から walk を消す（bin empty 前提）

対象:
- `hakozuna/hz3/src/hz3_inbox.c` の `hz3_inbox_drain()`

狙い:
- `HZ3_BIN_LAZY_COUNT` のときの `hz3_bin_count_walk()` を廃止し、fill は固定上限で行う。

前提（成立しやすい）:
- alloc slow に入る時点で対象 bin は概ね empty（head==NULL）。
  - `hz3_alloc_slow()` は hot path miss（local bin empty）後に呼ばれる。

実装案:
- debug のみ fail-fast: `bin->head != NULL` なら abort（境界違反を早期に露出）
- fill する数は上限 `HZ3_BIN_HARD_CAP`（first 以外を bin に押し込む）で固定し、余りは従来どおり central へ。

### S123-2: epoch trim を chunk 化（detached 全長 walk 禁止）

対象:
- `hakozuna/hz3/src/hz3_epoch.c` の `hz3_bin_trim()`

狙い:
- `HZ3_BIN_LAZY_COUNT` の “detached の tail/n を全長 walk” を禁止し、
  1回の epoch で返す excess を `TRIM_BATCH` に上限設定する（複数 epoch で収束させる）。

実装案:
- `target` までは通常どおり切る（最大 target walk）
- `target->next` から **最大 TRIM_BATCH** だけ切り出して central へ返す（最大 TRIM_BATCH walk）
- excess が大きい場合でも event の最悪コストは固定

### S123-3: S58 overage 判定を “正確len” から “>max_len 判定だけ” に変更（bounded）

対象:
- `hakozuna/hz3/src/hz3_tcache_decay.c` の `hz3_s58_adjust_targets()`

狙い:
- `len` の正確値ではなく「`len > max_len` か？」だけで overage を判定（bounded walk）。

実装案:
- helper: `has_more_than(head, max_len)`（最大 `max_len+1` の walk）
- `len` は “必要最低限” だけにする（lowwater/target 更新は次フェーズで再設計）
  - まずは overage shrink の判定だけ bounded にし、挙動を大きく変えない。

---

## A/B（推奨）

最低スイート（主戦場）:
- `hakozuna/hz3/scripts/run_bench_hz3_ssot.sh`（mixed/小中/混在）
- `hakozuna/out/bench_random_mixed_malloc_dist`（dist_app）

回帰検知（参考）:
- xmalloc-testN（remote-heavy を増幅するため参考）

GO/NO-GO:
- GO: mixed/dist_app が +1% 以上、かつ -1% 超の明確な回帰なし
- NO-GO: mixed/dist_app が -1% 超退行、または perf で event 側の walk が支配的に増える

---

## 結果（2026-01-15）

結論: **NO-GO（scale 既定には入れない）**。

計測（mixed median / dist_app）:
- Step0 基準: mixed `108,573,738` / dist_app `37,903,591.55`
  - log: `/tmp/hz3_ssot_33f7eb4d2_20260115_234331`
- Step1 `HZ3_BIN_COUNT_POLICY=2`: mixed `109,193,382` / dist_app `38,123,317.70`
  - log: `/tmp/hz3_ssot_33f7eb4d2_20260115_234558`
- Step2 +S123-1（inbox assume empty）: mixed `108,728,397` / dist_app `37,444,849.03`（dist_app **-1.21%**）
  - log: `/tmp/hz3_ssot_33f7eb4d2_20260115_234649`
- Step3 +S123-2（trim bounded）: mixed `110,677,521` / dist_app `38,081,952.78`
  - log: `/tmp/hz3_ssot_33f7eb4d2_20260115_234742`
- Step4 +S123-3（S58 overage bounded）: mixed `110,827,040` / dist_app `37,790,376.66`
  - log: `/tmp/hz3_ssot_33f7eb4d2_20260115_234837`

何がダメだったか（要点）:
- **勝ち筋が “mixed 側だけ”**で、dist_app が +1% に届かない（scale の勝ち筋にならない）。
- **S123-1（inbox assume empty）は dist_app で明確に回帰** → 「bin が empty のときだけ drain される」という前提が dist_app では崩れる可能性が高い。
- `HZ3_BIN_COUNT_POLICY` は Makefile 既定で `-D` 済みのため、A/B では `-UHZ3_BIN_COUNT_POLICY` を付けて上書きが必要だった。
- 実装/計測メモ:
  - `HZ3_S123_INBOX_DRAIN_FAILFAST=1`（debug）で dist_app を回し、**bin が empty でない drain が起きているか**を即時に露出できる。
  - `HZ3_BIN_COUNT_POLICY=2` のビルドでは、`-Werror` 回避のため unused になる引数等に `(void)` を付与した（`hakozuna/hz3/include/hz3_tcache.h`, `hakozuna/hz3/src/small_v2/hz3_small_v2_refill.inc`）。

再現メモ（Step 定義）:
- Step1: `-UHZ3_BIN_COUNT_POLICY -DHZ3_BIN_COUNT_POLICY=2`
- Step2: Step1 + `-DHZ3_S123_INBOX_DRAIN_ASSUME_EMPTY=1`
- Step3: Step2 + `-DHZ3_S123_TRIM_BOUNDED=1 -DHZ3_S123_TRIM_BATCH=64`
- Step4: Step3 + `-DHZ3_S123_S58_OVERAGE_BOUNDED=1`
