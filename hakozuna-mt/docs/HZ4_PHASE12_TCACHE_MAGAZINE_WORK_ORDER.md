# HZ4 Phase 12: MagBox（Magazine TCache）指示書

目的: R=0 の local-only で `tcache_pop/push` の “obj本体の next read/write” を減らし、hz3(p5b) との gap を詰める。

箱理論（Box Theory）
- **箱**: `MagBox`（研究箱）
- **境界1箇所**: `hz4_tcache_pop/push`（TcacheBox の API だけ）
- **戻せる**: **NO-GO のため本線から除去済み**（研究箱でのみ再実装して A/B）
- **見える化**: SSOT 10runs median（R=0）と perf（`hz4_tcache_*`）
- **Fail-Fast**: N/A（本線から除去済み）

---

## 背景

- R=0 SSOT 10runs（2026-01-26）で hz3(p5b) が hz4(perf) に対して **+30.58%** 優勢。
- NO-GO: slice refill / TLS hot-cold split / RouteBox head64 / tcache bulk push。
- まだ “fast path で obj を触らない” 余地がある → magazine 方式を試す。

---

## 実装概要

bin を「intrusive linked-list」だけにせず、TLS-only の小配列（magazine）を前段に追加する。

- **push（free）**
  - magazine に空きがあれば `bin->mag[top++]=obj`（obj本体に next を書かない）
  - 満杯なら overflow list（従来の intrusive list）へ落とす
- **pop（malloc）**
  - magazine にあれば `obj=mag[--top]`（obj本体に触らない）
  - 無ければ overflow list から最大 `CAP` 個を magazine に詰め直し、1個返す（ポインタ追跡を amortize）

---

## A/B フラグ

`hakozuna/hz4/core/hz4_config.h`

※ 現状は `HZ4_TCACHE_MAGAZINE/HZ4_TCACHE_MAG_CAP` の実装が本線に無いので、そのままではビルドできません（研究箱で復活させた場合のみ）。

---

## ビルド

baseline（mag OFF）
```sh
cd /mnt/workdisk/public_share/hakmem
make -C hakozuna/hz4 clean all \
  HZ4_DEFS_EXTRA='-DHZ4_PAGE_META_SEPARATE=0 -DHZ4_PAGE_DECOMMIT=0'
```

mag ON（perf lane）
```sh
cd /mnt/workdisk/public_share/hakmem
make -C hakozuna/hz4 clean all \
  HZ4_DEFS_EXTRA='-DHZ4_PAGE_META_SEPARATE=0 -DHZ4_PAGE_DECOMMIT=0 -DHZ4_TCACHE_MAGAZINE=1'
```

RSS lane（安全性チェック用、任意）
```sh
cd /mnt/workdisk/public_share/hakmem
make -C hakozuna/hz4 clean all \
  HZ4_DEFS_EXTRA='-DHZ4_PAGE_META_SEPARATE=1 -DHZ4_PAGE_DECOMMIT=1 -DHZ4_DECOMMIT_DELAY_QUEUE=1 -DHZ4_TCACHE_PURGE_BEFORE_DECOMMIT=1 -DHZ4_TCACHE_MAGAZINE=1'
```

---

## SSOT 10runs（R=0）

```sh
cd /mnt/workdisk/public_share/hakmem
OUT=/tmp/hz4_phase12_mag_r0_$(date +%Y%m%d_%H%M%S)
mkdir -p "$OUT"

for i in $(seq 1 10); do
  env -i PATH="$PATH" HOME="$HOME" \
    LD_PRELOAD="$PWD/hakozuna/hz4/libhakozuna_hz4.so" \
    taskset -c 0-15 \
    ./hakozuna/out/bench_random_mixed_mt_remote_malloc \
      16 2000000 400 16 2048 0 65536 \
    | tee "$OUT/run_${i}.txt"
done
```

判定:
- GO: R=0 **+5% 以上**（gap が大きいのでラインを高めに）
- NO-GO: **+2% 未満**なら撤退

sanity（R=90 1回）
```sh
env -i PATH="$PATH" HOME="$HOME" \
  LD_PRELOAD="$PWD/hakozuna/hz4/libhakozuna_hz4.so" \
  taskset -c 0-15 \
  ./hakozuna/out/bench_random_mixed_mt_remote_malloc \
    16 2000000 400 16 2048 90 65536 | tee "$OUT/sanity_r90.txt"
```

---

## 結果（2026-01-26）

結論: **NO-GO**（改善が +2% 未満）

- baseline（mag OFF）: 260.45M ops/s
- mag ON（CAP=8）: 265.46M ops/s（+1.92%）
- R=90 sanity: 106.89M ops/s ✓

補足: CAP sweep（Phase 12B）では CAP=16 が best だが +1.49% に留まり NO-GO。

アーカイブ:
- `hakozuna/hz4/archive/research/hz4_phase12_magazine_tcache/README.md`
