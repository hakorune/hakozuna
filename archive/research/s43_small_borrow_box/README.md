# S43: Small Borrow Box (TLS stash borrow) — NO-GO

## 目的
S42 で改善した remote-heavy small の残差を詰めるため、
alloc miss（event-only）で TLS stash から同一 bin を「借りる」箱を試す。

## 変更案（当初）
- `hz3_small_v2_alloc_slow()` の miss で TLS stash をスキャン
- 同一 bin の entry を local bin に push
- それ以外は通常 dispatch

## GO 条件
- `T=32 / R=50 / 16–2048` が +10% 以上改善
- `R=0` 固定費 -5% 以内
- mixed -1% 以内

## 結果: NO-GO（設計不一致）

### 理由
- TLS stash は **自分が他 shard に送る free** 専用で、`dst == my_shard` がほぼ存在しない。
- ring は FIFO で **特定 bin 抜き**が構造的に重い。
- `dst != my_shard` を借りると owner 境界を破壊し、Box Theory に反する。

## 代替案
- **OwnerSharedStashBox**（owner 別の共有 stash / MPSC）として別トラックで再設計。

## 参照
- 指示書: `hakozuna/hz3/docs/PHASE_HZ3_S43_SMALL_BORROW_BOX_WORK_ORDER.md`

