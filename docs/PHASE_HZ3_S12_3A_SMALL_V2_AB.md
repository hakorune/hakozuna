# PHASE_HZ3_S12_3A_SMALL_V2_AB

Date: 2025-12-31
Git: 77404758a
Bench: hakozuna/out/bench_random_mixed_malloc_args
Runs: 10
Iters: 20000000
WS: 400

## 用語（混同防止）

- `hakozuna`（従来本線）: `hakozuna/` 配下の allocator（S5/S6/LCache 等）。
- `hz3`: `hakozuna/hz3/` 配下の “別トラック” allocator（Learning-first bootstrap）。
- `Small v1`（hz3）: `hz3` の既存 small 実装（segmap + segmeta(tag) で ptr→分類）。
- `Small v2`（hz3）: 今回追加した self-describing small 実装（arena + seg hdr + page hdr で ptr→分類、segmap不要）。

この A/B は **hz3 の Small v1 vs Small v2** の比較であり、hakozuna 本線の small 実装との比較ではありません。

## Builds

v1 (Small v2 OFF):
```
CFLAGS="-O3 -fPIC -Wall -Wextra -Werror -std=c11 \
  -DHZ3_ENABLE=1 -DHZ3_SHIM_FORWARD_ONLY=0 \
  -DHZ3_SMALL_V2_ENABLE=0 -DHZ3_SEG_SELF_DESC_ENABLE=0"
make -C hakozuna/hz3 all_ldpreload
```

v2 (Small v2 ON):
```
CFLAGS="-O3 -fPIC -Wall -Wextra -Werror -std=c11 \
  -DHZ3_ENABLE=1 -DHZ3_SHIM_FORWARD_ONLY=0 \
  -DHZ3_SMALL_V2_ENABLE=1 -DHZ3_SEG_SELF_DESC_ENABLE=1"
make -C hakozuna/hz3 all_ldpreload
```

## Logs

- v1: /tmp/hz3_ssot_77404758a_20251231_162701
- v2: /tmp/hz3_ssot_77404758a_20251231_162749

## Median (ops/s)

| Range (bytes) | v1 (Small v1) | v2 (Small v2) | Delta (v2-v1) |
|--------------|---------------|---------------|---------------|
| 16-2048      | 85.61M        | 71.96M        | -15.9%        |
| 4096-32768   | 97.26M        | 73.98M        | -23.9%        |
| 16-32768     | 89.23M        | 70.88M        | -20.6%        |

## GO/NO-GO

NO-GO. Small/medium/mixed all regress with v2 under the same SSOT lane.

Next: investigate v2 overhead (arena/seg_hdr/page_hdr path) before enabling by default.

Update:
- S12-4（PageTagMap）で v2 の hot 固定費を大幅に削減し、small/medium は大きく改善した。
  結果: `hakozuna/hz3/docs/PHASE_HZ3_S12_4_RESULTS.md`
