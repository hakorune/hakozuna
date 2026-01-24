# S47-3: Pinned Avoidance Box (Segment Quarantine Extension)

## 目的
remote-heavy (T=32, R=50) での `alloc failed` を 0 固定にする。
S47 (segment quarantine) と S47-2 (ArenaGate) を維持したまま、pinned segment
(例: free_pages が 511 で張り付く) を避ける。

## 方式 (event-only)
- ArenaGate: alloc_full 時に leader だけが compaction を実行。
- Quarantine: draining_seg からの新規割当を停止。
- Avoid list: pinned / TTL 超過の seg を一時的にスキップ。
- Weighted scoring: free_pages を重視して空けやすい seg を優先。

### Weighted scoring
```
score = sampled_pages + (free_pages * HZ3_S47_FREEPAGES_WEIGHT)
```

### Avoid list (per-shard, leader only)
- avoid から TTL 経過で復帰。
- draining_seg が TTL 超過なら avoid に追加して再選定。

## 変更点
- `hakozuna/hz3/include/hz3_config.h`
  - `HZ3_S47_AVOID_ENABLE=1`
  - `HZ3_S47_AVOID_SLOTS=4`
  - `HZ3_S47_AVOID_TTL=8`
  - `HZ3_S47_FREEPAGES_WEIGHT=2`
- `hakozuna/hz3/src/hz3_segment_quarantine.c`
  - avoid list 管理 (add/remove/expire)
  - scan_and_select: TTL expire + avoid skip + weighted scoring
  - drain: 解放成功時に avoid から削除

## 結果 (RUNS=10)
- alloc_failed: 0 (10/10)
- 成功率: 100%
- **throughput が大きく低下** (2.3M - 21M ops/s, median ~14M)
  - 要再確認: ベンチ条件 / compaction 発火頻度 / headroom 設定

## 判定
- 安定性: GO (alloc failed 0)
- 性能: **要調査** (回帰が大きい可能性あり)

## 次の確認ポイント
1. compaction が常時発火していないか (headroom が高すぎる可能性)
2. pressure handler が過剰に drain していないか
3. R=50 のベンチ条件が S47-2 以前と同一か

