# PHASE_HZ3_S59-2: Idle Purge Comparison（mimalloc/tcmalloc/hz3）

Date: 2026-01-09  
Track: hz3  
Purpose: “RSSが落ちる条件”を固定し、hz3 の RSS 施策（S58/S60 など）の比較基準を作る。

---

## 1) 結果（idle=10s, threads=4）

| Allocator | Condition | RSS |
|---|---|---:|
| mimalloc | default | ~476MB |
| mimalloc | `MIMALLOC_PURGE_DELAY=0` | ~2.3MB |
| tcmalloc | default | ~470MB |
| hz3 | baseline（S61=0） | ~566MB→~1676MB（cycle進行で増加） |
| hz3 | `HZ3_S61_DTOR_HARD_PURGE=1` | ~138MB→~393MB（-76% / cycle3基準） |

重要:
- mimalloc は **デフォルトでは idle だけで purge しない**が、`MIMALLOC_PURGE_DELAY=0` で即 purge が可能。
- tcmalloc は default ではこの条件で RSS が落ちない（別途 “RSSを落とす設定” を見つけた場合のみ比較に採用）。
- hz3 は S61（thread-exit hard purge）で idle RSS を大きく削減できたが、mimalloc `PURGE_DELAY=0` との差分（~138MB残り）は small/sub4k 側が未対応。

---

## 2) 再現コマンド（例）

ベンチ:
- `bench_burst_idle_v2`（idle=10s, keep_ratio=0, threads=4）

補足（可視化オプション）:
- `idle_tick_ms`: idle 中に `hz3_epoch_force()` を周期呼び出し（hz3 の purge/tick を進める）
- `dump_mode`: `0=off / 1=smaps_rollup / 2=smaps`
- `S59_2_DUMP_PATH`: smaps 出力の保存先（stderr ではなくファイルへ）

mimalloc（reference: purgeが効くケース）:

```sh
MIMALLOC_PURGE_DELAY=0 LD_PRELOAD=/mnt/workdisk/public_share/hakmem/libmimalloc.so \
  ./hakozuna/bench/linux/hz3/bench_burst_idle_v2 200000 10 3 0 4
```

hz3（S58=1; purge無し）:

```sh
LD_PRELOAD=/mnt/workdisk/public_share/hakmem/libhakozuna_hz3_scale.so \
  ./hakozuna/bench/linux/hz3/bench_burst_idle_v2 200000 10 3 0 4
```

※ RSS の比較は “print の MB” ではなく、`measure_mem_timeseries.sh` の CSV を正とすること。

---

## 3) 解釈（SSOT）

この結果により、hz3 の RSS 施策は次の基準で評価できる:

- mimalloc default と比較して “勝てるか” ではなく、
- mimalloc `MIMALLOC_PURGE_DELAY=0` のような **即 purge が可能なケース**に対して、
  hz3 が同等の “coverage（madvise対象面積）” を作れるかが本質。

S57-B2（segment走査）では coverage 不足で NO-GO（台帳: `hakozuna/hz3/docs/PHASE_HZ3_S57_B2_PARTIAL_SUBRELEASE_POST_S58_WORK_ORDER.md`）。
次は S60（PurgeRangeQueueBox）で “返却した範囲をそのまま purge 対象にする” 方向へ進む。

---

## 4) Next（S60 → S61）

S60（PurgeRangeQueueBox）:
- `madvise` 発火の配線自体は成立したが、S59-2 の idle RSS 目的では **NO-GO**（対象面積が小さく、idle で RSS が動かない）。
- 台帳: `hakozuna/hz3/docs/PHASE_HZ3_S60_PURGE_RANGE_QUEUE_BOX_WORK_ORDER.md`

次:
- idle/exit の境界で “強い purge” を撃つ（S61）。
- 指示書: `hakozuna/hz3/docs/PHASE_HZ3_S61_DTOR_HARD_PURGE_BOX_WORK_ORDER.md`
