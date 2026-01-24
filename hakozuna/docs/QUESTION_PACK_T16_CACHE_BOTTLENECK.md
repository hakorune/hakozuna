# 質問パック: T=16 L1キャッシュボトルネック分析

## 背景

hz3は高remote-free比率のMTワークロードで優れた性能を示すメモリアロケータです。
しかしT=16以上でmimallocに対して性能が低下する問題が発見されました。

## ベンチマーク結果

### MT Remote R=90% (M ops/s)

| Condition | hz3 (S113) | mimalloc | tcmalloc | hz3 vs mi |
|-----------|------------|----------|----------|-----------|
| T=8 R=90% | **62.81** | 54.63 | 40.20 | **+15.0%** |
| T=16 R=90% | 69.71 | **86.89** | 71.33 | **-19.8%** |

T=8では勝つが、T=16では負ける。

### perf結果 (T=8 → T=16)

| 指標 | S113 T=8 | S113 T=16 | 変化 |
|------|----------|-----------|------|
| 命令数 | 6.44G | 12.11G | 1.88倍 |
| L1 miss率 | 4.80% | 5.77% | +20%悪化 |
| IPC | 0.88 | 0.86 | -2.3% |

| 指標 | mimalloc T=8 | mimalloc T=16 | 変化 |
|------|--------------|---------------|------|
| 命令数 | 4.88G | 9.94G | 2.04倍 |
| L1 miss率 | - | - | 安定 |
| IPC | - | - | 安定 |

## 現在のアーキテクチャ

### S44 Owner Stash (MPSC per-shard queue)

```c
typedef struct {
    _Atomic(void*) head;      // MPSC linked list head
    _Atomic(uint32_t) count;  // approximate count
} Hz3OwnerStashBin;

// グローバル配列: [NUM_SHARDS][NUM_SC]
// NUM_SHARDS = 32 (scale lane)
// NUM_SC ≈ 30 (size classes)
static Hz3OwnerStashBin g_hz3_owner_stash[HZ3_NUM_SHARDS][HZ3_SMALL_NUM_SC];
```

### Remote Free フロー

1. Thread A が ptr を free (owner = Shard X)
2. PTAG32 で owner 判定
3. Remote Stash (TLS) にバッチング
4. drain 時に S97 bucketing (owner別ソート)
5. Owner Stash[X] へ CAS push
6. Shard X の refill 時に atomic_exchange で一括 pop

### データ構造のサイズ

- Hz3OwnerStashBin: 16 bytes (8 + 4 + padding)
- g_hz3_owner_stash: 32 * 30 * 16 = 15,360 bytes
- 各 shard の bins は連続配置

## 仮説

### 仮説1: False Sharing

```
Cache Line (64 bytes):
┌────────────────────────────────────────────────────────────────┐
│ Bin[0].head │ Bin[0].count │ Bin[1].head │ Bin[1].count │ ... │
└────────────────────────────────────────────────────────────────┘
     ↑              ↑              ↑              ↑
  Thread A       Thread B       Thread C       Thread D
  (shard 0)      (shard 1)      (shard 2)      (shard 3)
```

異なる shard の bins が同一キャッシュラインに載り、T=16 で bouncing が発生？

### 仮説2: Owner Stash のホットスポット

T=16 では 16 スレッドが 32 shard に分散するが、特定 shard に集中すると競合が増える？

### 仮説3: Atomic 操作の増加

T=16 では CAS/exchange の回数が増え、メモリバリアのコストが顕在化？

## 質問

1. **False Sharing 対策**
   - Hz3OwnerStashBin を 64 bytes にパディングすべき？
   - shard ごとにキャッシュライン境界に配置すべき？
   - `__attribute__((aligned(64)))` の効果は？

2. **データ構造の再設計**
   - shard 数を T=16 以上に最適化すべき？ (現在 32)
   - per-thread owner stash (TLS) に変更すべき？
   - 階層化すべき？ (thread-local → shard-local → global)

3. **Remote Free 戦略**
   - T=16 以上で別の drain 戦略が有効？
   - バッチサイズを動的に調整すべき？
   - central bin への直接 push の閾値を変えるべき？

4. **mimalloc との設計差**
   - mimalloc の delayed_free が T=16 で安定する理由は？
   - hz3 が採用すべき mimalloc の設計要素は？

## 添付ファイル

`hz3_t16_cache_bottleneck.zip` に以下を含む:
- hz3_owner_stash.h / .c - Owner Stash 実装
- hz3_owner_stash_pop.inc / push.inc - pop/push 詳細
- hz3_tcache.h / .c - Thread Cache
- hz3_config_scale.h - scale lane 設定
- hz3_types.h - 型定義
- hz3_hot.c - ホットパス
- hz3_small_v2_refill.inc - refill 処理
- hz3_small_v2_push_remote.inc - remote push

## 期待する回答

1. L1キャッシュボトルネックの根本原因の特定
2. False sharing 対策の具体的なコード変更案
3. T=16+ で有効な設計変更の提案
4. A/Bテストで検証すべき項目のリスト
