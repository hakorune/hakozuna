# HZ4 Phase 10B: hz3 vs hz4（R=0）SSOT 10runs 指示書

目的: 「hz4 が hz3(p5b) より速い/遅い」を **同一条件・10runs median** で確定する。

箱理論（Box Theory）
- **箱**: `SSOTCompareBox`（観測専用）
- **境界1箇所**: `bench_random_mixed_mt_remote_malloc`（R=0）だけ
- **戻せる**: `.so` を `/tmp` に固定保存して混線を防ぐ
- **見える化**: 10runs の raw を保存し、median を明示
- **Fail-Fast**: 失敗時はそのまま止める（握りつぶさない）

---

## 0) SSOT 条件（固定）

- repo: `/mnt/workdisk/public_share/hakmem`
- bench: `hakozuna/out/bench_random_mixed_mt_remote_malloc`
- 条件: `T=16 iters=2000000 ws=400 size=16..2048 R=0 ring=65536`
- runs: **10**（median）
- pinning: `taskset -c 0-15`

ログ置き場:
```sh
OUT=/tmp/ssot_hz3_vs_hz4_r0_$(date +%Y%m%d_%H%M%S)
mkdir -p "$OUT"
```

---

## 1) hz4（perf lane）をビルドして .so 固定

```sh
cd /mnt/workdisk/public_share/hakmem
make -C hakozuna/hz4 clean all \
  HZ4_DEFS_EXTRA='-DHZ4_PAGE_META_SEPARATE=0 -DHZ4_PAGE_DECOMMIT=0'
cp -f hakozuna/hz4/libhakozuna_hz4.so /tmp/ssot_hz4_perf.so
sha1sum /tmp/ssot_hz4_perf.so | tee "$OUT/hz4_perf.sha1"
```

---

## 2) hz3（p5b 推奨 lane）をビルドして .so 固定

※ hz3 の “p5b” がどのターゲットかは環境で揺れるので、まず Makefile のターゲット名を探してから叩く。

ターゲット探索（候補を列挙）:
```sh
cd /mnt/workdisk/public_share/hakmem
make -C hakozuna/hz3 help 2>/dev/null | tee "$OUT/hz3_make_help.txt" || true
```

よくある候補（存在する方を使用）:
- `all_ldpreload_scale_s64_p5b`
- `all_ldpreload_scale_p5b`
- `all_ldpreload_p5b`

ビルド（例: scale lane の p5b）:
```sh
make -C hakozuna/hz3 clean all_ldpreload_scale_s64_p5b
```

生成された .so を固定（symlink の場合は実体をコピー）:
```sh
readlink -f hakozuna/hz3/libhakozuna_hz3_ldpreload.so | tee "$OUT/hz3_p5b_realpath.txt"
cp -f "$(cat "$OUT/hz3_p5b_realpath.txt")" /tmp/ssot_hz3_p5b.so
sha1sum /tmp/ssot_hz3_p5b.so | tee "$OUT/hz3_p5b.sha1"
```

---

## 3) 10runs（hz4）

```sh
cd /mnt/workdisk/public_share/hakmem
for i in $(seq 1 10); do
  env -i PATH="$PATH" HOME="$HOME" \
    LD_PRELOAD="/tmp/ssot_hz4_perf.so" \
    taskset -c 0-15 \
    ./hakozuna/out/bench_random_mixed_mt_remote_malloc \
      16 2000000 400 16 2048 0 65536 \
    | tee "$OUT/hz4_run_${i}.txt"
done
```

median 集計:
```sh
python3 - <<'PY'
import glob, re, statistics
paths=sorted(glob.glob("/tmp/ssot_hz3_vs_hz4_r0_*/hz4_run_*.txt"))
vals=[]
for p in paths:
  s=open(p).read()
  m=re.search(r"ops/s=([0-9.]+)", s)
  vals.append(float(m.group(1)))
print("hz4 runs:", len(vals))
print("hz4 median ops/s:", statistics.median(vals))
print("hz4 min/max:", min(vals), max(vals))
PY
```

---

## 4) 10runs（hz3 p5b）

```sh
cd /mnt/workdisk/public_share/hakmem
for i in $(seq 1 10); do
  env -i PATH="$PATH" HOME="$HOME" \
    LD_PRELOAD="/tmp/ssot_hz3_p5b.so" \
    taskset -c 0-15 \
    ./hakozuna/out/bench_random_mixed_mt_remote_malloc \
      16 2000000 400 16 2048 0 65536 \
    | tee "$OUT/hz3_run_${i}.txt"
done
```

median 集計:
```sh
python3 - <<'PY'
import glob, re, statistics
paths=sorted(glob.glob("/tmp/ssot_hz3_vs_hz4_r0_*/hz3_run_*.txt"))
vals=[]
for p in paths:
  s=open(p).read()
  m=re.search(r"ops/s=([0-9.]+)", s)
  vals.append(float(m.group(1)))
print("hz3 runs:", len(vals))
print("hz3 median ops/s:", statistics.median(vals))
print("hz3 min/max:", min(vals), max(vals))
PY
```

---

## 5) 最終判定（SSOT）

判定ルール（例）:
- 差分が **±2% 未満** → “同等（ノイズ域）”
- hz4 が **+2% 以上** → “hz4 優勢”
- hz3 が **+2% 以上** → “hz3 優勢”

結果を `CURRENT_TASK.md` に 1行で追記（`sha1` と `OUT` のパスも必須）。

