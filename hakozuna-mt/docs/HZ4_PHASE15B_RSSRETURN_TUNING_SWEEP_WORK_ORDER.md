# HZ4 Phase 15B: RSSReturnBox チューニング sweep 指示書（SSOT）

目的: Phase 15 で GO になった RSSReturnBox のパラメータを sweep し、
RSS の下がり幅と ops/s のバランスが最も良い点を SSOT で確定する。

箱理論（Box Theory）
- **箱**: `RSSReturnBox`
- **境界1箇所**: `hz4_collect()` の epoch でのみ処理（hot path は触らない）
- **戻せる**: compile-time フラグ（A/B 即切替）
- **見える化**: ru_maxrss（/usr/bin/time -v）+ ops/s（ベンチ出力）
- **Fail-Fast**: crash/abort はそのまま（隠さない）

---

## 0) SSOT 条件（固定）

- bench: `hakozuna/out/bench_random_mixed_mt_remote_malloc`
- 条件:
  - `T=16 iters=2000000 ws=400 size=16..2048 R=90 ring=65536`
  - 参考で `R=0` も 1点だけ確認（速度回帰の検知）
- 計測:
  - R=90: `ru_maxrss` は **3 runs median**
  - R=0 : ops/s は **10 runs median**（ただし sweep では 1点だけでOK）

---

## 1) Sweep するパラメータ

対象（RSS lane でのみ有効）:
- `HZ4_RSSRETURN_THRESH_PAGES`（しきい値）
- `HZ4_RSSRETURN_BATCH_PAGES`（1 epoch あたりの返却上限）
- `HZ4_RSSRETURN_SAFETY_PERIOD`（しきい値未達時の保険周期）

固定:
- `HZ4_PAGE_META_SEPARATE=1`
- `HZ4_PAGE_DECOMMIT=1`
- `HZ4_DECOMMIT_DELAY_QUEUE=1`
- `HZ4_TCACHE_PURGE_BEFORE_DECOMMIT=1`

推奨グリッド（まずは小さく）:
- THRESH: `16, 32, 64`
- BATCH : `4, 8, 16`
- PERIOD: `128, 256`（2点だけ）

合計 3*3*2 = 18 点（R=90 だけ 3 runs なので現実的）。

---

## 2) 実行（R=90: 3 runs median）

ログ置き場:
```sh
cd /mnt/workdisk/public_share/hakmem
OUT=/tmp/hz4_phase15b_rssreturn_sweep_$(date +%Y%m%d_%H%M%S)
mkdir -p "$OUT"
```

コマンド雛形:
```sh
build_one () {
  local THRESH=$1 BATCH=$2 PERIOD=$3
  make -C hakozuna/hz4 clean all \
    HZ4_DEFS_EXTRA=\"-DHZ4_PAGE_META_SEPARATE=1 -DHZ4_PAGE_DECOMMIT=1 -DHZ4_DECOMMIT_DELAY_QUEUE=1 -DHZ4_TCACHE_PURGE_BEFORE_DECOMMIT=1 \
      -DHZ4_RSSRETURN=1 -DHZ4_RSSRETURN_THRESH_PAGES=${THRESH} -DHZ4_RSSRETURN_BATCH_PAGES=${BATCH} -DHZ4_RSSRETURN_SAFETY_PERIOD=${PERIOD}\"
}

run_r90_once () {
  /usr/bin/time -v env -i PATH=\"$PATH\" HOME=\"$HOME\" \
    LD_PRELOAD=\"$PWD/hakozuna/hz4/libhakozuna_hz4.so\" \
    ./hakozuna/out/bench_random_mixed_mt_remote_malloc 16 2000000 400 16 2048 90 65536 \
    1> /dev/null
}
```

Sweep 本体:
```sh
for THRESH in 16 32 64; do
  for BATCH in 4 8 16; do
    for PERIOD in 128 256; do
      KEY=\"t${THRESH}_b${BATCH}_p${PERIOD}\"
      echo \"=== ${KEY} ===\" | tee -a \"$OUT/summary.txt\"
      build_one \"$THRESH\" \"$BATCH\" \"$PERIOD\" |& tee \"$OUT/build_${KEY}.txt\"

      for i in 1 2 3; do
        run_r90_once 2> \"$OUT/time_${KEY}_${i}.txt\"
      done
    done
  done
done
```

---

## 3) 集計（ru_maxrss median）

```sh
python3 - <<'PY'
import glob, re, statistics, os
base = max(glob.glob('/tmp/hz4_phase15b_rssreturn_sweep_*'), key=os.path.getmtime)
times = glob.glob(base + '/time_t*_b*_p*_*.txt')

groups = {}
for p in times:
    m = re.search(r'time_(t\\d+_b\\d+_p\\d+)_(\\d+)\\.txt$', p)
    if not m: continue
    key = m.group(1)
    txt = open(p).read()
    m2 = re.search(r'Maximum resident set size \\(kbytes\\):\\s*(\\d+)', txt)
    if not m2: continue
    groups.setdefault(key, []).append(int(m2.group(1)))

rows=[]
for key, vals in groups.items():
    if len(vals) != 3: continue
    med = int(statistics.median(vals))
    rows.append((med, key, min(vals), max(vals)))

rows.sort()
print('best (lowest rss) top10:')
for med, key, mn, mx in rows[:10]:
    print(f'{key}: rss_med_kb={med} range={mn}-{mx}')
PY
```

---

## 4) 速度回帰チェック（R=0: 10 runs median, best 2点だけ）

ru_maxrss の top2 だけに対して、R=0 を 10 runs で確認:
- GO 条件: Phase15 の `-1.30%` を大きく超えない（例: -5% 以内）

---

## 5) 反映

決まった “best point” を:
- `docs/benchmarks/` に結果メモ（表）
- `CURRENT_TASK.md` に採用値を明記
- 必要なら `hz4_config.h` のデフォルト（RSS lane）に反映

