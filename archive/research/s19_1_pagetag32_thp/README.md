# S19-1 NO-GO: PTAG32 THP / TLB 対策（`MADV_HUGEPAGE` init-only）

目的:
- `mixed (16–32768)` の PTAG32（tag32 table）ランダムアクセスで dTLB 負けが支配になるなら、init-only で THP を促して改善できるか確認する。
- hot path に命令を追加しない（Box Theory: event-only）。

実装（本線に残っているがデフォルトOFF）:
- gate: `HZ3_PTAG32_HUGEPAGE=0/1`（default 0）
- `hakozuna/hz3/src/hz3_arena.c` で PTAG32 `mmap` 後に `madvise(..., MADV_HUGEPAGE)`（best-effort）

GO条件:
- SSOT（`RUNS=30`）で `small/medium` 非退行、`mixed` が安定して改善（+0.5% 以上の再現性）

結論: **NO-GO**
- 変動が大きく、`mixed` が改善する場合もあるが、別条件/別回では悪化も観測された。
- `mixed` の baseline `dTLB-load-misses` がそもそも小さく（測定では桁が低い）、改善余地が限定的。
- THP/コンパクション/ページ移動などOS都合の揺れに飲まれやすい（再現性が弱い）。

SSOTログ（参考）:
- 以前の“効いた”側（指示書に記録）:
  - baseline（THP=0）: `/tmp/hz3_ssot_6cf087fc6_20260101_192633`
  - THP=1: `/tmp/hz3_ssot_6cf087fc6_20260101_192834`
- 追加の `RUNS=30` 再検証（このREADME作成時の再測定）:
  - baseline（THP=0）: `/tmp/hz3_ssot_6cf087fc6_20260101_200035`
  - THP=1: `/tmp/hz3_ssot_6cf087fc6_20260101_200245`
  - この回は `mixed` が **-2.08%**（THP vs baseline）で悪化

perf stat ログ（参考）:
- baseline（THP=0）: `/tmp/hz3_s19_1_perf_baseline.txt` / `/tmp/hz3_s19_1_perf_baseline_now.txt`
- THP=1: `/tmp/hz3_s19_1_perf_thp.txt` / `/tmp/hz3_s19_1_perf_thp_now.txt`

再現コマンド（同条件）:
```sh
cd /mnt/workdisk/public_share/hakmem

# baseline (THP=0)
make -C hakozuna/hz3 clean
make -C hakozuna/hz3 all_ldpreload -j8 \
  HZ3_LDPRELOAD_DEFS='-DHZ3_ENABLE=1 -DHZ3_SHIM_FORWARD_ONLY=0 \
    -DHZ3_SMALL_V2_ENABLE=1 -DHZ3_SEG_SELF_DESC_ENABLE=1 \
    -DHZ3_SMALL_V2_PTAG_ENABLE=1 -DHZ3_PTAG_V1_ENABLE=1 \
    -DHZ3_PTAG_DSTBIN_ENABLE=1 -DHZ3_PTAG_DSTBIN_FASTLOOKUP=1 \
    -DHZ3_PTAG32_HUGEPAGE=0'
SKIP_BUILD=1 RUNS=30 ITERS=20000000 WS=400 ./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh

# THP=1
make -C hakozuna/hz3 clean
make -C hakozuna/hz3 all_ldpreload -j8 \
  HZ3_LDPRELOAD_DEFS='-DHZ3_ENABLE=1 -DHZ3_SHIM_FORWARD_ONLY=0 \
    -DHZ3_SMALL_V2_ENABLE=1 -DHZ3_SEG_SELF_DESC_ENABLE=1 \
    -DHZ3_SMALL_V2_PTAG_ENABLE=1 -DHZ3_PTAG_V1_ENABLE=1 \
    -DHZ3_PTAG_DSTBIN_ENABLE=1 -DHZ3_PTAG_DSTBIN_FASTLOOKUP=1 \
    -DHZ3_PTAG32_HUGEPAGE=1'
SKIP_BUILD=1 RUNS=30 ITERS=20000000 WS=400 ./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh
```

復活条件（もし再挑戦するなら）:
- OS/カーネル条件込みで “効く” 側が安定再現できる（= 1台/1カーネル固定で `RUNS=30` を複数回回しても改善が安定）。
- もしくは THP を使わず、tagmap 自体の配置/サイズ/アクセスパターンで勝つ（S17/S18系の延長）。

