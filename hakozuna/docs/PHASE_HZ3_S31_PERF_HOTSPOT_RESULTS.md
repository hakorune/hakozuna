# S31: perf hotspot 分析結果

## Status: COMPLETED

Date: 2026-01-02

## 背景

S30 で「tail に特定の主犯はない」と判明。
S31 では perf record で dist=app の hotspot を直接観測し、gap の真因を特定した。

## 計測条件

```bash
perf record -g --call-graph dwarf,16384 -- \
  env LD_PRELOAD=<SO> ./hakozuna/out/bench_random_mixed_malloc_dist \
  20000000 400 16 32768 0x12345678 app
```

## 結果: 関数レベル

### hz3 (ops/s=50.8M)

| Function | Overhead |
|----------|----------|
| main | 30.20% |
| sample_from_buckets | 19.17% |
| __strcmp_avx2 | 16.27% |
| **hz3_free** | **10.55%** |
| **hz3_malloc** | **7.60%** |
| free (wrapper) | 2.58% |
| malloc (wrapper) | 2.36% |
| **Allocator 合計** | **~23%** |

### tcmalloc (ops/s=51.5M)

| Function | Overhead |
|----------|----------|
| main | 32.86% |
| sample_from_buckets | 21.31% |
| __strcmp_avx2 | 19.10% |
| **tc_malloc** | **8.73%** |
| **operator delete[]** | **6.80%** |
| **Allocator 合計** | **~15.5%** |

**差分: ~7.5%** → これが dist=app の -7.69% gap の正体

## 結果: 命令レベル (perf annotate)

### hz3_malloc の hotspot

```asm
# TLS init check (合計 ~20%)
12.26%:  je     2c78 <hz3_malloc+0x128>      ; init 未完了なら slow path へ
 7.64%:  cmpb   $0x0,%fs:0x4c86(%rbx)        ; TLS init フラグ読み

# size class 計算
 6.31%:  sub    $0x1,%r12d
 5.66%:  add    $0x8,%rdx
```

**問題**: 毎回の malloc で TLS init check が走っている

### hz3_free の hotspot

```asm
# dst (owner thread) 比較
 7.81%:  cmp    %al,%fs:0x4c85(%r12)         ; PTAG32 の dst と current dst の比較

# bin range 判定
 5.31%:  cmp    $0x7f,%ebx

# freelist push
 5.26%:  addw   $0x1,0x8(%rax)               ; count++
 3.27%:  mov    %rbp,(%rax)                  ; ptr store
```

**問題**: dst 比較が毎回 7.81% を消費

### tcmalloc tc_malloc の hotspot

```asm
# TLS アクセス（init check なし）
 6.64%:  mov    0x12b55(%rip),%rax
 3.12%:  mov    %fs:(%rax),%rdi

# size class 計算
 7.96%:  lea    0x7(%r8),%eax
 3.57%:  shr    $0x3,%eax

# freelist pop
 4.34%:  mov    (%rax),%r8
 2.38%:  mov    (%r8),%rdx
```

**特徴**: TLS は常に初期化済み前提、分岐が少ない

## 分析: 主犯

### 1. TLS init check (hz3_malloc の ~20%)

```c
// 現在のコード (毎回チェック)
if (!t_hz3_tcache.inited) {
    hz3_tcache_ensure_init_slow();
}
```

tcmalloc は:
- TLS を lazy init ではなく eager init（または pthread_key 方式）
- hot path では init 確認を完全にスキップ

**インパクト**: malloc overhead の約 1/4 を占める

### 2. dst 比較 (hz3_free の ~8%)

```c
// 現在のコード (毎回比較)
uint8_t tag_dst = tag >> 24;
if (tag_dst == current_dst) {
    // local free
} else {
    // remote free
}
```

**問題**:
- dist=app では 80% が tiny (同じ dst)
- にもかかわらず毎回 TLS 読み + 比較

## 次の最適化候補 (S32)

| 優先度 | 対象 | 見込み効果 |
|--------|------|-----------|
| **1** | TLS init check 削除 | malloc overhead 20%→0% |
| **2** | dst 比較の分岐予測改善 | free overhead 8%→2% |
| 3 | bin計算のLUT化 | malloc/free 各 5%→2% |

### 推奨: TLS init check 削除

現在の lazy init を以下のいずれかに変更:

1. **pthread_key + destructor**: スレッド生成時に自動 init
2. **early init**: main 前に全スレッド init 済みにする
3. **asm 最適化**: init 済みフラグを専用レジスタに保持（非現実的）

最も現実的なのは **pthread_key 方式**:
```c
static pthread_once_t key_init = PTHREAD_ONCE_INIT;
static pthread_key_t  key;

void hz3_tcache_init_key(void) {
    pthread_key_create(&key, hz3_tcache_destroy);
}

void* hz3_malloc(size_t size) {
    hz3_tcache_t* tc = pthread_getspecific(key);
    if (__builtin_expect(!tc, 0)) {
        tc = hz3_tcache_create();
        pthread_setspecific(key, tc);
```

