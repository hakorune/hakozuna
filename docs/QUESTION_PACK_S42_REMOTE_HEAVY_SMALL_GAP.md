# hz3 remote-heavy small gap（vs tcmalloc）質問パック

目的:
- `T=32 / R=50 / size=16–2048` で **tcmalloc が大幅に強い**。
- hz3 の弱点（remote-heavy small）を、**hot path を汚さず**に改善する案を相談したい。

前提: Box Theory
- 変更は箱に閉じる（境界1箇所）
- compile-time A/B（戻せる）
- 観測はワンショット/最小
- Fail-Fast を用意

---

## 1) ベンチ結果（現状のギャップ）

T=32, R=50, size=16–2048:
- system: 16.49M
- tcmalloc: 122.69M
- hz3-fast: 36.71M
- hz3-scale: 89.89M

→ hz3-scale は改善したが、tcmalloc には **-27% ほど差**がある。

---

## 2) perf 結果（remote-heavy small）

### hz3-scale（LD_PRELOAD）
コマンド:
```
perf record -e cycles:u -F 999 -g -- env LD_PRELOAD=./libhakozuna_hz3_scale.so \
  ./hakozuna/out/bench_random_mixed_mt_remote_malloc 32 250000 400 16 2048 50 65536
```

主要ホット（self, cycles:u）:
- bench_thread: 36.9%
- hz3_small_v2_alloc_slow: 16.6%
- hz3_malloc: 9.6%
- pthread_mutex_lock: 6.0%
- hz3_small_v2_central_push_list: 5.8%
- hz3_free: 4.6%
- pthread_mutex_unlock: 4.0%
- hz3_dstbin_flush_remote_budget: 3.4%
- hz3_small_v2_push_remote_list: 2.1%
- hz3_remote_stash_push: 1.2%

レポート: `/tmp/perf_hz3_scale_T32_R50_small_report.txt`

### tcmalloc（LD_PRELOAD）
コマンド:
```
perf record -o /tmp/perf_tcmalloc_T32_R50_small.data -e cycles:u -F 999 -g -- \
  env LD_PRELOAD=/lib/x86_64-linux-gnu/libtcmalloc.so \
  ./hakozuna/out/bench_random_mixed_mt_remote_malloc 32 250000 400 16 2048 50 65536
```

主要ホット（self, cycles:u）:
- bench_thread: 31.7%
- SpinLock::SpinLoop(): 17.2%
- tc_malloc: 14.6%
- operator delete[]: 13.1%
- CentralFreeList::FetchFromOneSpans: 4.9%
- ThreadCache::ReleaseToCentralCache: 3.9%
- CentralFreeList::ReleaseToSpans: 3.0%

レポート: `/tmp/perf_tcmalloc_T32_R50_small_report.txt`

---

## 3) 仮説（hz3 側の弱点）

- **central への push/lock が重い**
  - `pthread_mutex_lock/unlock` + `*_central_push_list` が目立つ
- **remote flush の頻度が高い**
  - `hz3_dstbin_flush_remote_budget` が 3.4%
- **small_v2_alloc_slow が重い**
  - remote-heavy で slow alloc が多発している可能性

---

## 4) 相談したい質問

### Q1. remote-heavy small のギャップに効く “event-only 箱” は？
例:
- flush 側で `(dst,bin)` を coalesce して push_list 回数を減らす
- central lock の stripe 分割
- cache cap の調整（threads 依存の縮小）

**hot path の命令数を増やさない**前提で、最優先の箱はどれ？

### Q2. tcmalloc が強い理由は “spin lock + transfer” に見えるが、hz3 で近づける最小変更は？
hot を汚さずにできる “tcmalloc 的” 手はある？

### Q3. hz3 にとって「remote-heavy small」だけ伸ばす安全な A/B は？
fast lane を壊さず、scale lane だけで A/B できる設計案がほしい。

---

## 5) 参照してほしいソース

1) free hot path: `hakozuna/hz3/src/hz3_hot.c`
2) remote stash/flush: `hakozuna/hz3/src/hz3_tcache.c`
3) small v2 central: `hakozuna/hz3/src/hz3_small_v2.c`
4) inbox: `hakozuna/hz3/src/hz3_inbox.c`
5) config/build: `hakozuna/hz3/include/hz3_config.h`, `hakozuna/hz3/Makefile`

