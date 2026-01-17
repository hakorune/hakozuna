# PHASE_HZ3_S14: hz3_large “Huge/Large Cache” 強化（Work Order）

目的:
- `mimalloc-bench/bench/malloc-large`（5–25MiB, live<=20）で hz3 が tcmalloc に大差で負けている原因を潰す。
- small/medium（16–32768）には **一切固定費を増やさず**、`hz3_large`（>32KB）だけを強化する。

背景（いま起きていること）:
- cache-thrash / cache-scratch は hz3 ≈/＞ tcmalloc で、hot/dispatch は詰まっている。
- `malloc-large` は hz3 が極端に遅い。
  - `std::make_unique<char[]>(n)` は value-init で **毎回ゼロ埋め**され、全ページに触れる。
  - hz3 は `mmap → munmap` を繰り返しているため、**新規ページ fault**が毎回起きやすい。
  - tcmalloc はこのサイズ帯で **再利用（page cache）**が効きやすく、fault を避けやすい。

S49 診断（mstressN / large 経路）:
- hz3 は large 確保ごとに **個別 mmap**（サイズ多様）
- tcmalloc は **内部プール**で再利用（syscall が少ない）
- 計測:
  - mmap 回数: hz3 532 / tcmalloc 318（1.7x）
  - usec/call: hz3 619 / tcmalloc 12（51x）
  - total mmap time: hz3 0.329s / tcmalloc 0.004s（82x）
- 原因: **first-fit の large cache がサイズ多様で機能しない**

参照（導線）:
- SSOT-all（mimalloc-bench subset を含む）: `hakozuna/scripts/run_bench_ssot_all.sh`
- mimalloc-bench subset: `hakozuna/scripts/run_bench_mimalloc_bench_subset_compare.sh`
- 現行実装: `hakozuna/hz3/src/hz3_large.c`

制約（箱理論/運用）:
- allocator挙動の切替は compile-time `-D` のみ（envノブ禁止）
- hot path（16–32768 の TLS push/pop）は **変更禁止**
- NO-GO は revert + `hakozuna/hz3/archive/research/` に保存（README + ログ）

---

## 0) 今回の方針（S14の“勝てる形”）

`hz3_large` は “mmap correctness-first” から、以下へ進化させる（S49 反映）:

- **LargeCacheBox を size class 化**
  - 4KB/8KB/16KB/.../1MB でクラス分けし、**LIFO で O(1) push/pop**
  - free: `munmap` せず **同一 class の cache**へ戻す（上限超えのみ munmap）
  - alloc: **同一 class からのみ取得**（なければ mmap）
  - これにより **mmap/munmap と major fault を減らす**
- **上限の引き上げ**（size class ごとの cap）
  - `MAX_BYTES` / `MAX_NODES` を全体と class の 2段で制御

重要:
- small/medium の経路は触らない（`hz3_hot.c` の size<=32768 を一切変更しない）
- `hz3_large` の内部だけで完結する箱として実装する

---

## 1) 実装スコープ（触ってよい/ダメ）

### 触ってよい（S14）
- `hakozuna/hz3/src/hz3_large.c`（主戦場）
- `hakozuna/hz3/include/hz3_large.h`（必要なら拡張）
- `hakozuna/hz3/include/hz3_config.h`（compile-time flags 追加）
- docs/bench のみ（結果記録）

### 触ってはいけない（S14）
- `hz3_small` / `hz3_small_v2` / `hz3_medium` の hot path 固定費
- PTAG/segmap の dispatch（small/medium/mixed を落とすリスクが高い）

---

## 2) 追加する compile-time flags（A/B）

（名前はこのまま推奨。変更するなら docs も揃える）

- `HZ3_LARGE_CACHE_ENABLE=0/1`（default 1）
  - 0: 現状どおり `mmap/munmap`
  - 1: cache を使う

- `HZ3_LARGE_CACHE_MAX_BYTES`（default 512MiB）
  - cache に保持する上限バイト数（0 は “保持しない” と同義）
  - `malloc-large` を狙うなら初期値例: `512u<<20`（512MiB）〜 `1024u<<20`

- `HZ3_LARGE_CACHE_MAX_NODES`（default 64）
  - cache ノード数上限（0 は “無制限” だが推奨しない）
  - 初期値例: 64

運用:
- “趣味最強”でまず勝ち筋を見たいなら、A/B は `HZ3_LARGE_CACHE_MAX_BYTES` を大きめに置いて刺さるか確認。
- 実アプリ向けの RSS 制御は、その後に tighten する。

---

## 3) データ構造（hz3_large 内部だけ）

### 3-1) 既存 `Hz3LargeHdr` を拡張（再利用状態）

- `uint8_t in_use`（1=allocated, 0=cached/free）
- `Hz3LargeHdr* next_free`（cache list 用）

