# HZ4 B34 Post-Commit Benchmark Instruction (for Claude)

目的:
- `d6bc503e2` (`HZ4_TLS_DIRECT` default restore) 後の実力を、条件混線なしで再固定する。
- まず `guard/main/cross128` の MT 3レーンで再確認し、その後に必要なら full matrix/perf/RSS へ進む。
- 失敗時は「どこで止まったか」を残して再開可能にする。

---

## 0) 実行前ルール（必須）

1. 作業ディレクトリ:
```bash
cd /mnt/workdisk/public_share/hakmem
```

2. 使う bench バイナリ/so は固定:
- bench:
  - `./hakozuna/out/bench_random_mixed_malloc_args`
  - `./hakozuna/out/bench_random_mixed_mt_remote_malloc`
- allocator so:
  - hz4: `./hakozuna/hz4/libhakozuna_hz4.so`
  - hz3: `./allocators/hz3/libhakozuna_hz3_scale.so`
  - mimalloc: `./allocators/mimalloc/libmimalloc.so`
  - tcmalloc: `./allocators/tcmalloc/libtcmalloc_minimal.so`

3. clean env 実行（PATH/HOME 最小）を優先:
- 既存スクリプトが `env -i` を使うので、それを優先利用する。

4. pin CPU:
- `taskset -c 0-15`

5. 失敗時:
- allocatorごとの `.so` sha1 と `run log` を必ず残す。

---

## 1) Quick Health Gate（最初に必ず）

```bash
make -C hakozuna/hz4 clean test
```

通過条件:
- `All tests passed!`

失敗したら:
- ベンチへ進まない。`out/*` テスト失敗ログを添えて停止。

---

## 2) B34 再確認（3レーン A/B）

目的:
- `HZ4_TLS_DIRECT=0` vs default（=1）を再確認。
- 既に取得済みの `/tmp/hz4_b34_tls_direct_ab_20260214_130959` と同方向か確認。

実行:
```bash
OUT=/tmp/hz4_b34_tls_direct_ab_rerun_$(date +%Y%m%d_%H%M%S)
mkdir -p "$OUT"

# base
make -C hakozuna/hz4 clean libhakozuna_hz4.so HZ4_DEFS_EXTRA='-DHZ4_TLS_DIRECT=0'
cp -f hakozuna/hz4/libhakozuna_hz4.so "$OUT/libhakozuna_hz4_base.so"

# var (default)
make -C hakozuna/hz4 clean libhakozuna_hz4.so
cp -f hakozuna/hz4/libhakozuna_hz4.so "$OUT/libhakozuna_hz4_var.so"

sha1sum "$OUT/libhakozuna_hz4_base.so" "$OUT/libhakozuna_hz4_var.so" \
  ./allocators/tcmalloc/libtcmalloc_minimal.so > "$OUT/dso_sha1.txt"
```

レーン:
- `guard_r0`: `5000000 400 16 2048 0`
- `main_r50`: `5000000 400 16 32768 50`
- `cross128_r90`: `2000000 400 16 131072 90`

RUNS:
- `7`（screen）

通過目安:
- `guard_r0`: `>= +2%`
- `main_r50`: `>= +1%`
- `cross128_r90`: `>= -1%`

---

## 3) Full Matrix（比較の正）

目的:
- 「R=0/50/90 × guard/main/cross128」の 9セルを同条件で取得。
- allocator 4者比較の勝ち負けを再固定。

実行コマンド:
```bash
OUT=/tmp/hz4_post_b34_matrix_$(date +%Y%m%d_%H%M%S)
RUNS=7 \
./scripts/run_mt_lane_remote_matrix.sh \
  --runs "$RUNS" \
  --outdir "$OUT" \
  --bench-bin ./hakozuna/out/bench_random_mixed_mt_remote_malloc \
  --threads 16 \
  --ws 400 \
  --slots 65536 \
  --remote-pcts 0,50,90 \
  --lanes guard,main,cross128 \
  --iters-guard 2000000 \
  --iters-main 2000000 \
  --iters-cross128 600000 \
  --allocs hz3,hz4,mimalloc,tcmalloc \
  --hz3-so ./allocators/hz3/libhakozuna_hz3_scale.so \
  --hz4-so ./hakozuna/hz4/libhakozuna_hz4.so \
  --mimalloc-so ./allocators/mimalloc/libmimalloc.so \
  --tcmalloc-so ./allocators/tcmalloc/libtcmalloc_minimal.so \
  --cpu-list 0-15 \
  --timeout-sec 180 \
  --allow-fail 1 \
  --no-shuffle
```

