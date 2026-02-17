# mt_remote main レーン詳細調査 (2026-02-13)

## 測定条件

- ベンチマーク: `bench_random_mixed_mt_remote_malloc`
- 引数: `threads=16 iters=2000000 ws=400 min=16 max=32768 remote_pct=90 ring_slots=65536`
- CPU pinning: `taskset -c 0-15`

## 1. 性能比較 (RUNS=10)

### hz4
| Run | ops/s |
|-----|-------|
| 1 | 43,516,383 |
| 2 | 51,948,891 |
| 3 | 52,031,672 |
| 4 | 53,059,600 |
| 5 | 51,641,865 |
| 6 | 52,679,150 |
| 7 | 54,379,582 |
| 8 | 49,358,038 |
| 9 | 52,160,628 |
| 10 | 51,932,252 |

**中央値: 約 52.0M ops/s**

### tcmalloc
| Run | ops/s |
|-----|-------|
| 1 | 51,845,648 |
| 2 | 54,542,217 |
| 3 | 56,748,578 |
| 4 | 54,492,330 |
| 5 | 57,495,056 |
| 6 | 54,020,456 |
| 7 | 53,607,596 |
| 8 | 54,832,822 |
| 9 | 47,223,644 |
| 10 | 41,540,990 |

**中央値: 約 54.2M ops/s**

### 比較結果
- **hz4**: 52.0M ops/s
- **tcmalloc**: 54.2M ops/s
- **差**: tcmallocが約 +4% 速い（誤差範囲内）

**結論**: mt_remoteレーンではhz4とtcmallocが**ほぼ同等**の性能

## 2. 詳細カウンター分析

### Midボックス統計 (HZ4_MID_STATS_B1)

```
malloc_calls=3,753,634
malloc_alloc_cache_hit=1,139,534 (30.3%)  ← 低い！
malloc_lock_path=2,614,100 (69.7%)         ← 70%がロックパス！
malloc_bin_hit=2,607,940
malloc_bin_miss=6,160
malloc_page_create=6,160

free_calls=3,753,633
free_batch_enq=3,753,633 (100%バッチ化)
free_batch_flush=1,509,936
```

**重要発見**:
- **70%のmallocがロックパス**を通過
- これは通常のmalloc_args（44%）よりも**高い**
- リモートフリーが多い環境ではMidボックスの競合が激しい

### OS/リモート層統計 (HZ4_OS_STATS)

```
リモート操作:
  remote_flush_calls=2,316       ← リモートフラッシュ呼び出し
  inbox_push_list=156,522        ← インボックスリストプッシュ
  rlen_ge_th=2,316               ← 閾値以上のリスト

インボックス:
  inbox_consume=4,686            ← インボックス消費
  inbox_stash_max=27             ← スタッシュ最大
  carry_miss=2,052               ← キャリーミス

ページ操作:
  seg_acq=556                    ← セグメント獲得
  large_acq=9,437,184            ← large獲得バイト
  large_rel=3,538,944            ← large解放バイト
```

### ロックシャード統計 (HZ4_OS_STATS_B9)

```
ls_acq_self=0
ls_acq_steal=0
ls_acq_miss=0
ls_rel_self=0
ls_rel_steal=0
ls_rel_miss=0
p2_acq_self=0
...
```

**重要**: B9カウンターがすべて0！
- mt_remoteではロックシャード統計が機能していないか、別のロックパスを通っている
- Midボックスの`hz4_mid_sc_lock`は別の統計を使っている可能性

## 3. ベンチマーク統計

```
[EFFECTIVE_REMOTE] target=90.0% actual=90.0% fallback_rate=0.00%
[STATS] allocated=16,001,603
        local_freed=1,597,913 (10%)
        remote_sent=14,400,484 (90%)     ← 目標通り90%リモート
        remote_received=14,400,484
        ring_full_fallback=0             ← リングフルなし
        overflow_sent=0
        overflow_received=0
```

**リモートフリー性能**:
- 目標90%のリモートフリーを達成
- リングバッファ溢れ（fallback）は発生していない
- overflowも発生していない

## 4. ボトルネック分析

### 主要ボトルネック

1. **Midボックスのロック競合 (69.7%)**
   - `malloc_lock_path=2,614,100`
   - スレッド間でMidページを競合
   - 改善案: Midアクティブランの再設計（スレッドローカルフリー対応）

2. **キャッシュヒット率低い (30.3%)**
   - `malloc_alloc_cache_hit=1,139,534`
   - 改善案: TCacheサイズ調整、またはスレッドローカルキャッシュ増強

3. **インボックス処理**
   - `inbox_consume=4,686`
   - `inbox_stash_max=27`
   - リモートフリーの消費がボトルネックの可能性

### 改善優先度

| 優先度 | 項目 | 期待効果 |
|--------|------|----------|
| 1 | Midロック競合軽減 | 大（70%→40%目標） |
| 2 | TCacheヒット率向上 | 中（30%→50%目標） |
| 3 | インボックス消費最適化 | 小〜中 |

## 5. 比較まとめ

| レーン | hz4 | tcmalloc | 比率 | 状態 |
|--------|-----|----------|------|------|
| mt_remote main | 52.0M | 54.2M | 96% | **ほぼ同等** |
| malloc_args main | 38.2M | 98.0M | 39% | 大幅遅延 |

**mt_remoteではhz4が強い**設計が機能しているが、まだtcmallocを追い越していない。

## 6. 次のアクション

1. **Midロック競合の詳細調査**
   - `hz4_mid_sc_lock`の実装を確認
   - スピンロック vs ミューテックスの評価
   - ロックシャード数の増加検討（16→32/64）

2. **アクティブラン再設計（安全版）**
   - スレッドローカルフリーリストの実装
   - 所有者チェック付きアクティブラン

3. **TCacheチューニング**
   - サイズクラス別の深さ調整
   - リモートフリー時のバッチサイズ最適化
