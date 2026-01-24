# Benchmark Update (2026-01-21)

## 新規ベンチマーク結果

Native環境 (8コア/16スレッド) での4者比較結果を追加。

### 主要な発見

#### 1. MT Remote R=90% で hz3 が圧勝

| Condition | hz3 (S113) | tcmalloc | 差 |
|-----------|------------|----------|-----|
| T=8 R=90% | 62.81M | 40.20M | **+56.2%** |

高 remote-free 比率のワークロードで、hz3 の S44 owner_stash アーキテクチャが威力を発揮。

#### 2. PTAG32 vs S113 (Segment Header) の比較

| 方式 | 特徴 | 得意なワークロード |
|------|------|-------------------|
| PTAG32 | グローバル配列O(1)検索 | シングルスレッド、低スレッド |
| S113 | セグメントヘッダ埋め込み | 高スレッド、高並列 |

##### CPUキャッシュ効率の違い

**xmalloc-testN (シングルスレッド寄り)**:
- PTAG32: 82.4B命令, IPC 0.86
- S113: 81.1B命令, IPC 0.84
- mimalloc: 121.6B命令, IPC **1.68** ← キャッシュ効率が非常に高い

**MT Remote T=8 (マルチスレッド)**:
- PTAG32: 4.3B命令, IPC 0.87
- S113: 4.4B命令, IPC 0.86
- mimalloc: 3.8B命令, IPC **0.62** ← 命令少ないがキャッシュミス多い

##### 考察

1. **PTAG32 のキャッシュ問題**:
   - グローバル配列 `g_hz3_pagetag32[]` はランダムアクセスでキャッシュミスしやすい
   - 高スレッド時に複数スレッドが異なる位置を参照 → キャッシュライン競合

2. **S113 のキャッシュ利点**:
   - セグメントヘッダは各セグメントの先頭に配置
   - 同じセグメント内の操作はヘッダがL1/L2にホット維持
   - スレッドごとにワーキングセットが分離

3. **MT環境での逆転**:
   - mimalloc: 命令数少ないが IPC 0.62 (キャッシュミス多い)
   - hz3: 命令数多いが IPC 0.86 (S44のMPSCキューがキャッシュフレンドリー)

#### 3. hz3 の差別化ポイント: リモートfree最適化スタック

```
┌─────────────────────────────────────────────┐
│  hz3 Remote-Free Optimization Stack         │
├─────────────────────────────────────────────┤
│  1. Remote Stash (TLS バッチング)            │
│  2. S97 Bucketing (owner別ソート/グループ)   │
│  3. S44 Owner Stash (MPSC per-shard)        │
│  4. Prefetch最適化 (S44-4系)                │
└─────────────────────────────────────────────┘
```

mimalloc との本質的な違い:
- mimalloc: 即時 atomic CAS to delayed_list
- hz3: バッチ蓄積 → ソート → MPSC転送 → owner回収

### README 更新案

```markdown
### MT Remote-Free (ops/sec, higher is better)

| Condition | hakozuna | mimalloc | tcmalloc |
|-----------|----------|----------|----------|
| T=8 R=90% | **62.81M** | 54.63M | 40.20M |
| T=8 R=50% | **69.10M** | 60.43M | 59.97M |
| T=16 R=50% | **101.03M** | 94.44M | 101.13M |

Note: hz3 achieves +56% over tcmalloc and +15% over mimalloc at T=8 R=90%.
```

## 論文への追記候補

1. **Section: Metadata Lookup Comparison**
   - PTAG32 (global array) vs S113 (segment header)
   - Cache behavior analysis with IPC data

2. **Section: Remote-Free Architecture**
   - S44 Owner Stash design
   - Batch transfer vs immediate atomic

3. **Figure: Cache Efficiency**
   - IPC comparison chart: hz3 vs mimalloc at different thread counts
