# HZ4 Phase 4: Event-Driven Collect (Pending Seg Queue 強化)

目的:
- hz4 の **collect 側の重さ**を下げる
- scan を最小化し、**pending seg queue を唯一の入口**にする
- Box Theory の境界を **remote_flush / collect の2箇所**に固定

前提:
- Phase 0/1/2 は完了済み（core box + bench + page0 予約）
- 現状課題: collect の pop/drain が重い（pop支配）
- 目標: **event-driven**（notify → queue → drain）で scan を抑制

---

## Box 構成（SSOT）

FREE fast
  └─ TLS rbuf → RemoteFlushBox → page.remote_head + pending seg notify

MALLOC slow
  └─ CollectBox → pending seg queue からのみ drain

境界:
- `hz4_remote_flush()`（notify と publish はここに集約）
- `hz4_collect()`（drain と requeue はここに集約）

---

## 1) Pending Seg Queue を SSOT に固定

### 1.1 状態機械（必須）

```
IDLE(0) ── notify ──> QUEUED(1) ── pop ──> PROC(2)
  ^                        |                     |
  └────── no pending ──────┘        pending残 ────┘ (requeue)
```

- **notify**: pending bit を立てた後に `qstate` を 0→1 に CAS し、成功時のみ enqueue
- **pop**: `pop_all()` で list 取得後、各 seg を PROC にして drain
- **finish**: pending が残っていれば再 enqueue（取りこぼし防止）

### 1.2 ルール（Box Theory）
- **pending bits の更新は flush 側だけ**（境界1箇所）
- **qstate の更新は segq 側だけ**（境界1箇所）
- hot path で scan しない

---

## 2) CollectBox のスリム化（scan を抑制）

### 2.1 Collect の基本形

```
hz4_collect():
  qlist = segq_pop_all(owner)
  while (qlist && obj_budget && seg_budget):
     drain_segment(seg)
     segq_finish(seg)  // pending 残りなら requeue
  if got == 0:
     scan_fallback (最小限)
```

### 2.2 scan_fallback の条件
- **queue empty かつ got==0 の時のみ**
- Phase 4 では **scan は最小実装 or disable** を推奨

---

## 3) drain_segment の軽量化

### 3.1 pending_bits の扱い
- `atomic_exchange(word, 0)` を基本（bitごとの CAS は使わない）
- word=0 の bit0 は常に無視（page0 予約）

### 3.2 budget 設計
```
obj_budget:  既定 64 (A/B)
seg_budget:  既定 4  (A/B)
```
- budget を超えたら即 return（hot path を止めない）

---

## 4) 追加すべき統計（必要最小限）

TLS stats (cold path):
- `segs_popped`
- `segs_drained`
- `segs_requeued`
- `objs_drained`
- `pages_drained`
- `scan_fallback`

目標指標:
- **requeue rate < 20%**
- **scan_fallback ≈ 0**

---

## 5) 実装順序（小さく積む）

1. **segq notify を固める**
   - pending bit set → qstate CAS → enqueue
2. **collect の scan fallback 条件を固定**
3. **drain_segment の budget を hard limit**
4. **stats を確認（bench で requeue rate を記録）**

---

## 8) 追記: carry / pageq の評価結果（2026-01-22）

- **carry 化は GO**（高 remote% で +16〜20%）
- **remote_mask は NO-GO**（HZ4_REMOTE_SHARDS=4 では効果なし）
- **pageq（seg 内 page queue）を導入**して bitmap scan を回避（本フェーズ）
- **pageq は GO**（害なし + 微増、segq 操作大幅削減）

理由:
- remainder pushback の再走査が pop 支配だったため carry が効いた
- pageq は「pending のある page だけ」列挙するため scan を減らす

---

## 6) ベンチ（Phase 4 SSOT）

```
make -C hakozuna/hz4 test
make -C hakozuna/hz4 bench
```

観測:
- `segs_requeued / segs_drained` が下がるか
- `scan_fallback` が 0 付近か

---

## 7) A/B 指標（必須）

| 指標 | 期待 |
|------|------|
| collect throughput | +10% 以上 |
| segs_requeued rate | 低下 |
| scan_fallback | ほぼ 0 |

---

## 8) 注意（Box Theory）

- **Hot path を触らない**
- 境界は2箇所固定（flush/collect）
- 変更は `HZ4_*` フラグで戻せる形にする
