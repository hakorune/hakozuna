# hz3 OwnerSharedStashBox 設計相談パック（ChatGPT Pro 用）

目的:
- S43（TLS stash から borrow）が設計不一致で NO-GO になった。
- 代替として **OwnerSharedStashBox**（owner 別共有 stash / MPSC）を検討したい。

前提: Box Theory
- hot path を汚さない
- event-only で境界を 1 箇所に集約
- compile-time A/B で戻せる

---

## 1) 直近の成果（S42）

S42 Small Transfer Cache Box（scale lane）で大幅改善:
- T=32 R=50 16–2048: **+128%**
- T=32 R=0 16–2048: **+136%**
- T=32 R=50 16–32768: **+281%**

S42 は GO、scale lane でデフォルト化済み。

---

## 2) S43 NO-GO 理由（設計不一致）

- TLS stash は **自分が他 shard へ送る free** 専用で、借り先として不適合
- FIFO ring から特定 bin を抜くのが重い
- `dst != my_shard` を借りると owner 境界を壊す

結論: TLS stash borrow は NO-GO

---

## 3) 相談したい新設計（OwnerSharedStashBox）

### アイデア
owner 別に「共有 stash」を持ち、owner 宛の戻りを直接参照できる箱を作る。

候補:
- `g_owner_stash[owner][sc]` で MPSC
- `push_list` / `pop_batch` の event-only API
- 既存 inbox/central と役割を分ける

### 相談したい点
1) **owner stash の役割は inbox と何が違うべき？**
2) **MPSC で lock-free を狙うべきか？** それとも spin lock で十分か？
3) **S42 transfer cache と役割が被らない設計境界**はどこに置くべきか？
4) **hot path を汚さず**に「borrow」に近い効果を出せる最小設計は？
5) **失敗時の戻しやすさ（A/B）**を確保する最小 API は？

---

## 4) 参照してほしいファイル

- `hakozuna/hz3/src/hz3_tcache.c`（remote stash/flush）
- `hakozuna/hz3/src/hz3_small_v2.c`（small central / transfer）
- `hakozuna/hz3/src/hz3_inbox.c`
- `hakozuna/hz3/src/hz3_central.c`
- `hakozuna/hz3/include/hz3_types.h`
- `hakozuna/hz3/include/hz3_config.h`

