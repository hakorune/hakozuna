# PHASE_HZ3_S62-4: Scale Lane Default ON（S62 常時ON反映）

Status: DONE

Date: 2026-01-09  
Track: hz3  
Previous:
- S62-3: AlwaysOnCandidate **GO**（SSOT ±2%、churn +3%）

---

## 0) 目的

S62（small_v2 retire/purge）を **scale lane の既定**に反映する。
hot path 0 コストのまま、常時ONに近づける。

sub4k は MT hang 既知のため **既定OFF** を維持。

---

## 1) 変更点（1 箇所）

`hakozuna/hz3/Makefile` の `HZ3_SCALE_DEFS` に追記:

```
-DHZ3_S62_RETIRE=1 \
-DHZ3_S62_PURGE=1 \
```

※ `HZ3_SCALE_DEFS` は scale lane と mem lanes に共通で適用される（意図通り）。

---

## 2) ビルド確認

```
make -C hakozuna/hz3 clean all_ldpreload_scale
```

---

## 3) SSOT 再確認（任意）

```
RUNS=10 ./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh
```

GO 条件:
- small/medium/mixed が **±2%以内**（S62-3 の結果と同等）

結果（2026-01-09）:
- logs: `/tmp/hz3_ssot_cc0308ea0_20260109_155559`
- median ops/s: small=112219247.16 / medium=113771823.71 / mixed=107385839.325
- 判定: PASS（±2%）

---

## 4) ドキュメント更新

- `CURRENT_TASK.md` に「S62 default ON（scale lane）」を追記
- `hakozuna/hz3/docs/BUILD_FLAGS_INDEX.md` にこの指示書を追加（済みならスキップ）

---

## 5) リスク / 留意点

- `HZ3_S62_OBSERVE` は default OFF のまま（stats-only）
- `HZ3_S62_SUB4K_RETIRE` は default OFF のまま（MT hang 回避）

---

## 6) 結果ログ（2026-01-09）

```
[HZ3_S62_PURGE] dtor_calls=1 scanned_segs=5 candidate_segs=1
  pages_purged=255 madvise_calls=1 purged_bytes=1044480
[HZ3_S62_RETIRE] dtor_calls=1 pages_retired=204 objs_processed=695
```

メモ:
- scale lane 既定で S62 retire/purge が発火することを確認

---

## 7) 補足（S62 効果の最新まとめ）

```
pages_retired=46840
pages_purged=47430
purged_bytes=185 MB
RSS delta=+128 KB（ほぼ0）
```

メモ:
- thread-exit で purge が集中するため、計測窓によって RSS 変化が見えにくい
- 長時間稼働での “膨張抑制” に効く箱として評価する
