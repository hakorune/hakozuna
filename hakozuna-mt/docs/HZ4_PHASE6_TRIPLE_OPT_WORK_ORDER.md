# HZ4 Phase 6: Triple Optimization Work Order

目的:
- hz4 の **T=16 R=90** のボトルネックを 3 点同時に潰す  
  1) TLS 固定費削減  
  2) memset 低減  
  3) remote_flush 最適化

前提:
- SegQ O(1) push_list 実装済み
- Phase6 perf 分析で hot が特定済み  
  `__tls_get_addr` / `memset` / `hz4_remote_flush`

---

## 0) ルール（Box Theory）

- 変更は **箱単位**（TCache / OS / Collect / RemoteFlush）
- **境界は 1 箇所**に固定
- A/B は compile-time で切替可能にする（`-D` or `#ifdef`)
- Fail‑Fast を維持

---

## 1) TLS 固定費削減 (TCache Box)

### 狙い
`__tls_get_addr` の割合を下げる（~6%）

### 実装候補
- `-ftls-model=initial-exec` を **hz4 だけ**に適用
- TLS 参照をまとめる（`hz4_tls_get()` を hot path で 1 回だけ呼ぶ）

### A/B
```
HZ4_TLS_INITIAL_EXEC=1 (default ON, set 0 to disable)
```

### 検証
- perf report で `__tls_get_addr` の self% が減るか
- ST (T=1) と MT (T=16 R=90) の ops/s を比較

---

## 2) memset 低減 (OS / MidBox)

### 狙い
`memset` 4–5% を削減

### 実装候補
- page/segment の **全体 memset を削減**し、必要なフィールドだけ初期化
- MidBox の page carve で **必要最小だけ zero**  
  (free list を作るときに必要なヘッダのみ)

### A/B
```
HZ4_SEG_ZERO_FULL=0 (new)
```

### 検証
- perf report で `__memset` の self% が減るか
- smoke test (`hz4_route_edges.c`) が通るか

---

## 3) remote_flush 最適化 (RemoteFlush Box)

### 狙い
`hz4_remote_flush` 3% 台を削減

### 実装候補
- flush の条件を厳しくする（閾値増加）
- バッチサイズ拡大 or 連結処理の最適化
- collect 側で carry を優先し、flush 回数を減らす

### A/B
```
HZ4_REMOTE_FLUSH_BATCH=... (new)
HZ4_REMOTE_FLUSH_THRESHOLD=... (new)
```

---

## 4) ベンチ (SSOT)

```
ITERS=2000000 WS=400 MIN=16 MAX=2048 R=90 RING=65536

LD_PRELOAD=./hakozuna/hz4/libhakozuna_hz4.so \
  ./hakozuna/out/bench_random_mixed_mt_remote_malloc 16 $ITERS $WS $MIN $MAX $R $RING

LD_PRELOAD=./libhakozuna_hz3_scale.so \
  ./hakozuna/out/bench_random_mixed_mt_remote_malloc 16 $ITERS $WS $MIN $MAX $R $RING
```

---

## 5) 成果物 (SSOT)

```
T=16 R=90
hz4: <ops/s>
hz3: <ops/s>
delta: <hz4/hz3 - 1>
perf: <top3>
notes: <採用/NO-GO>
```

---

## 6) 注意

- 3項目は **独立 A/B** を守る（同時に混ぜない）
- GO/NO‑GO を明確に 기록
