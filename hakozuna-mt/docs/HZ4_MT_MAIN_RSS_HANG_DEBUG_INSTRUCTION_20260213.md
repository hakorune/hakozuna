# HZ4 MT main RSS ハング調査 指示書（他AI向け, 2026-02-13）

目的:
- `hz4` の `mt_main` RSS 計測で発生した長時間ハングを、再現可能な手順で切り分ける。
- 原因を次の2択に確定する。
  1) 運用/DSO混線起因（`(deleted)` DSO など）
  2) `hz4` 本体の競合バグ（lock wait / livelock）

---

## 0. 事象（固定事実）

- 発生日: **2026-02-13**
- 該当ラン: `/tmp/hz4_fullbench_20260213_135055/mt_main`
- `ops` フェーズは完了（40/40）。
- `rss_hz4_1` が約33分停止（手動 `SIGTERM`）。
- 同じ条件で `rss_hz4_2` / `rss_hz4_3` は約1秒で完走。
- 停止中は 1 スレッドのみ実行、他スレッドは `futex_wait_queue` 待機。
- 停止中プロセスに対する `perf` では `libhakozuna_hz4.so (deleted)` 表示あり。

結論候補:
- 「古い DSO を掴んだまま実行」か、
- 「低頻度の本体バグ」かを分離する。

---

## 1. ルール（Fail-Fast）

- ベンチ実行中に `make -C hakozuna/hz4 ...` を走らせない（DSO差し替え禁止）。
- 実行 DSO は `/tmp` へコピーした固定ファイルを使う。
- ハング検知閾値は **15秒**（通常は約1秒で完走）。
- ハングを1回でも捕捉したら、その時点で採取して停止。

---

## 2. 前提固定（毎回）

```bash
cd /mnt/workdisk/public_share/hakmem

BENCH=./hakozuna/out/bench_random_mixed_mt_remote_malloc
ARGS='16 2000000 400 16 32768 90 65536'
CPU='0-15'

OUT=/tmp/hz4_mtmain_hang_dbg_$(date +%Y%m%d_%H%M%S)
mkdir -p "$OUT"

# hz4 をビルドして固定 DSO を作る（以後この so だけ使用）
make -C hakozuna/hz4 clean all
HZ4_SO=/tmp/hz4_mtmain_hang_dbg_$(date +%Y%m%d_%H%M%S).so
cp -f ./hakozuna/hz4/libhakozuna_hz4.so "$HZ4_SO"
sha1sum "$HZ4_SO" | tee "$OUT/hz4_so.sha1"

# strict preload 検証（必須）
./scripts/verify_preload_strict.sh \
  --so "$HZ4_SO" \
  --bin "$BENCH" \
  --runs 3 \
  --outdir "$OUT/preload_verify"
```

---

## 3. ハング再現 + 自動採取（そのまま実行）

```bash
cd /mnt/workdisk/public_share/hakmem

BENCH=./hakozuna/out/bench_random_mixed_mt_remote_malloc
ARGS='16 2000000 400 16 32768 90 65536'
CPU='0-15'
OUT=${OUT:-/tmp/hz4_mtmain_hang_dbg_$(date +%Y%m%d_%H%M%S)}
mkdir -p "$OUT"
HZ4_SO=${HZ4_SO:?set HZ4_SO first}

echo -e "run\trc_or_hang\thung\tops" > "$OUT/runs.tsv"

for i in $(seq 1 50); do
  log="$OUT/run_${i}"
  echo "[RUN] $i" | tee -a "$OUT/progress.log"

  env LD_PRELOAD="$HZ4_SO" taskset -c "$CPU" "$BENCH" $ARGS >"${log}.out" 2>"${log}.err" &
  pid=$!
  start=$(date +%s)
  hung=0

  while kill -0 "$pid" 2>/dev/null; do
    now=$(date +%s)
    elapsed=$((now - start))
    if (( elapsed > 15 )); then
      hung=1
      break
    fi
    sleep 0.2
  done

  if (( hung == 0 )); then
    wait "$pid"
    rc=$?
    ops=$(grep -oE 'ops/s=[0-9.]+' "${log}.out" | head -1 | cut -d= -f2 || echo nan)
    echo -e "${i}\t${rc}\t0\t${ops}" >> "$OUT/runs.tsv"
    continue
  fi

  snap="$OUT/hang_run_${i}"
  mkdir -p "$snap"
  echo "$pid" > "$snap/pid.txt"

  # 観測（SSOT）
  ps -L -p "$pid" -o pid,tid,psr,pcpu,state,wchan:30,comm > "$snap/ps_L.txt" || true
  tr '\0' '\n' < /proc/"$pid"/environ | grep -E '^(LD_PRELOAD|HZ4_|HAKMEM_)' > "$snap/environ.txt" || true
  grep -E 'hakozuna_hz4|libhakozuna_hz4' /proc/"$pid"/maps > "$snap/maps_hz4.txt" || true

  for t in /proc/"$pid"/task/*/wchan; do
    tid=${t%/wchan}; tid=${tid##*/}
    w=$(cat "$t" 2>/dev/null || echo NA)
    echo -e "${tid}\t${w}"
  done | sort -k2,2 > "$snap/wchan_by_tid.tsv"

  hot_tid=$(ps -L -p "$pid" -o tid=,pcpu= | sort -k2 -nr | head -1 | awk '{print $1}')
  if [[ -n "${hot_tid:-}" ]]; then
    perf record -F 199 -g -t "$hot_tid" -o "$snap/perf_hot_tid.data" -- sleep 3 || true
    perf report --stdio --no-children -n -i "$snap/perf_hot_tid.data" > "$snap/perf_hot_tid.txt" || true
  fi

  kill -TERM "$pid" || true
  sleep 1
  kill -KILL "$pid" 2>/dev/null || true
  wait "$pid" 2>/dev/null || true

  echo -e "${i}\tHANG\t1\tnan" >> "$OUT/runs.tsv"
  echo "[HANG] captured at run $i -> $snap" | tee -a "$OUT/progress.log"
  break
done
```

