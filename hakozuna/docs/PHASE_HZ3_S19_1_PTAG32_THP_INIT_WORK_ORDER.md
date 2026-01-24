# PHASE_HZ3_S19-1: PTAG32 THP / TLB 対策（init-only, hot 0命令）Work Order

目的:
- S17/S18-1で hot 命令列はかなり削れたため、残りの +1% は **TLB / I-cache / 最後の1 load** 領域になりやすい。
- `mixed (16–32768)` の PTAG32（tag32 table）ランダムアクセスで発生しうる dTLB 負けを、**init-only**で改善する。
- hot path に命令を追加しない（Box Theory: Event-only）。

背景:
- S17: PTAG dst/bin direct（GO）
- S18-1: PTAG32 fast lookup（RUNS=30で mixed +2.20% のGO）
- S18-2: flatbin（NO-GO、アーカイブ済み）

参照:
- S17: `hakozuna/hz3/docs/PHASE_HZ3_S17_PTAG_DSTBIN_DIRECT_WORK_ORDER.md`
- S18-1: `hakozuna/hz3/docs/PHASE_HZ3_S18_1_PTAG32_FASTLOOKUP_WORK_ORDER.md`

---

## アイデア

PTAG32（例: 4GB arena なら 1M pages → 4MBテーブル）へのランダムアクセスは、
データキャッシュより **dTLB** が支配になるケースがある。

そこで、PTAG32 mmap 直後に:
- `madvise(tag32, bytes, MADV_HUGEPAGE)` を付与し THP を促す（ヒント）

を行う。成功しなくても安全（ヒントなので）。

hot path には一切変更を入れない（init-only）。

---

## 実装タスク

### S19-1A: compile-time gate

例:
- `HZ3_PTAG32_HUGEPAGE=0/1`（default 0）

### S19-1B: init-only で madvise

適用箇所:
- `hz3_arena_do_init()` の PTAG32 確保直後（`g_hz3_page_tag32` が決まった直後）

順序:
1) `mmap` で PTAG32 領域を確保
2) `madvise(..., MADV_HUGEPAGE)`（失敗しても無視）
3) `memset`（または既存の初期化）でコミット
4) base publish（S18-1 の不変条件を崩さない）

注意:
- `MADV_HUGEPAGE` は Linux 固有。`#ifdef MADV_HUGEPAGE` でガードする。
- hot path で `getenv`/sysconf などは呼ばない（init-onlyなのでOKだが、原則シンプルに）。

---

## A/B 計測

### SSOT (RUNS=10)
- small/medium/mixed が ±0%（回帰なし）
- mixed が +0.5〜+1% でも GO 候補（hot 0命令で得られる改善として妥当）

注意:
- THP はカーネル設定/メモリ断片化/タイミングで効き方が揺れやすいので、最終判定は `RUNS=30` 推奨（少なくとも同条件を複数回）。

### perf stat（mixed）
最小:
- `cycles,instructions`
TLB切り分け:
- `dTLB-load-misses,iTLB-load-misses`（取れる環境のみ）

期待方向:
- `dTLB-load-misses` 減（取れるなら）
- `cycles` 減（instructions はほぼ同等でもOK）

---

## GO/NO-GO

GO:
- small/medium ±0%
- mixed +0.5% 以上、または `cycles`/`dTLB-load-misses` が明確に減る

NO-GO:
- mixed ±0%（動かない）
- small/medium が落ちる

NO-GO の場合:
- revert
- `hakozuna/hz3/archive/research/s19_1_pagetag32_thp/README.md` に SSOT/perf を固定
- `hakozuna/hz3/docs/NO_GO_SUMMARY.md` に追記

---

## 結果（RUNS=30 / perf stat 3x, mixed）

SSOTログ（RUNS=30）:
- baseline（S18-1 FASTLOOKUP + THP=0）: `/tmp/hz3_ssot_6cf087fc6_20260101_192633`
- THP（S19-1, `HZ3_PTAG32_HUGEPAGE=1`）: `/tmp/hz3_ssot_6cf087fc6_20260101_192834`

median（hz3）:
- baseline: small 102.34M / medium 102.30M / mixed 96.55M
- THP:      small 102.26M / medium 104.95M / mixed 99.15M

差分（THP vs baseline）:
- small:  -0.08%（非退行）
- medium: +2.59%
- mixed:  +2.69%

perf stat（mixed, 3 runs）ログ:
- baseline: `/tmp/hz3_s19_1_perf_baseline.txt`
- THP: `/tmp/hz3_s19_1_perf_thp.txt`

要点（mean, THP vs baseline）:
- `dTLB-load-misses`: -17.1%
- `cycles`: +0.36%（この条件では大差なし）

---

## 最終判定（2026-01-01 再検証）

結論: **NO-GO**
- `RUNS=30` の再検証で `mixed` が悪化するケースが出ており、安定した改善として扱えない。
- THP は環境/断片化/カーネル都合の揺れが大きく、効果が再現しない。

アーカイブ:
- `hakozuna/hz3/archive/research/s19_1_pagetag32_thp/README.md`
