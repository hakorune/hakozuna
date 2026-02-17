# HZ4 Phase 12B: MagBox（MAG_CAP sweep）SSOT 10runs 指示書

目的: Magazine TCache の cap を振って、**+5%** に届く余地があるかを SSOT 10runs median で確定する。

箱理論（Box Theory）
- **箱**: `MagBox`（既に実装済み）
- **境界1箇所**: `HZ4_TCACHE_MAG_CAP` だけを変えて A/B
- **戻せる**: compile-time 定数のみ（本線のロジックは同じ）
- **見える化**: SSOT 10runs median + R=90 sanity（1回）
- **Fail-Fast**: build/test 落ちたら即停止

---

## 0) SSOT 条件（固定）

- bench: `hakozuna/out/bench_random_mixed_mt_remote_malloc`
- 条件: `T=16 iters=2000000 ws=400 size=16..2048 R=0 ring=65536`
- pinning: `taskset -c 0-15`
- runs: **10**（median）

ログ置き場:
```sh
cd /mnt/workdisk/public_share/hakmem
OUT=/tmp/hz4_phase12b_magcap_sweep_$(date +%Y%m%d_%H%M%S)
mkdir -p "$OUT"
```

---

## 1) 比較する CAP 候補

まずは 4 点（増やしすぎない）:
- `HZ4_TCACHE_MAG_CAP=8`（現状の基準）
- `HZ4_TCACHE_MAG_CAP=16`
- `HZ4_TCACHE_MAG_CAP=32`
- `HZ4_TCACHE_MAG_CAP=64`

---

## 2) 10runs（R=0）を CAP ごとに実行

共通の build flags（perf lane）:
※ 現状は `HZ4_TCACHE_MAGAZINE/HZ4_TCACHE_MAG_CAP` の実装が本線に無いので、そのままではビルドできません（研究箱で復活させた場合のみ）。

手順（CAP を替えてビルド→10runs）:
```sh
cd /mnt/workdisk/public_share/hakmem

for CAP in 8 16 32 64; do
  echo \"=== CAP=$CAP ===\" | tee \"$OUT/cap_${CAP}.log\"

  make -C hakozuna/hz4 clean all \
    HZ4_DEFS_EXTRA=\"-DHZ4_PAGE_META_SEPARATE=0 -DHZ4_PAGE_DECOMMIT=0 -DHZ4_TCACHE_MAGAZINE=1 -DHZ4_TCACHE_MAG_CAP=${CAP}\" \
    |& tee \"$OUT/build_cap_${CAP}.txt\"

  for i in $(seq 1 10); do
    env -i PATH=\"$PATH\" HOME=\"$HOME\" \
      LD_PRELOAD=\"$PWD/hakozuna/hz4/libhakozuna_hz4.so\" \
      taskset -c 0-15 \
      ./hakozuna/out/bench_random_mixed_mt_remote_malloc \
        16 2000000 400 16 2048 0 65536 \
      | tee \"$OUT/cap_${CAP}_run_${i}.txt\"
  done

  # R=90 sanity 1回
  env -i PATH=\"$PATH\" HOME=\"$HOME\" \
    LD_PRELOAD=\"$PWD/hakozuna/hz4/libhakozuna_hz4.so\" \
    taskset -c 0-15 \
    ./hakozuna/out/bench_random_mixed_mt_remote_malloc \
      16 2000000 400 16 2048 90 65536 \
    | tee \"$OUT/cap_${CAP}_sanity_r90.txt\"
done
```

---

## 3) median 集計（CAP ごと）

```sh
python3 - <<'PY'
import glob, re, statistics
from collections import defaultdict

groups = defaultdict(list)
for p in glob.glob('/tmp/hz4_phase12b_magcap_sweep_*/*cap_*_run_*.txt'):
    m = re.search(r'cap_(\\d+)_run_', p)
    if not m: continue
    cap = int(m.group(1))
    s = open(p).read()
    m2 = re.search(r'ops/s=([0-9.]+)', s)
    if not m2: continue
    groups[cap].append(float(m2.group(1)))

for cap in sorted(groups):
    vals = groups[cap]
    med = statistics.median(vals)
    print(f'CAP={cap} runs={len(vals)} median={med/1e6:.2f}M min={min(vals)/1e6:.2f}M max={max(vals)/1e6:.2f}M')
PY
```

---

## 4) 判定

- GO（継続）: **best CAP が baseline(cap=8) に対して +5% 以上**
- NO-GO（打ち切り）: best CAP でも **+2% 未満**

NO-GO の場合は:
- MagBox は “opt-in 研究箱” 扱いに格下げし、次は **BumpSlabBox（lane隔離）**へ進む。

---

## 結果（2026-01-26）

結論: **NO-GO**

- CAP=8:  268.72M（265.55–276.37M）
- CAP=16: 272.73M（266.90–279.01M）best（+1.49% vs CAP=8）
- CAP=32: 268.46M（261.79–276.98M）
- CAP=64: 269.59M（266.60–275.75M）
