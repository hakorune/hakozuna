# HZ4 Phase 10C: hz3 vs hz4（R=0）perf diff 指示書（箱の責務確定）

目的: SSOT 10runs で確認された **hz3(p5b) 優勢 +30%** の差を、`perf` で “箱単位” に分解して責務を確定する。

箱理論（Box Theory）
- **箱**: `PerfDiffBox`（観測専用）
- **境界1箇所**: `bench_random_mixed_mt_remote_malloc`（R=0）だけ
- **戻せる**: `.so` を `/tmp` に固定して混線しない
- **見える化**: `perf record --call-graph fp` と `perf report --stdio` を保存
- **Fail-Fast**: 計測が崩れたら中止（データシンボルが上位など）

---

## 0) 前提（SSOT固定）

- repo: `/mnt/workdisk/public_share/hakmem`
- 条件: `T=16 iters=2000000 ws=400 size=16..2048 R=0 ring=65536`
- pinning: `taskset -c 0-15`
- `.so`:
  - hz4: `/tmp/ssot_hz4_perf.so`
  - hz3: `/tmp/ssot_hz3_p5b.so`

ログ置き場:
```sh
OUT=/tmp/perf_hz3_vs_hz4_r0_$(date +%Y%m%d_%H%M%S)
mkdir -p "$OUT"
```

---

## 1) 事前チェック（perf が “関数” を見ているか）

期待:
- 上位に `hz*_malloc/free` が出る
- `g_*` のような **データシンボルが上位に出ない**

---

## 2) hz4 perf record（R=0）

```sh
cd /mnt/workdisk/public_share/hakmem
env -i PATH="$PATH" HOME="$HOME" \
  LD_PRELOAD="/tmp/ssot_hz4_perf.so" \
  taskset -c 0-15 \
  perf record --call-graph fp -F 999 -o "$OUT/hz4.data" -- \
    ./hakozuna/out/bench_random_mixed_mt_remote_malloc \
      16 2000000 400 16 2048 0 65536 \
  >/dev/null
perf report -i "$OUT/hz4.data" --stdio --no-children | head -n 120 | tee "$OUT/hz4_top.txt"
```

---

## 3) hz3 perf record（R=0）

```sh
cd /mnt/workdisk/public_share/hakmem
env -i PATH="$PATH" HOME="$HOME" \
  LD_PRELOAD="/tmp/ssot_hz3_p5b.so" \
  taskset -c 0-15 \
  perf record --call-graph fp -F 999 -o "$OUT/hz3.data" -- \
    ./hakozuna/out/bench_random_mixed_mt_remote_malloc \
      16 2000000 400 16 2048 0 65536 \
  >/dev/null
perf report -i "$OUT/hz3.data" --stdio --no-children | head -n 120 | tee "$OUT/hz3_top.txt"
```

---

## 4) 箱への割当（ここが成果物）

`$OUT/hz4_top.txt` と `$OUT/hz3_top.txt` の上位関数を次に分類する:

- **MallocBox**: `hz*_malloc`, `hz*_small_malloc`, `*_size_to_sc`, `*_tcache_pop`, refill 呼び出し
- **FreeBox**: `hz*_free`, `*_small_free_*`, `*_tcache_push`, remote 判定/分岐
- **RefillBox/CollectBox**: `*_refill`, `*_collect*`, `*_drain*`, inbox/carry 関連
- **SegmentBox/OSBox**: `*_seg_*`, `*_os_*`

最低限の提出物:
- `hz3_top.txt` / `hz4_top.txt`
- “hz3 には無い/薄いが hz4 で重い” 関数 3つ
- “hz4 には無い/薄いが hz3 で重い” 関数 3つ（基本は無い想定）

---

## 5) 次の Phase（実装）は 1箱だけ

Phase 10C の後は:
- 「差分の責務が一番大きい箱」だけを選ぶ
- `#ifndef` フラグで A/B
- +5% 未満なら撤退

