# PHASE_HZ3_S54: SegmentPageScavengeBox（OBSERVE）

目的:
- S53（large cache）では RSS があまり下がらないケースがあるため、RSS の主成分が **segment/medium 側**にあるかを切り分ける。
- hot path に固定費を入れず、event-only（S46 pressure handler）で **統計だけ**を取る。

結論（このフェーズ）:
- **OBSERVEのみ**（`madvise` はしない）。one-shot で「削減ポテンシャル」を可視化して、次の FROZEN/LEARN に進むか判断する。

判定（現状）:
- pressure を意図的に発生させた実測でも `max_potential_bytes` は **~1MB 程度**で、segment/medium 側の page-scavenge は RSS 削減の主因になりにくい。
- よって **Phase 2（FROZEN / 実madvise）は現状 NO-GO**。必要なら OBSERVE を計測ツールとして残す。

---

## フラグ

- `HZ3_SEG_SCAVENGE_OBSERVE=0/1`
  - default: 0（opt-in）
  - 1 のとき、終了時に `[HZ3_SEG_SCAVENGE_OBS]` を 1 回だけ出す（atexit）。
- `HZ3_SEG_SCAVENGE_MIN_CONTIG_PAGES=...`
  - default: 32（=128KiB）
  - “madvise候補” とみなす最低連続 free pages（OBSERVEでは候補数の推定にのみ使用）。

---

## 統計の取り方（箱の境界）

入口:
- S46 pressure handler（`hz3_pressure_check_and_flush()`）の event-only タイミングで `hz3_seg_scavenge_observe()` を呼ぶ。

観測対象:
- my_shard の pack pool（S49 SegmentPackingBox）を **観測API**経由で read-only 走査する。
  - `hz3_pack_observe_owner(owner, ...)`
  - `extern g_pack_pool` のような内部構造露出はしない（箱の境界を維持）。

---

## 出力（one-shot）

形式:
```
[HZ3_SEG_SCAVENGE_OBS] calls=<n> max_segments=<n> max_free_pages=<n> max_candidate_pages=<n> max_potential_bytes=<n>
```

意味:
- `calls`: observe が呼ばれた回数（pressureが発火した回数の目安）
- `max_segments`: pack pool に見えている segment 数（最大値）
- `max_free_pages`: free_pages の合計（最大値）
- `max_candidate_pages`: `max_contig_free >= MIN_CONTIG` の “候補ページ” 推定（最大値）
- `max_potential_bytes`: `max_candidate_pages * 4096`（概算）

---

## ビルド/実行

```bash
make -C hakozuna/hz3 clean all_ldpreload_scale \
  HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_SEG_SCAVENGE_OBSERVE=1'
```

例（mstress）:
```bash
env -u LD_PRELOAD LD_PRELOAD=./libhakozuna_hz3_scale.so \
  ./mimalloc-bench/out/bench/mstress 2>&1 | rg 'HZ3_SEG_SCAVENGE_OBS'
```

---

## `calls=0` について（正常）

`calls=0` は **pressure が発火していない**だけ（実装不具合ではない）。

- OBSERVE は S46 pressure handler にしかフックしていないため、pressure が来ない実行では観測が 0 回になる。
- scale lane は `HZ3_ARENA_SIZE=64GB`（`hakozuna/hz3/Makefile`）なので、短いベンチだと pressure が来ないことがある。

---

## pressure を意図的に発生させて観測する（推奨）

S54 は **event-only** 観測なので、評価時は「pressure を起こす」設定を別 lane（A/B）として切るのが安全。

### 1) arena を小さくしてビルド（A/B）

例（まず 8GB → ダメなら 4GB → 2GB）:

```bash
make -C hakozuna/hz3 clean all_ldpreload_scale \
  HZ3_SCALE_ARENA_SIZE=0x200000000ULL \
  HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_SEG_SCAVENGE_OBSERVE=1'

# 4GB
make -C hakozuna/hz3 clean all_ldpreload_scale \
  HZ3_SCALE_ARENA_SIZE=0x100000000ULL \
  HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_SEG_SCAVENGE_OBSERVE=1'
```

### 2) segment/medium を使うワークロードで実行

`mstress` は pressure が来ないことがあるため、まずは hz3 内製の MT mixed（16–32768）で確認する。

```bash
env -u LD_PRELOAD LD_PRELOAD=./libhakozuna_hz3_scale.so \
  ./hakozuna/out/bench_random_mixed_mt_remote_malloc 32 5000000 400 16 32768 50 \
  2>&1 | rg 'HZ3_SEG_SCAVENGE_OBS|alloc failed'
```

注意:
- 出力は atexit one-shot なので、異常終了（abort/segv）だと出ない。
  - まずは iters を下げる等で「完走」優先で観測する。

---

## 実測例（参考）

8GB arena（`HZ3_SCALE_ARENA_SIZE=0x200000000ULL`）で MT mixed を回して pressure を発生させた例:

```
[HZ3_SEG_SCAVENGE_OBS] calls=11 max_segments=9 max_free_pages=317 max_candidate_pages=247 max_potential_bytes=1011712
```

```
[HZ3_SEG_SCAVENGE_OBS] calls=27 max_segments=8 max_free_pages=216 max_candidate_pages=77 max_potential_bytes=315392
```

読み取り:
- `max_potential_bytes` が **~0.3–1.0MB 程度**だと、segment/medium 側の “contig free pages” の RSS 削減ポテンシャルは小さい。
- この場合は S54（page-scavenge）を先に進めるより、large cache（S53）側や別の RSS 主因を疑う方が早い。

---

## 次フェーズへの判定（FROZEN）

次のどれかを満たすなら、FROZEN（実 `madvise`）を検討:
- `max_potential_bytes` が十分大きい（例: 数百MB）
- `max_candidate_pages / max_free_pages` がそれなりに高い

逆に、`max_candidate_pages` がほぼ 0 の場合:
- RSS の主成分が “large cache 以外（segment/medium 以外）” か、
- free page の連続性（contig）が弱すぎて、page単位 release が効きにくい可能性がある。

---

## S54 の扱い（台帳）

- **本線に残す**: OBSERVE は「主因の切り分け器」として有用。
- **Phase 2（実 `madvise`）は凍結**: 現状の観測では RSS 改善の見込みが薄い（費用対効果が悪い）。
