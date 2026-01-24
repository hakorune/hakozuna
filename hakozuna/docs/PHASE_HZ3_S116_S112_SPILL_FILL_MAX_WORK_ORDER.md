# PHASE HZ3 S116: S112 SpillFillMaxBox（full drain 後の spill_array 充填を制限）— Work Order

目的（ゴール）:
- xmalloc-testN のように owner stash が “大量に溜まってから drain” されるケースで、
  S112（atomic_exchange full drain）後に **spill_array を CAP まで埋めるための余計な walk + store** を削る。
- 余剰は `spill_overflow`（head only）に残すことで、stash_pop の追加 work を抑える。

背景:
- perf で `hz3_owner_stash_pop_batch` が Top1 を占めるケースがある。
- S112 は CAS retry を消したが、drain list が大きいと “spill_array 充填” が支配しうる。

箱の境界（1箇所）:
- `hakozuna/hz3/src/hz3_owner_stash.c` の `hz3_owner_stash_pop_batch()` 内、
  S112 ブロックの “spill_array fill” ループのみ。

フラグ:
- `HZ3_S116_S112_SPILL_FILL_MAX=...`
  - 既定: `HZ3_S67_SPILL_CAP`（従来どおり CAP まで埋める）
  - `0`: spill_array を一切埋めず、余剰はすべて `spill_overflow` に保存
  - 小さめ（例: `8`/`16`）: spill_array のメリットは残しつつ、余計な walk を抑える

期待:
- “drain が大きい” workload で `hz3_owner_stash_pop_batch` の self% を下げる。

---

## A/B（推奨）

ビルド例:
```sh
make -C hakozuna/hz3 clean all_ldpreload_scale
cp ./libhakozuna_hz3_scale.so /tmp/s116_base.so

make -C hakozuna/hz3 clean all_ldpreload_scale \
  HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_S116_S112_SPILL_FILL_MAX=0'
cp ./libhakozuna_hz3_scale.so /tmp/s116_treat0.so

make -C hakozuna/hz3 clean all_ldpreload_scale \
  HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_S116_S112_SPILL_FILL_MAX=16'
cp ./libhakozuna_hz3_scale.so /tmp/s116_treat16.so
```

最低スイート:
- `xmalloc-testN`（T=8）
- `mt_remote_r50_small` / `dist_app`（回帰チェック）

GO/NO-GO:
- GO: xmalloc-test が +1% 以上（または perf で pop_batch self% が有意に下がる）かつ回帰が小さい
- NO-GO: xmalloc-test が改善しない/退行、または dist/r50 が -1% 超退行

---

## 結果（A/B）

結論: **NO-GO**（workload 依存で符号が反転し、scale lane 既定にできない）。

- xmalloc-testN T=8（median）:
  - Base: 82.42M
  - Treat0（FILL_MAX=0）: 82.71M（+0.34%）
  - Treat16（FILL_MAX=16）: 81.85M（-0.69%）
- mixed bench（median）:
  - Treat0（FILL_MAX=0）: **-1.37%**（回帰）
  - Treat16（FILL_MAX=16）: +0.37%（誤差域）

運用:
- `HZ3_S116_S112_SPILL_FILL_MAX` は既定（`HZ3_S67_SPILL_CAP`）のまま維持。
- 研究用の opt-in としてのみ残す。
