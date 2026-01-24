# HZ4 Phase 6: Standalone Allocator (Full malloc/free)

目的:
- hz4 を **完全な allocator** として動作させる
- LD_PRELOAD で **通常の malloc/free** として計測できる状態にする
- hot path を汚さず、Boundary(Flush/Collect) の箱を保つ

前提:
- Phase 4 (carry/pageq) 完了
- Phase 5 比較で T=16 逆転確認済み

---

## 0) 方針（Box Theory）

- 箱は分離: **OS Box / Segment Box / Page Box / TCache Box / Remote Box**
- 境界は固定:
  - **RemoteFlushBox**: `hz4_remote_flush()`
  - **CollectBox**: `hz4_collect()`
- hot path で環境変数は見ない（compile-time）
- 失敗時は **Fail-Fast**

---

## 1) 最小構成（Phase 6-0）

まずは **small-only allocator** として動く最小構成を作る。

範囲:
- サイズ 16–2048B を hz4 で処理
- それ以外は **即 abort**（Fail-Fast）

目的:
1. 正しい `malloc/free` 動作
2. MTでクラッシュしない
3. benchmark を回せる

---

## 2) 追加する箱（ファイル）

### 2.1 OS Box（segment acquire/release）

ファイル: `hakozuna/hz4/src/hz4_os.c`

API:
```
void* hz4_os_seg_acquire(void);
void  hz4_os_seg_release(void* seg_base);
```

要件:
- `mmap(MAP_PRIVATE|MAP_ANONYMOUS)` で 1MB segment を確保
- `HZ4_SEG_SIZE` アライン保証
- 失敗時は `abort()`（Fail-Fast）

### 2.2 Segment Box

ファイル: `hakozuna/hz4/src/hz4_segment.c`

API:
```
hz4_seg_t* hz4_seg_create(uint8_t owner);
void       hz4_seg_init_pages(hz4_seg_t* seg, uint8_t owner);
```

要件:
- seg header / page headers の初期化
- page0 予約（page_idx=0 は使用禁止）
- remote_head 初期化 / queued=0

### 2.3 TCache Box

ファイル: `hakozuna/hz4/src/hz4_tcache.c`

API:
```
void* hz4_malloc(size_t size);
void  hz4_free(void* ptr);
```

要件:
- TLS cache per sizeclass
- cache miss → `hz4_collect` or `hz4_refill` へ
- cache overflow → remote free or local free へ

### 2.4 SizeClass Box

ファイル: `hakozuna/hz4/include/hz4_sizeclass.h`

要件:
- 16B alignment
- size→sc, sc→size

---

## 3) TLS と Owner の初期化

ファイル: `hakozuna/hz4/src/hz4_tls_init.c`

要件:
- `__thread hz4_tls_t tls;`
- thread first touch で init
- owner id = `tid & HZ4_SHARD_MASK`

---

## 4) malloc/free エクスポート

ファイル: `hakozuna/hz4/src/hz4_shim.c`

要件:
- `void* malloc(size_t)` / `void free(void*)` を export
- **hz4 の範囲外サイズは abort**
- `malloc(0)` は最小サイズへ丸め

---

## 5) ビルド / ベンチ

### 5.1 ビルド

```
make -C hakozuna/hz4 clean all
```

出力:
- `libhakozuna_hz4.so`

### 5.2 ベンチ

```
LD_PRELOAD=./libhakozuna_hz4.so \
  ./hakozuna/out/bench_random_mixed_mt_remote_malloc 16 2000000 400 16 2048 90 65536
```

---

## 6) Phase 6-1 (拡張)

- large alloc を mmap に対応（>2KB）
- free で large 判定（RegionId or header）
- TLS cache の tuning

---

## 7) チェックリスト

- [ ] T=1 の malloc/free が正しく動く
- [ ] T>1 で segfault/loop なし
- [ ] bench が完走する
- [ ] boundary 2箇所でのみ remote を扱う

