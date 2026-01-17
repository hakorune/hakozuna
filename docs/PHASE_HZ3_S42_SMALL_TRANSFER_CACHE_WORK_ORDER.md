# S42: Small Transfer Cache Box (remote-heavy small gap) — Work Order

Status: **COMPLETE (GO)**

目的:
- `T=32 / R=50 / size=16–2048` の remote-heavy small で tcmalloc 差（-27%）を詰める。
- hot path を汚さず、**event-only の箱**で `small central (mutex+list)` の負担を削る。

背景（perf）:
- hz3-scale で `pthread_mutex_lock/unlock` と `hz3_small_v2_central_push_list` が目立つ。
- small の central が remote-heavy のボトルネックと推定。

---

## 1) Box 定義（境界1箇所）

**SmallTransferCacheBox**

役割:
- remote から戻る small list を **中央(mutex)ではなく transfer cache** で吸収する。
- alloc miss 時は transfer から先に取り、足りないときだけ central に落ちる。

API（event-only）:
- `hz3_small_xfer_push_list(owner, sc, head, tail, n)`
- `hz3_small_xfer_pop_batch(owner, sc, out[], want)`

境界:
- `hz3_small_v2_push_remote_list()` → xfer へ
- `hz3_small_v2_central_pop_batch()` の前段に xfer を挿入

戻し:
- `HZ3_S42_SMALL_XFER=0/1` で A/B
- fast lane には入れない（scale lane 専用）

---

## 2) 最小実装（第1段）

データ構造（global, owner x sc）:
```
typedef struct {
    atomic_flag lock;
    uint16_t count;
    void* slots[HZ3_S42_SMALL_XFER_SLOTS];
} Hz3SmallXferBin;
```

Push:
- lock → `slots` に詰められるだけ詰める（O(min(n,空き)))
- 余りは **中央へそのまま送る**（既存 central push）

Pop:
- lock → `slots` から最大 `want` だけ取り出す
- 足りなければ既存 central から補充

ポイント:
- 連結リストの while(pop) を減らし、mutex の濃度を下げる
- hot path には入れない（event-only で完結）

---

## 3) フラグ

compile-time:
- `HZ3_S42_SMALL_XFER=0/1`（default 0）
- `HZ3_S42_SMALL_XFER_SLOTS=64`（仮）

有効範囲:
- scale lane（`HZ3_REMOTE_STASH_SPARSE=1`）だけで有効化する

---

## 4) 計測（SSOT）

対象:
- `T=32 / R=50 / size=16–2048`（remote-heavy small）
- `T=32 / R=0 / size=16–2048`（固定費）
- `T=32 / R=50 / size=16–32768`（mixed 影響）

GO/NO-GO:
- GO: `T=32 / R=50 / 16–2048` が +10% 以上改善
- NO-GO: `R=0` 固定費が -5% 以上退行 or mixed が -1% 以上退行

観測:
- event-only のワンショットで十分
  - `xfer_push_calls`, `xfer_pop_hit`, `xfer_pop_miss`

---

## 5) 作業ステップ

1. 新箱を追加（`hz3_small_xfer.c/.h`）
2. `hz3_small_v2_push_remote_list()` を xfer 経由に
3. alloc miss で `xfer_pop_batch` を先に試す
4. A/B 測定（scale lane のみ）
5. SSOT 更新（結果/ログ/GO判定）

---

## 6) 実測結果（報告）

結果サマリ（reported medians）:

| 条件 | Baseline | Treatment | Delta | 判定 |
|------|----------|-----------|-------|------|
| T=32 R=50 16–2048 | 42.7M | 97.3M | +128% | GO |
| T=32 R=0 16–2048 | 171M | 404M | +136% | GO |
| T=32 R=50 16–32768 | 23.7M | 90.3M | +281% | GO |

メモ:
- scale lane で `HZ3_S42_SMALL_XFER=1` をデフォルト化済み（fast lane は未変更）。
- ログパスは計測側の SSOT に追記すること。

---

## 7) 参照ファイル

- `hakozuna/hz3/src/hz3_small_v2.c`
- `hakozuna/hz3/src/hz3_tcache.c`
- `hakozuna/hz3/src/hz3_hot.c`
- `hakozuna/hz3/src/hz3_small_xfer.c`
- `hakozuna/hz3/include/hz3_config.h`
- `hakozuna/hz3/Makefile`

---

## 8) S42-1: XferPop Prefetch MicroOpt（候補）

背景:
- `hz3_small_xfer_pop_batch()` は、`chain.head` から `obj->next` を追う walk がある。
- remote-heavy では remote object がコールドになりやすく、`hz3_obj_get_next(cur)` が cache miss になりやすい。

箱の境界（1箇所）:
- `hakozuna/hz3/src/hz3_small_xfer.c` の `hz3_small_xfer_pop_batch()` の walk loop だけを触る。

フラグ（compile-time のみ、既定 OFF）:
- `HZ3_S42_XFER_POP_PREFETCH=0/1`
- `HZ3_S42_XFER_POP_PREFETCH_DIST=1/2`

A/B:
- r50/r90（3run median）に加えて、`R=0`（固定費）も必ず取る。
- build example:
  - `make -C hakozuna/hz3 clean all_ldpreload_scale HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_S42_XFER_POP_PREFETCH=1 -DHZ3_S42_XFER_POP_PREFETCH_DIST=1'`

### S42-1 結果（2026-01-13）: NO-GO

| 条件 | Baseline | Prefetch (dist=1) | 変化 |
|------|----------|-------------------|------|
| r90 | 93.44M | 91.55M | **-2.0%** |
| r50 | 125.16M | 118.22M | **-5.5% NO-GO** |
| R=0 | 428.7M | 424.3M | -1.0% |

原因推測:
- S44 owner_stash（MPSC, lock-free）と異なり、S42 xfer は mutex 保護下で動作
- xfer cache 経由のオブジェクトは既にやや温かく、prefetch の効果が薄い
- prefetch 命令のオーバーヘッドが利得を上回った

**scale 既定には入れない**（`HZ3_S42_XFER_POP_PREFETCH=0` を維持）
