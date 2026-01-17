# S121 Page-Local Remote 修正計画

## Status: S121-C GO / S121-D NO-GO（完了）

## 背景

S121-B (empty→non-empty enqueue) を実装したが、まだ 21% 遅い。

### 計測結果

| Config | free/sec | vs S121=0 |
|--------|----------|-----------|
| mimalloc | 113M | +38% |
| S121=0 baseline | 82M | baseline |
| S121-B | 64.5M | **-21%** |
| **S121-C (drain-all)** | **83.5M** | **+1.8% (GO)** |
| S121-D (page packet) | 15M | **-82% (NO-GO)** |

### Perf 分析

```
S121-B push_one 内訳:
16.23%  lock cmpxchg  (page->remote_head CAS)
 4.45%  xchg          (remote_state)
 8.00%  lock cmpxchg  (pageq push CAS)
12.62%  jmp (retry)   ← pageq への高競合!
```

### 根本問題

1. **pageq が中央ボトルネック化**: (owner, sc) 単位 = 元の owner stash と同じ粒度
2. **pop が CAS で 1page ずつ**: producer と殴り合って retry 増加
3. **objs/page = 1.2**: queue 操作の償却が効かない
4. **empty→non-empty が 84%**: ほぼ毎 push で pageq 操作

---

## ChatGPT Pro 提案（優先順）

### 案1: pageq を drain-all に変更 (最小改修)

**狙い**: pop 側の CAS retry (12.62%) を消す

**現状**:
```c
// pop: CAS で 1page ずつ
page = hz3_pageq_pop(owner, sc);  // CAS per page
```

**変更**:
```c
// pop: atomic_exchange で丸ごと奪取
page_list = hz3_pageq_drain_all(owner, sc);  // 1回の exchange
// ローカルで走査して処理
```

**期待効果**:
- pop 側の retry が消える
- producer と consumer の競合が減少

---

### 案2: Page通知の packet 化

**狙い**: pageq への push 頻度を 1/K に削減

**現状**:
```c
// push: empty→non-empty で即 pageq push
if (old_head == NULL) {
    pageq_push(page);  // 84% の push で発生
}
```

**変更**:
```c
// push: TLS packet に溜めてから batch push
if (old_head == NULL) {
    tls_packet_add(owner, sc, page);
    if (packet_full()) {
        pageq_push_packet(packet);  // K pages まとめて
    }
}
```

**期待効果**:
- pageq push 頻度が 1/K に
- 1 dequeue で K pages 分の仕事

---

### 案3: mimalloc 型 cadence collect

**狙い**: pageq (通知) 自体を消す

**現状**:
- push: page に積む + pageq で通知
- pop: pageq から page を取得

**変更**:
- push: page に積むだけ (通知なし)
- pop: owner が自分の pages を定期スキャン

**期待効果**:
- 通知競合が完全に消える

**リスク**:
- 設計が大きく変わる
- owner の page 管理が必要

---

## 実装順序

### Phase 1: 案1 (drain-all pageq)

1. `hz3_pageq_drain_all()` を追加
2. `hz3_owner_stash_pop_batch()` を drain-all 方式に変更
3. requeue を push_list でまとめて実行
4. 計測・比較

### Phase 2: 案2 (packet 化) - 案1 の効果次第

1. TLS page packet 構造を追加
2. push 側を packet 経由に変更
3. pop 側を packet drain に対応
4. 計測・比較

### Phase 3: 案3 (cadence collect) - 案2 でもダメなら

---

## 実装結果

### S121-C: drain-all pageq (2025-01-14) - **GO**

**実装内容**:
1. `hz3_pageq_drain_all()` を追加（atomic_exchange で丸ごと取得）
2. `hz3_owner_stash_pop_batch()` を drain-all 方式に変更
3. 余剰ページは push_list でまとめて requeue

**計測結果**:
```
S121=1 (drain-all): 83.5M free/sec
S121=0 baseline:    82.0M free/sec
差分:               +1.8%
```

**結論**:
- pop 側の CAS retry が消えて baseline 復帰
- **GO** - S121 のデフォルト pop 実装として採用

---

### S121-D: Page Packet Notification (2025-01-14) - **NO-GO**

**狙い**: pageq への push 頻度を 1/K に削減

**実装内容**:
1. TLS に `Hz3PagePacketSlot[N_SLOTS]` を追加
2. empty→non-empty 時に packet に溜めて K pages で flush
3. thread 終了時に残り packet を flush

**設定**:
```c
HZ3_S121_D_PACKET_K = 4   // 4 pages per batch
HZ3_S121_D_N_SLOTS = 8    // 8 slots per thread
```

**計測結果**:
```
S121-C baseline:    83.5M free/sec
S121-D (K=4, N=8):  15.0M free/sec  (-82%)
S121-D (K=4, N=1):  15.0M free/sec  (-82%)
```

**Stats 出力**:
```
[HZ3_S121D_STATS] packet_flushes=6,638,839 pages_flushed=26,555,356
[HZ3_S121D_STATS] slot_hit=19,916,517 (75.0%)  slot_evict=0 (0.0%)
```

**敗因分析**:
- batching 自体は機能（75% slot 再利用、~4 pages/flush）
- しかし packet 管理のオーバーヘッド（linked list 操作 per page）が
  pageq CAS 削減効果を大幅に上回った
- slot 検索（N=8→N=1）は影響なし → pure overhead が問題

**結論**:
- **NO-GO** - packet 方式は採用せず
- S121-C (drain-all) で baseline 復帰は達成済み
- さらなる改善が必要なら案3 (cadence collect) を検討

---

## 不変条件チェックリスト

### remote_state の意味
- 0: idle (pageq に入っていない)
- 1: queued (pageq に入っている)

### 正しさの条件
1. `remote_state == 1` なら、その page は必ず pageq に存在するか処理中
2. `remote_head != NULL && remote_state == 0` は許容 (次の push で enqueue)
3. pop 後の二度見: `remote_head` を再確認して lost wakeup 防止

### drain-all での注意点
- 複数 page を取得するので、各 page の処理が独立である必要
- requeue は処理後にまとめて (push_list)
- remote_state の二度見ロジックは各 page で個別に適用

---

## 参考

- mimalloc paper: https://www.microsoft.com/en-us/research/wp-content/uploads/2019/06/mimalloc-tr-v1.pdf
- tcmalloc design: https://google.github.io/tcmalloc/design.html
- Vyukov queue: https://int08h.com/post/ode-to-a-vyukov-queue/
