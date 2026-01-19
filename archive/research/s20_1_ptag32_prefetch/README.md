# S20-1 NO-GO: PTAG32 prefetch（hot +1命令）

目的:
- `mixed (16–32768)` の `hz3_free` で PTAG32（dst/bin直結）ロードが支配なら、in-range確定後に 1要素だけ prefetch して改善できるか確認する。

実装（本線に残っているがデフォルトOFF）:
- gate: `HZ3_PTAG32_PREFETCH=0/1`（default 0）
- `hakozuna/hz3/include/hz3_arena.h` の `hz3_pagetag32_lookup_fast()` 内で、tag32 load 直前に `__builtin_prefetch()` を追加。

GO条件:
- `RUNS=30` の SSOT で `mixed +0.5%` 以上、かつ `small/medium` 非退行（±0%）

結論: **NO-GO**
- `small` が **-1.9%** 退行し、`mixed` も **+0.42%** と目標未達。
- `medium` は大きく伸びたが、`mixed` が主戦場で、かつ `small` 退行は許容できない。

SSOTログ（RUNS=30, ITERS=20000000, WS=400）:
- baseline（prefetch=0）: `/tmp/hz3_ssot_6cf087fc6_20260101_203735`
- prefetch=1: `/tmp/hz3_ssot_6cf087fc6_20260101_203946`

median（hz3）差分（prefetch vs baseline）:
- small:  -1.91%
- medium: +5.97%
- mixed:  +0.42%

再現コマンド:
```sh
cd /mnt/workdisk/public_share/hakmem

# baseline (prefetch=0)
make -C hakozuna/hz3 clean
make -C hakozuna/hz3 all_ldpreload -j8 \
  HZ3_LDPRELOAD_DEFS='-DHZ3_ENABLE=1 -DHZ3_SHIM_FORWARD_ONLY=0 \
    -DHZ3_SMALL_V2_ENABLE=1 -DHZ3_SEG_SELF_DESC_ENABLE=1 \
    -DHZ3_SMALL_V2_PTAG_ENABLE=1 -DHZ3_PTAG_V1_ENABLE=1 \
    -DHZ3_PTAG_DSTBIN_ENABLE=1 -DHZ3_PTAG_DSTBIN_FASTLOOKUP=1 \
    -DHZ3_PTAG32_PREFETCH=0'
SKIP_BUILD=1 RUNS=30 ITERS=20000000 WS=400 ./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh

# experiment (prefetch=1)
make -C hakozuna/hz3 clean
make -C hakozuna/hz3 all_ldpreload -j8 \
  HZ3_LDPRELOAD_DEFS='-DHZ3_ENABLE=1 -DHZ3_SHIM_FORWARD_ONLY=0 \
    -DHZ3_SMALL_V2_ENABLE=1 -DHZ3_SEG_SELF_DESC_ENABLE=1 \
    -DHZ3_SMALL_V2_PTAG_ENABLE=1 -DHZ3_PTAG_V1_ENABLE=1 \
    -DHZ3_PTAG_DSTBIN_ENABLE=1 -DHZ3_PTAG_DSTBIN_FASTLOOKUP=1 \
    -DHZ3_PTAG32_PREFETCH=1'
SKIP_BUILD=1 RUNS=30 ITERS=20000000 WS=400 ./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh
```

復活条件:
- `mixed` が安定して +0.5% 以上かつ `small` 非退行（複数回 `RUNS=30` で確認）。

