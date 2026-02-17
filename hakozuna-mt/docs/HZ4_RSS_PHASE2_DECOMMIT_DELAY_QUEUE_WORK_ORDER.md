# HZ4 RSS Phase 2: DecommitQueueBox (Delay + Budget)

目的:
- Phase 1F の「decommit が効くが、local-only でスラッシングして激遅」を解消する。
- RSS を下げつつ、ops/s の回復（mimalloc の purge delay 相当）を狙う。

非目的:
- hot path に常時コストを足すこと（free/malloc の通常分岐は増やさない）。
- scan を常時回すこと（queue のみを対象にする）。

---

## Box Theory
- 新しい箱: **DecommitQueueBox**
  - 「空になった候補ページ」を deadline 付きでキューに積む
  - deadline 到来したページだけを、budget 付きで decommit する
- 変換点は 1 箇所:
  - CollectBox（refill 境界）で **期限到来分だけ** purge→decommit を実行
- 切り戻し:
  - フラグで OFF（Phase 1F の即時 decommit 経路は封印）

---

## 0) 現状（Phase 1F）の問題

- `used_count==0` の瞬間に purge→decommit を実行すると、
  local-only (R=0) で `decommit≈commit` を繰り返し、syscall thrash で激遅になる。

必要なのは:
- 「空になった直後」は返さない
- 「しばらく空のまま」なら返す

---

## 1) 新フラグ案（opt-in）

- `HZ4_DECOMMIT_DELAY_QUEUE=0/1`（default 0）
- `HZ4_DECOMMIT_DELAY_TICKS`（default 50 など）
- `HZ4_DECOMMIT_BUDGET_PER_COLLECT`（default 1〜4）

※ `ticks` は wallclock ではなく、「CollectBox の呼び出し回数」や「epoch」などの安定した内部カウンタで良い。

---

## 2) メタデータ（PageMetaBox の拡張）

必要最小:
- `empty_deadline_tick`（例: `uint32_t`）
- `queued_decommit`（0/1、二重enqueue防止）

原則:
- owner thread のみ更新（atomic でなくて良い）
- 非ゼロ = pending（deadline を持つ）

---

## 3) DecommitQueueBox の形

### 3.1 Queue

- owner thread ごとに単純な SPSC/ローカル queue（MPSC 不要）
- node は page_meta の `qnext` を再利用してもよいが、pageq と混線しないこと
  - 推奨: decommit 用に `dqnext` を新設（PageMetaBox）

### 3.2 Enqueue

トリガ:
- `used_count` が 0 になったとき（owner free 境界）

動作:
- `empty_deadline_tick = now + HZ4_DECOMMIT_DELAY_TICKS`
- `queued_decommit=1` で queue に入れる（既に入っていたら何もしない）

※ この時点では purge/decommit をしない（遅延）。

---

## 4) Decommit 実行（CollectBox 境界）

CollectBox の最後（または最初）に:
- `now_tick++`（TLS/owner のカウンタ）
- queue から最大 `HZ4_DECOMMIT_BUDGET_PER_COLLECT` 個だけ処理
  - `deadline <= now_tick` のページだけ
  - still empty 判定:
    - `used_count==0`
    - remote が空（必要なら）
    - その page の tcache 参照が purge 済み（Phase 1F の purge をここで実行）
  - OK なら decommit
  - NG（途中で使われた等）なら `queued_decommit=0` で捨てる

---

## 5) SSOT 検証

確認すべきこと:
- R=0 の `page_decommit` が **大幅に減る**（スラッシングが止まる）
- R=0 の ops/s が baseline に近づく
- R=50 の RSS 低下は維持できる

ログは `HZ4_OS_STATS` のみ（debug prints は既定OFF）。