注意:
- 既存の `g_hz3_large_buckets`（ptr→hdr 安全判定）の仕組みは維持して良い
  - LD_PRELOAD 環境では “非hz3 pointer” を触って落ちるのを避ける必要があるため
  - `malloc-large` のボトルネックは mmap/fault で、hash+mutex 自体ではない

### 3-2) Free cache（単純でOK）

- `Hz3LargeHdr* g_hz3_large_free_head`（単一リスト）
- `size_t g_hz3_large_cached_bytes`
- `size_t g_hz3_large_cached_nodes`
- すべて `g_hz3_large_lock` 下で操作（正しさ優先）

アルゴリズム:
- alloc:
  - `need = align(sizeof(hdr)) + size`
  - free list を線形走査し、`hdr->map_size >= need` の最初を採用
  - 見つからなければ `mmap`
- free:
  - `hdr = hz3_large_take(ptr)` で “自分の”ヘッダを取る（安全）
  - `in_use==0` の場合は double-free（debug fail-fast / release no-op）
  - cache 上限超なら `munmap`、超えなければ free list に push

線形走査が許容な理由:
- `malloc-large` は live <= 20 で、list 長も小さい（20〜数十）
- syscall/fault の削減が圧倒的に効く

---

## 4) 正しさ（fail-fast の条件）

debug only（`#if !defined(NDEBUG)` 等）:
- `hdr->magic == HZ3_LARGE_MAGIC`
- `hdr->user_ptr == ptr`
- `hdr->map_base == hdr`（mmap 直後の契約）
- `hdr->map_size >= hz3_large_user_offset() + hdr->req_size`
- double-free 検出: `in_use==0` で abort

release:
- false negative OK（見つからなければ next_free へ）
- false positive NG（勝手に munmap しない）

---

## 5) ベンチ/GO/NO-GO

### 実行

- SSOT-all で `malloc-large` を含めて実行（mimalloc-bench subset をON）:
```bash
DO_MIMALLOC_BENCH_SUBSET=1 \
  ./hakozuna/scripts/run_bench_ssot_all.sh
```

（単体）:
```bash
RUNS=10 \
  ./hakozuna/scripts/run_bench_mimalloc_bench_subset_compare.sh
```

### GO/NO-GO（目安）

- GO:
  - `malloc-large` の hz3 が **tcmalloc に対して -50% 未満**まで改善（まずは大差解消）
  - small/medium/mixed は **±0% 以内**（回帰なし）
- NO-GO:
  - small/medium/mixed に回帰が出る（S14の目的違反）
  - `malloc-large` が改善しない（cacheが機能していない）

---

## 6) S50 結果（GO）

S50 実装:
- large cache を size class 化（8分割/倍）
- class size にページ丸めし、alloc/free で同一 class を使う
- safety: magic / map_size の検証を追加

結果（mstressN）:

| Allocator | Time | vs Baseline | mmap calls |
|-----------|------|-------------|------------|
| hz3 NO-S50 | 1.62s | - | 146 |
| hz3 S50 | 1.06s | +35% | 39 |
| glibc | 0.97s | +40% | - |
| mimalloc | 0.98s | +39% | - |
| tcmalloc | 0.90s | +44% | - |

判定:
- mstressN 改善: **+35%（GO）**
- mmap 回数: **3.7x 減（GO）**
- 安定性: 10/10 pass

備考:
- page-align 後に class を再計算し、alloc/free の class 不一致バグを修正
- `HZ3_S50_LARGE_SCACHE=1` をデフォルト化（scale lane は明示 ON のまま）

---

## 6-1) S50-1 cap tuning（A/B）

ベンチ（r=1, n=1, hz3のみ）:

| 設定 | mstressN | glibc-simple | malloc-large | RSS |
|------|---------:|-------------:|-------------:|----:|
| baseline | 1.42s | 2.82s | 9.46s | 913MB |
| MAX_BYTES=2GB | 1.40s | 2.84s | 3.22s | 2.4GB |
| MAX_NODES=8192 | 1.42s | 2.87s | 9.49s | 913MB |
| EVICT=1 | 1.42s | 2.88s | 10.21s | 911MB |

所見:
- `MAX_BYTES=2GB` は **malloc-large を大幅改善**するが RSS が大きく増える。
- `MAX_NODES=8192` / `EVICT=1` は改善なし（むしろ悪化）。
- mstressN / glibc-simple には cap 変更の影響がほぼ無い。

暫定判断:
- 既定は維持（512MB / nodes=64 / evict=0）。
- `malloc-large` 専用の性能確認には `MAX_BYTES=2GB` を A/B で使う。

---

## 7) NO-GO の保存（必須）

NO-GO になった場合:
1. revert
2. `hakozuna/hz3/archive/research/s14_large_cache/README.md` を作成
3. baseline/experiment の SSOT ログパスを固定（`/tmp/...`）
4. `hakozuna/hz3/docs/NO_GO_SUMMARY.md` にリンク追加
