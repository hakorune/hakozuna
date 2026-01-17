# S43: Small Borrow Box (remote-heavy small gap) — Work Order

Status: **NO-GO**

目的:
- S42 で改善した remote-heavy small の残差をさらに詰める。
- hot path を汚さず、**alloc miss（event-only）**で借りて埋める。

背景:
- remote-heavy では “自分が free した remote” が高確率で存在する。
- owner 側の central/lock を経由せずに **自分の stash から借りる**と、slow alloc を減らせる。

---

## 1) NO-GO 理由（設計不一致）

- remote stash（TLS ring）は **「自分が他 shard へ送る free」専用**であり、
  `dst == my_shard` のエントリは基本存在しない。
  - 借りる対象が自 shard にならず、owner 境界を崩す。
- ring は FIFO で、**特定 bin のみを抜く**のが構造的に重い。
  - 削除ロジックが複雑化し、event-only でも固定費が増える。
- `dst != my_shard` を借りるのは **owner 境界の破壊**で Box Theory に反する。

結論: TLS stash からの borrow は **設計前提と不整合**のため NO-GO。

---

## 2) 次の設計候補（別Box）

- **OwnerSharedStashBox**（owner 別の共有 stash / MPSC）
  - これなら owner 宛の “戻り” を直接参照できる
  - ただし inbox/central に近い構造になり、別トラックとして設計が必要

---

## 3) アーカイブ

- `hakozuna/hz3/archive/research/s43_small_borrow_box/README.md`

---

## 4) 参考（当初の案）

以下は NO-GO となった旧案（記録用）。

### Box 定義（境界1箇所）

**SmallBorrowBox**

役割:
- `hz3_small_v2_alloc_slow()` の miss で、**自分の remote stash から同一 bin を借りる**。
- 借りたオブジェクトは tag の owner を変えずに使う（free 時に自然に remote 返却される）。

API（event-only）:
- `hz3_small_borrow_from_remote(bin, want, out[], *got)`

境界:
- `hz3_small_v2_alloc_slow()` の central pop の前段

戻し:
- `HZ3_S43_SMALL_BORROW=0/1` で A/B
- scale lane のみ有効化

---

### 最小実装案

対象:
- scale lane の sparse ring (`Hz3RemoteStashRing`)

方針:
- ring を **限定スキャン**（例: 8/16 entry）
- `entry.bin == target_bin` のものだけを回収して local bin に push
- それ以外は **通常の dispatch**（owner 側へ flush）する

擬似コード:
```
borrowed = 0;
scan = HZ3_S43_SMALL_BORROW_SCAN;
while (scan-- && ring not empty && borrowed < want) {
  if (entry.bin == target_bin) {
    out[borrowed++] = entry.ptr;  // local bin へ
  } else {
    dispatch(entry);              // owner へ
  }
  advance ring cursor
}
```

ポイント:
- hot path には入れない（alloc miss のみ）
- owner/tag は変更しない（整合性は維持）
- scan 上限で固定費を制御

---

### フラグ

compile-time:
- `HZ3_S43_SMALL_BORROW=0/1`（default 0）
- `HZ3_S43_SMALL_BORROW_SCAN=8`（仮）

有効範囲:
- scale lane（`HZ3_REMOTE_STASH_SPARSE=1`）のみ

---

### 計測（SSOT）

対象:
- `T=32 / R=50 / size=16–2048`（main）
- `T=32 / R=0 / size=16–2048`（固定費）
- `T=32 / R=50 / size=16–32768`（mixed）

GO/NO-GO:
- GO: `T=32 / R=50 / 16–2048` が +10% 以上改善
- NO-GO: `R=0` 固定費が -5% 以上退行 or mixed が -1% 以上退行

観測（event-only, 1回だけ）:
- `borrow_hit`, `borrow_miss`, `borrow_scan`

---

### 作業ステップ

1. borrow API を追加（`hz3_small_borrow.c/.h`）
2. `hz3_small_v2_alloc_slow()` に borrow を差し込む
3. ring scan budget を固定（compile-time）
4. A/B 測定（scale lane のみ）
5. SSOT 更新（結果/ログ/GO判定）

---

### 参照ファイル

- `hakozuna/hz3/src/hz3_small_v2.c`
- `hakozuna/hz3/src/hz3_tcache.c`
- `hakozuna/hz3/src/hz3_hot.c`
- `hakozuna/hz3/include/hz3_types.h`
- `hakozuna/hz3/include/hz3_config.h`
- `hakozuna/hz3/Makefile`
