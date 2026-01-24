# PHASE HZ3 S114: PTAG32 TLS CacheBox（arena_base / tag32_base の atomic load を避ける）— Work Order

目的（ゴール）:
- `hz3_pagetag32_lookup_hit_fast()`（free leaf / realloc / usable_size 等で使われる）から
  `g_hz3_arena_base` の atomic load を避け、mimalloc の segmap bit-check に近づける。
- “挙動不変” のまま fixed-cost を落とす（A/B 可能、切り戻し可能）。

背景:
- xmalloc-testN のような free-heavy workload では PTAG32 hit が支配的。
- PTAG32 hit の最初の `g_hz3_arena_base` atomic load が固定費になりやすい（差分見積り: ~0.7%）。

箱割り（境界は1箇所）:
- 境界は `hakozuna/hz3/include/hz3_arena.h` の inline:
  - `hz3_arena_page_index_fast()`
  - `hz3_pagetag32_lookup_fast()`
  - `hz3_pagetag32_lookup_hit_fast()`

ここでのみ:
- `t_hz3_cache.arena_base` と `t_hz3_cache.page_tag32` を TLS にキャッシュし、NULL のときだけ global を読む。

フラグ:
- `HZ3_S114_PTAG32_TLS_CACHE=0/1`（default OFF, research opt-in）
  - 実装フラグ: `HZ3_PTAG_DSTBIN_TLS`（`hz3_config_small_v2.h` で `S114` と同義に正規化）

設計:
- TLS は zero-init なので、tcache “未初期化” でも `arena_base/page_tag32` の read/write は安全。
- NULL のときだけ:
  - `arena_base = atomic_load(g_hz3_arena_base, acquire)` を 1回行い TLS に保存
  - `page_tag32 = g_hz3_page_tag32` を TLS に保存
- hot path（hit）では TLS load のみで進む。

---

## A/B（推奨）

ビルド例（scale lane）:
```sh
make -C hakozuna/hz3 clean all_ldpreload_scale
cp ./libhakozuna_hz3_scale.so /tmp/s114_base.so

make -C hakozuna/hz3 clean all_ldpreload_scale \
  HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_S114_PTAG32_TLS_CACHE=1'
cp ./libhakozuna_hz3_scale.so /tmp/s114_treat.so
```

最低スイート:
- `xmalloc-testN`（T=8）
- `dist_app` / `dist_uniform`（回帰チェック）
- `mt_remote_r50_small`（回帰チェック）

GO/NO-GO:
- GO: xmalloc-test が +0.5% 以上、かつ dist/r50 が -1% を超えて退行しない
- NO-GO: xmalloc-test が改善しない（±0%）か、dist/r50 が -1% 超退行

---

## 結果（2026-01-14）

A/B:
- xmalloc-testN T=8: base **83.40M** → treat **82.84M**（**-0.67%**）
- mixed bench: base **108.11M** → treat **107.37M**（**-0.68%**）

判定:
- **NO-GO**（既定OFF維持）

原因（観測）:
- base は `g_hz3_arena_base` の global load が単純で軽い（GOT → load）。
- treat は TLS `%fs:` アクセス + 追加レジスタ（例: `%rbx`）が入り、わずかに重くなった可能性が高い。

---

## S113 との関係

- S113（SegMath free fastdispatch）は NO-GO だが、S114 は “PTAG32 をさらに速くする箱” なので独立に価値がある。
- もし S113 を再評価するなら、S114 を ON にした上で S113 の A/B を取り直す（arena_base load の差分が消えるため）。
