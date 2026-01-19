# S50: Large Cache Size-Class 設計 相談パック

目的:
- mstressN / glibc-simple / malloc-large で **hz3 の large 経路が負ける**原因を解消する。
- 小さい経路（<=32KB）は触らず、`hz3_large` 内で完結する Box を設計する。

## 1) 現状の観測（S49）

mstressN 計測:
- hz3: large で **個別 mmap**（サイズ多様）
- tcmalloc: **内部プール再利用**で syscall が少ない

差分:
- mmap 回数: hz3 532 / tcmalloc 318（1.7x）
- usec/call: hz3 619 / tcmalloc 12（51x）
- total mmap time: hz3 0.329s / tcmalloc 0.004s（82x）

原因:
- hz3 の large cache が **first-fit 単一リスト**で、サイズ多様だとヒットしにくい。

## 2) 目標

- mstressN: +30〜50% 改善（mmap time を 1/5〜1/10 に低減）
- glibc-simple / malloc-large も改善
- small/medium/mixed の固定費は **増やさない**

## 3) 制約（箱理論）

- compile-time A/B のみ（env ノブ禁止）
- hot path（<=32KB）の修正禁止
- `hz3_large` 内で完結する箱として実装

## 4) 既存 large 実装（要点）

- `hz3_large_alloc` は `mmap` → header 付与 → hash insert
- cache は **単一 free list**（first-fit）
- グローバル lock 1本
- cap: `HZ3_LARGE_CACHE_MAX_BYTES` / `HZ3_LARGE_CACHE_MAX_NODES`

## 5) 提案（S50: size class 化）

- large を **size class**（4KB/8KB/16KB/.../1MB + >1MB）に分ける
- class 毎に **LIFO** で O(1) push/pop
- cap は **全体 + class** の 2段構え
- alloc は **同一 class のみ**（なければ mmap）

## 6) 質問（設計の分岐）

1. **class 境界**の推奨:
   - 4KB〜1MB の 2冪で固定で良い？
   - >1MB の扱いは 1クラスで良い？それとも 2〜3階層？

2. **cap の推奨値**:
   - 全体 `MAX_BYTES`/`MAX_NODES` の初期値は？
   - class ごとの `MAX_BYTES/NODES` は固定比率で良い？

3. **サイズの丸め**:
   - `size` を class に **切り上げ**て再利用（内的断片化許容）で良い？
   - それとも “完全一致のみ” が良い？

4. **再利用ポリシー**:
   - 同一 class のみ利用（単純）で良い？
   - 「1クラス上は許容」など許すとヒット率は上がるが断片化は増える。どちらが良い？

5. **madvise 方針**:
   - cache 保持時に `madvise(DONTNEED)` を入れるべき？
   - それとも mmap 再利用重視で触らないべき？

6. **lock 粒度**:
   - global lock 1本のままで良い？
   - class 毎 lock に分けるべき？

7. **aligned_alloc 絡み**:
   - large aligned alloc の再利用も class cache に乗せるべき？
   - 別枠の cache を作るべき？

## 7) 判断材料（ベンチ抜粋）

mimalloc-bench (r=1, n=1):
- mstressN: hz3 2.04s / tc 1.32s / mi 1.35s
- rptestN: hz3 4.52s / tc 2.87s / mi 2.54s
- glibc-simple: hz3 2.91s / tc 1.71s / mi 2.36s
- xmalloc-testN: hz3 4.16s / tc 29.2s / mi 0.44s

SSOT (ST):
- 16–2048: hz3 112M / tc 91M
- 4096–32768: hz3 112M / tc 111M
- 16–32768: hz3 106M / tc 110M

## 8) 期待する回答

- class 設計の最終案（境界 + cap + policy）
- A/B で切り戻せる最小フラグ設計
- GO/NO-GO の推奨判定基準
