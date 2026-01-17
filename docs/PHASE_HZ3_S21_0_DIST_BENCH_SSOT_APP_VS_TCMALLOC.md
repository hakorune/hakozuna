# S21-0: dist bench（app）で hz3 vs tcmalloc を比較（SSOT-HZ3）

目的:
- `mixed (16–32768)` が “4KB支配のストレス” になりやすい問題を避け、実アプリ寄りの分布（small支配）でも hz3 と tcmalloc を比較する。

使用ベンチ:
- `hakozuna/out/bench_random_mixed_malloc_dist`
- dist: `app`（80%: 16–256 / 15%: 257–2048 / 5%: 2049–32768）

実行（SSOT-HZ3）:
```bash
cd hakmem

make -C hakozuna bench_malloc_dist

# hz3 build（S21 PTAG32-first realloc/usable_size をON）
make -C hakozuna/hz3 clean
make -C hakozuna/hz3 -j8 all_ldpreload \
  HZ3_LDPRELOAD_DEFS='-DHZ3_ENABLE=1 -DHZ3_SHIM_FORWARD_ONLY=0 \
    -DHZ3_SMALL_V2_ENABLE=1 -DHZ3_SEG_SELF_DESC_ENABLE=1 \
    -DHZ3_SMALL_V2_PTAG_ENABLE=1 -DHZ3_PTAG_V1_ENABLE=1 \
    -DHZ3_PTAG_DSTBIN_ENABLE=1 -DHZ3_PTAG_DSTBIN_FASTLOOKUP=1 \
    -DHZ3_REALLOC_PTAG32=1 -DHZ3_USABLE_SIZE_PTAG32=1'

RUNS=10 ITERS=20000000 WS=400 SKIP_BUILD=1 \
  BENCH_BIN=./hakozuna/out/bench_random_mixed_malloc_dist \
  BENCH_EXTRA_ARGS="app" \
  MIMALLOC_SO=./mimalloc-install/lib/libmimalloc.so \
  TCMALLOC_SO=/usr/lib/x86_64-linux-gnu/libtcmalloc.so \
  ./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh
```

ログ:
- `/tmp/hz3_ssot_6cf087fc6_20260101_215211`

結果（median ops/s）

| workload | system | hz3 | mimalloc | tcmalloc |
|----------|--------|-----|----------|
| small    | 41.15M | 55.47M | 57.01M | 57.37M |
| medium   | 12.97M | 55.59M | 42.19M | 59.17M |
| mixed    | 35.51M | 52.64M | 51.48M | 58.27M |

所感:
- “実アプリ寄り（small支配）” でも、hz3 は tcmalloc に迫れるが、mixed（app）ではまだ差が残る。
- `realloc/usable_size` の PTAG32-first 化は hot SSOT（malloc/free only）には直接効かないが、統一に向けた安全な前進として維持できる。

補足:
- `app` は `min..max` によって一部バケットが無効になるため、ベンチ側で無効バケットの weight を 0 に落とす（clamp）実装が必要。
