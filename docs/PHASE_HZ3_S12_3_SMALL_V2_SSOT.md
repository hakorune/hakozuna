# PHASE_HZ3_S12-3: Small v2（self-describing）SSOT

目的:
- small（16B–2048B）を self-describing 化し、`free()` の ptr→分類で segmap 依存を消す。
- hz3 単体 `LD_PRELOAD` で small/medium/mixed が強い状態を SSOT 化する。

参照:
- 設計: `hakozuna/hz3/docs/PHASE_HZ3_SMALL_V2_SELF_DESC_DESIGN.md`
- SSOT lane: `hakozuna/docs/SSOT_THREE_LANES.md`
- SSOT script: `hakozuna/hz3/scripts/run_bench_hz3_ssot.sh`

---

## Build

```bash
make -C hakozuna/hz3 clean
CFLAGS="-O3 -fPIC -Wall -Wextra -Werror -std=c11 \
  -DHZ3_ENABLE=1 -DHZ3_SHIM_FORWARD_ONLY=0 \
  -DHZ3_SMALL_V2_ENABLE=1 -DHZ3_SEG_SELF_DESC_ENABLE=1" \
  make -C hakozuna/hz3 all_ldpreload
```

---

## SSOT Run

```bash
SKIP_BUILD=1 RUNS=10 ITERS=20000000 WS=400 \
  ./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh
```

ログ出力先:
- `/tmp/hz3_ssot_${git}_YYYYmmdd_HHMMSS/`

このフェーズの例（報告ログ）:
- `/tmp/hz3_ssot_88e9ef214_20251231_162105`

---

## Results（median, ops/s）

| Range | system | hz3 (small v2 ON) |
|------:|-------:|-------------------:|
| small (16–2048) | 35.21M | 71.68M |
| medium (4096–32768) | 13.75M | 75.68M |
| mixed (16–32768) | 15.63M | 70.84M |

