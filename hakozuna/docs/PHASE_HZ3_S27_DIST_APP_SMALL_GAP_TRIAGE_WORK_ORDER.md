# S27: dist=app の small（16–2048）残差を詰める（triage→小さい箱）

目的:
- `dist=app` で残っている hz3↔tcmalloc gap の主犯が small（16–2048）側という前提で、原因を分解して次の箱を決める。
- hot path を太らせない（追加分岐/追加ロードを増やさない）。変更は slow/event-only に閉じる。

前提:
- allocator の挙動切替は compile-time `-D` のみ（envノブ禁止）。ベンチの引数はOK。
- 判定は原則 `RUNS=30`（`RUNS=10` の±1%はノイズ扱い）。

参照:
- SSOT: `hakozuna/hz3/scripts/run_bench_hz3_ssot.sh`
- distベンチ: `hakozuna/out/bench_random_mixed_malloc_dist`
- S25: `hakozuna/hz3/docs/PHASE_HZ3_S25_DIST_APP_GAP_POST_S24_TRIAGE_WORK_ORDER.md`
- S26: `hakozuna/hz3/docs/PHASE_HZ3_S26_16K32K_SUPPLY_WORK_ORDER.md`

---

## ゴール（GO条件）

- `dist=app` の `small (16–2048)` で hz3 が tcmalloc に追いつく（目安: gap ≤ 1%）。
- `dist=app` の `mixed (16–32768)` の gap も連動して改善（目安: +1pt 以上）。
- uniform SSOT で退行なし（±1%以内）。

---

## Step 1: small-only の再baseline（RUNS=30）

`dist=app` の small（16–2048）だけを測り、gap を固定する。

```bash
cd hakmem
make -C hakozuna bench_malloc_dist

SKIP_BUILD=0 RUNS=30 ITERS=20000000 WS=400 \
  BENCH_BIN=./hakozuna/out/bench_random_mixed_malloc_dist \
  BENCH_EXTRA_ARGS="0x12345678 app" \
  MIMALLOC_SO=./mimalloc-install/lib/libmimalloc.so \
  TCMALLOC_SO=/usr/lib/x86_64-linux-gnu/libtcmalloc.so \
  ./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh
```

※この SSOT は `small/medium/mixed` 全部回るので、ログから `*_small.log` だけを見る。

---

## Step 2: small を 2帯域に分解（tiny vs non-tiny）

狙い:
- `app` の small は「16–256 が 80%」と「257–2048 が 15%」で性質が違う。
- どちらが gap の主因かを先に決める（当てずっぽう最適化を防ぐ）。

### 2-A: tiny（16–256 を 100%）

```bash
BENCH_EXTRA_ARGS="0x12345678 trimix 16 256 100 257 2048 0 2049 32768 0"
```

### 2-B: 257–2048 を 100%

```bash
BENCH_EXTRA_ARGS="0x12345678 trimix 16 256 0 257 2048 100 2049 32768 0"
```

判定:
- 2-A が主に遅い → tiny（16–256）の箱（サイズ→bin / refill / carve）を疑う
- 2-B が主に遅い → 257–2048 の箱（ページ内 carve / central 返却）を疑う
- 両方遅い → 小物の供給（page alloc/return）側が疑わしい

---

## Step 3: 次の箱（S28候補）

### S28-A: size→bin table（hotの命令数を落とす）

条件:
- tiny が主犯で、`hz3_small_sc_from_size()` / ルーティングが支配的な場合。

方針:
- `size` を小さい table で `bin` に直接変換（分岐/算術を減らす）。
- hot に “追加分岐” を増やさない（単純な table lookup に寄せる）。

### S28-B: small slow path の refill 形（event-only）

条件:
- 257–2048 が主犯で、ページ切り出し/central返却が支配的な場合。

方針:
- `hz3_small_v2_alloc_slow()` の “1回の page 供給→bin充填数” を増やして amortize。
- central への返却は list を O(1) で扱う（walkしない）。

### S28-C: segment/page provider（event-only）

条件:
- small でも segment/new page の頻度が高い場合（page churn）。

方針:
- small 専用の “current_seg” の運用を見直す（断片化/bitmap探索の固定費を減らす）。

---

## 注意

- S26で “16KB–32KB の供給” は改善できたので、S27 は **small側だけ**を対象にする。
- 変更は “hot 0命令 or hot削減” を原則にし、event-only で積む。