補足（work order）:
- S32 の指示書: `hakozuna/hz3/docs/PHASE_HZ3_S32_TLS_INIT_AND_DST_ROW_OFF_WORK_ORDER.md`
    }
    // ... fast path (no init check)
}
```

ただし pthread_getspecific も TLS アクセスなので、完全にゼロにはならない。
tcmalloc は TLS を「常に存在」前提で設計しており、null check を回避している。

## ログ

- `/tmp/s31_perf/hz3_distapp.data` - hz3 perf record
- `/tmp/s31_perf/tcmalloc_distapp.data` - tcmalloc perf record

---

## 2026-01-15 refresh（dist=app / trunk）

### 計測条件

```bash
perf record -g --call-graph dwarf,16384 -- \
  env -u LD_PRELOAD LD_PRELOAD=./libhakozuna_hz3_scale.so \
  ./hakozuna/out/bench_random_mixed_malloc_dist \
  20000000 400 16 32768 0x12345678 app

perf record -g --call-graph dwarf,16384 -- \
  env -u LD_PRELOAD LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libtcmalloc.so \
  ./hakozuna/out/bench_random_mixed_malloc_dist \
  20000000 400 16 32768 0x12345678 app
```

### ops/s

- hz3: 58,078,706.30 ops/s
- tcmalloc: 58,334,433.12 ops/s

### perf.data

- `/tmp/perf_distapp_hz3.data`
- `/tmp/perf_distapp_tcmalloc.data`

### Top (perf report --stdio, head -n 80)

hz3:
- main (self 34.28%, children 68.45%)
- sample_from_buckets.constprop.0 (12.15%)
- __strcmp_avx2 (8.64%)
- hz3_free (1.94% child)
- hz3_malloc (1.08% child)

tcmalloc:
- main (self 33.97%, children 71.51%)
- sample_from_buckets.constprop.0 (17.51% child, 22.12% self elsewhere)
- tc_malloc (7.45% child)
- operator delete[] (4.93% child)
- __strcmp_avx2 (17.17% self elsewhere)

注記:
- kptr_restrict の警告あり（kernel symbol 未解決）。
- perf の Top は bench main 側の寄与が大きく、allocator の差分は子要素の低比率に埋もれる。

---

## 2026-01-16 refresh（dist=app / S125 fast dispatch）

### 計測条件

```bash
perf record -g --call-graph dwarf,16384 -- \
  env -u LD_PRELOAD LD_PRELOAD=./libhakozuna_hz3_scale.so \
  ./hakozuna/out/bench_random_mixed_malloc_dist \
  20000000 400 16 32768 0x12345678 app

perf record -g --call-graph dwarf,16384 -- \
  env -u LD_PRELOAD LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libtcmalloc.so \
  ./hakozuna/out/bench_random_mixed_malloc_dist \
  20000000 400 16 32768 0x12345678 app
```

### ops/s

- hz3: 89,813,495.83 ops/s
- tcmalloc: 89,839,277.73 ops/s

### perf.data

- `/tmp/perf_distapp_hz3.data`
- `/tmp/perf_distapp_tcmalloc.data`

### Top (perf report --stdio, head -n 80)

hz3:
- main (self 57.36%, children 66.27%)
- hz3_free (1.40% child)
- hz3_malloc (1.84% child)

tcmalloc:
- main (self 61.65%, children 78.58%)
- tc_malloc (15.04% self)
- operator delete[] (14.57% self)

注記:
- kptr_restrict の警告あり（kernel symbol 未解決）。
- S125 で sample_from_buckets/strcmp が Top から外れたが、bench main が依然支配的。

---

## 2026-01-17 refresh（r90_pf2 / S97-2 bucket=2 / S96 push stats）

### 計測条件

```bash
make -C hakozuna/hz3 clean all_ldpreload_scale \
  HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_S44_PREFETCH_DIST=2 -DHZ3_S97_REMOTE_STASH_BUCKET=2 \
  -DHZ3_S97_REMOTE_STASH_SKIP_TAIL_NULL=1 -DHZ3_S96_OWNER_STASH_PUSH_STATS=1'

env -u LD_PRELOAD LD_PRELOAD=./libhakozuna_hz3_scale.so \
  ./hakozuna/out/bench_random_mixed_mt_remote_malloc \
  32 2000000 400 16 2048 90 65536
```

### [HZ3_S96_PUSH]（SSOT）

- calls=3335990 objs=7105654 n1=0 nmax=9 cas_retry_total=89 cas_retry_max=1 cas_retry_gt0=89 sc_lt32_objs=1732067 sc_ge32_objs=5373587 hist=1:0 2:2958858 3-4:369987 5-8:7144 9-16:1 17-32:0 33+:0
