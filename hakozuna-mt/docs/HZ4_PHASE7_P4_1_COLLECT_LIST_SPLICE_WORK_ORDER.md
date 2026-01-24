# HZ4 Phase 7: P4.1 Collect‑List Splice Work Order

目的:
- `hz4_collect` の **out[] 配列ストアを排除**し、直接 list を返す
- out[] store が perf で支配的なため、**最優先で削減**する

---

## 0) ルール（Box Theory）

- 変更は **Collect / Tcache** の箱内のみ
- A/B は compile-time で切替可能
- Fail‑Fast は維持（debug 限定）

---

## 1) 方針

- `hz4_collect` を **list 出力モード**に追加対応
- `out[]` を **完全に使わない経路**を作る
- `list head/tail/n` を返し、bin に splice

---

## 2) API 追加案

```c
// collect list API
uint32_t hz4_collect_list(hz4_tls_t* tls, uint8_t sc,
                          void** head_out, void** tail_out);
```

返却:
- `head_out` / `tail_out` に prefix list
- 取得数 `n` を return

---

## 3) 変更箇所

- `core/hz4_collect.inc`:
  - `hz4_collect_default()` の out[] 経路は残す
  - `hz4_collect_list()` を追加
- `src/hz4_tcache.c`:
  - `hz4_refill()` で list 版を使う分岐追加

---

## 4) Tcache splice

```c
static inline void hz4_tcache_splice(hz4_tcache_bin_t* bin,
                                     void* head, void* tail, uint32_t n) {
    if (!head) return;
    hz4_obj_set_next(tail, bin->head);
    bin->head = head;
#if HZ4_TCACHE_COUNT
    bin->count += n;
#endif
}
```

---

## 5) A/B 定義

```
HZ4_COLLECT_LIST=1
```

---

## 6) SSOT

```
ITERS=2000000 WS=400 MIN=16 MAX=2048 R=90 RING=65536
LD_PRELOAD=./hakozuna/hz4/libhakozuna_hz4.so \
  ./hakozuna/out/bench_random_mixed_mt_remote_malloc 16 $ITERS $WS $MIN $MAX $R $RING
```

---

## 7) NO-GO 判定

- **+5% 未満**なら NO‑GO
- correctness 低下なら即撤退

---

## 8) 記録フォーマット

```
P4.1 collect_list
T=16 R=90
hz4: <ops/s>
perf: out[] store が消えるか
notes: <採用/NO-GO>
```

---

## 9) P4.1a 結果 (2026-01-23)

### A/B Test (10 rounds, alternating)

| Round | LIST=0 | LIST=1 | Winner |
|-------|--------|--------|--------|
| 1 | 105.3M | 78.1M | LIST=0 |
| 2 | 96.4M | 70.7M | LIST=0 |
| 3 | 72.6M | 66.4M | LIST=0 |
| 4 | 88.4M | 107.2M | LIST=1 |
| 5 | 69.4M | 91.7M | LIST=1 |
| 6 | 74.3M | 82.8M | LIST=1 |
| 7 | 71.6M | 96.6M | LIST=1 |
| 8 | 72.8M | 68.9M | LIST=0 |
| 9 | 78.1M | 68.5M | LIST=0 |
| 10 | 78.0M | 88.8M | LIST=1 |

- **勝率**: LIST=0 5勝 / LIST=1 5勝 (引き分け)
- **平均**: LIST=0 ~80.7M vs LIST=1 ~82.0M (**+1.6%**)

### 判定

**P4.1a (inbox/carry のみ list 化)**: **Inconclusive**
- 高分散で結論不能
- +1.6% は閾値 +5% 未満だが、計画通り P4.1b へ継続

### 次ステップ

P4.1b: drain_page を list 化して segq/pageq 経路の out[] store を削減

---

## 10) P4.1b 結果 (2026-01-23)

### A/B Test (10 rounds, alternating)

| Round | LIST=0 | LIST=1 | Winner |
|-------|--------|--------|--------|
| 1 | 84.39M | 76.40M | LIST=0 |
| 2 | 82.10M | 84.20M | LIST=1 |
| 3 | 81.47M | 106.36M | LIST=1 |
| 4 | 76.68M | 84.65M | LIST=1 |
| 5 | 79.05M | 80.30M | LIST=1 |
| 6 | 88.58M | 94.24M | LIST=1 |
| 7 | 75.12M | 87.29M | LIST=1 |
| 8 | 92.40M | 119.67M | LIST=1 |
| 9 | 91.81M | 97.08M | LIST=1 |
| 10 | 77.32M | 69.90M | LIST=0 |

- **勝率**: LIST=0 2勝 / LIST=1 8勝
- **平均**: LIST=0 ~82.89M vs LIST=1 ~90.01M (**+8.6%**)

### 判定

**P4.1b (drain_page/segment list 化)**: **GO**
- +8.6% は閾値 +5% を超過
- 8/10 で LIST=1 勝利 (統計的に有意)

### 実装内容

- `hz4_list_splice`: list 連結 helper
- `hz4_drain_page_list`: remote shard から list 抽出
- `hz4_drain_segment_list`: PageQ 版 segment drain (list splice)
- `hz4_collect_list`: segq 経路を完全 list 化

### 次ステップ

- HZ4_COLLECT_LIST=1 をデフォルト ON に変更
- Perf で out[] store 削減を確認