主要出力:
- `$OUT/summary.tsv`
- `$OUT/matrix.md`
- `$OUT/meta.txt`
- `$OUT/preload_verify_*`

---

## 4) 追加3本（必要なら）

目的:
- pure mid / pure large / xlarge を確認して、9セルだけでは見えない偏りを補完する。

実行コマンド:
```bash
OUT=/tmp/hz4_post_b34_matrix_ext_$(date +%Y%m%d_%H%M%S)
RUNS=7 \
./scripts/run_mt_lane_remote_matrix.sh \
  --runs "$RUNS" \
  --outdir "$OUT" \
  --bench-bin ./hakozuna/out/bench_random_mixed_mt_remote_malloc \
  --threads 16 \
  --ws 400 \
  --slots 65536 \
  --remote-pcts 0,50,90 \
  --lanes guard,main,cross128 \
  --extended \
  --allocs hz3,hz4,mimalloc,tcmalloc \
  --hz3-so ./allocators/hz3/libhakozuna_hz3_scale.so \
  --hz4-so ./hakozuna/hz4/libhakozuna_hz4.so \
  --mimalloc-so ./allocators/mimalloc/libmimalloc.so \
  --tcmalloc-so ./allocators/tcmalloc/libtcmalloc_minimal.so \
  --cpu-list 0-15 \
  --timeout-sec 180 \
  --allow-fail 1 \
  --no-shuffle
```

---

## 5) perf（差があるセルだけ）

推奨対象:
- `guard_r0`（small-only）
- `main_r50`（混在）
- `cross128_r90`（large混在）

手順:
1. matrix で差が大きいセルを特定
2. そのセルのみ `perf stat` と `perf record` 実施

実行:
```bash
OUT_BASE=/tmp/hz4_b27_probe_fixfull_$(date +%Y%m%d_%H%M%S) \
RUNS=7 PERF_RUNS=3 SMOKE=0 \
DO_TC=1 DO_PERF_STAT=1 DO_PERF_RECORD=1 DO_RSS=0 DO_OBS=0 \
./scripts/run_hz4_b27_deep_probe.sh
```

注意:
- このスクリプト名は B27 だが、`base/b27` 比較部分は読み替えが必要。
- B34 専用 perf が必要な場合は、この構成をコピーして `base/var` 用に別スクリプト化する。

---

## 6) RSS（必要時のみ）

matrix の勝敗が出たあと、メモリ効率を確認する場合:

```bash
OUT=/tmp/hz4_post_b34_matrix_rss_$(date +%Y%m%d_%H%M%S)
RUNS=5 \
./scripts/run_mt_lane_remote_matrix.sh \
  --runs "$RUNS" \
  --outdir "$OUT" \
  --bench-bin ./hakozuna/out/bench_random_mixed_mt_remote_malloc \
  --threads 16 \
  --ws 400 \
  --slots 65536 \
  --remote-pcts 0,50,90 \
  --lanes guard,main,cross128 \
  --allocs hz3,hz4,mimalloc,tcmalloc \
  --hz3-so ./allocators/hz3/libhakozuna_hz3_scale.so \
  --hz4-so ./hakozuna/hz4/libhakozuna_hz4.so \
  --mimalloc-so ./allocators/mimalloc/libmimalloc.so \
  --tcmalloc-so ./allocators/tcmalloc/libtcmalloc_minimal.so \
  --cpu-list 0-15 \
  --timeout-sec 180 \
  --allow-fail 1 \
  --do-rss 1 \
  --rss-runs 1 \
  --no-shuffle
```

---

## 7) 報告フォーマット（Claude提出用）

最低限、次を1レポートで提出:

1. 実行 commit:
- `git rev-parse --short HEAD`

2. DSO 固定:
- `summary.tsv` と `dso_sha1.txt`（または `preload_verify_*`）

3. 3レーンA/B（B34 rerun）:
- `guard_r0/main_r50/cross128_r90` の median delta

4. 9セル matrix:
- winner table（hz3/hz4/mimalloc/tcmalloc）

5. 異常:
- timeout / segv / alloc fail / perf fail の件数とセル名

6. 次アクション提案:
- 1行で「次に攻める箱」を指定

---

## 8) 判定基準（今回）

- B34 は既に GO(default) 固定。
- この計測の目的は「再現確認」と「次箱の優先度固定」。
- 次箱優先は、以下を採用:
  - `guard_r0` が大差で負ける: small-side fixed cost box を優先
  - `cross128_r90` が負ける: large-path box を優先
  - `main_r50` だけ弱い: mid lock-path / mixed routing を優先