---

## 4. カウンター採取（ハング未再現でも実施）

`lock_path` 偏りと mid/OS の状態を確認する。

```bash
cd /mnt/workdisk/public_share/hakmem

make -C hakozuna/hz4 clean libhakozuna_hz4.so \
  HZ4_DEFS_EXTRA='-DHZ4_MID_STATS_B1=1 -DHZ4_OS_STATS=1'

HZ4_SO_STATS=/tmp/hz4_mtmain_stats_$(date +%Y%m%d_%H%M%S).so
cp -f ./hakozuna/hz4/libhakozuna_hz4.so "$HZ4_SO_STATS"
sha1sum "$HZ4_SO_STATS" | tee "$OUT/hz4_so_stats.sha1"

for i in $(seq 1 5); do
  env LD_PRELOAD="$HZ4_SO_STATS" taskset -c 0-15 \
    ./hakozuna/out/bench_random_mixed_mt_remote_malloc \
    16 2000000 400 16 32768 90 65536 \
    > "$OUT/stats_run_${i}.out" 2> "$OUT/stats_run_${i}.err"
done

grep -h '\[HZ4_MID_STATS_B1\]' "$OUT"/stats_run_*.err > "$OUT/mid_stats_b1.log" || true
grep -h '\[HZ4_OS_STATS' "$OUT"/stats_run_*.err > "$OUT/os_stats.log" || true
```

---

## 5. 判定基準

### A. DSO混線（運用問題）

以下を満たす:
- `maps_hz4.txt` に `(deleted)` が出る
- 固定 DSO (`/tmp/...so`) に切り替えると 50 run で再現しない

対応:
- ランナー側の運用修正（ベンチ中の再ビルド禁止、固定SOコピー必須）
- 必要なら `ssot_mt_remote_matrix.sh` に RSS timeout を追加（15〜30秒）

### B. hz4 本体バグ

以下を満たす:
- 固定 DSO でも再現
- `wchan` が futex 待ちに偏り、hot tid が同じループを回り続ける

対応:
- `perf_hot_tid.txt` と `mid_stats_b1.log` を根拠に、該当箱（Mid lock/owner/remote）を切り分け
- 最小再現（1 allocator, 1 lane）を先に作ってから実装

---

## 6. 提出物（必須）

- `$OUT/hz4_so.sha1`
- `$OUT/runs.tsv`
- `$OUT/progress.log`
- `$OUT/hang_run_*/ps_L.txt`（ハング時のみ）
- `$OUT/hang_run_*/maps_hz4.txt`（ハング時のみ）
- `$OUT/hang_run_*/wchan_by_tid.tsv`（ハング時のみ）
- `$OUT/hang_run_*/perf_hot_tid.txt`（採れた場合）
- `$OUT/mid_stats_b1.log`
- `$OUT/os_stats.log`

報告テンプレ（短く）:
- 再現回数: `X / 50`
- 固定SOでの再現有無: `yes/no`
- `(deleted)` 出現: `yes/no`
- 最上位待機: `futex_wait_queue` 比率
- 次アクション: `運用修正` or `コード修正`
