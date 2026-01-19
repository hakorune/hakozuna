# S25: dist=app の残差を詰める（S24後の再baseline + triage）

目的:
- S24（budgeted flush + remote hint）後の `dist=app` mixed に残る hz3↔tcmalloc の差を、**当てずっぽうではなく triage で原因分解**して次の箱（実装）を決める。
- hot path を太らせない（追加分岐/追加ロードを最小に）。Knob/flush は event-only に隔離する。

前提:
- allocator の挙動切替は compile-time `-D` のみ（envノブ禁止）。ベンチの引数はOK。
- 再現性のため `RUNS=30` + seed固定（`0x12345678`）を基本にする。

参照:
- S24: `hakozuna/hz3/docs/PHASE_HZ3_S24_REMOTE_BANK_FLUSH_BUDGET_WORK_ORDER.md`
- distベンチ: `hakozuna/out/bench_random_mixed_malloc_dist`
- SSOT: `hakozuna/hz3/scripts/run_bench_hz3_ssot.sh`
- triageの過去: `hakozuna/hz3/docs/PHASE_HZ3_S22_DIST_APP_MIXED_TRIAGE_WORK_ORDER.md`

---

## ゴール（GO条件）

- `dist=app` の `mixed (16–32768)` で hz3 が tcmalloc に追いつく（目安: gap ≤ 1%）。
- uniform SSOT（`bench_random_mixed_malloc_args`）で small/medium/mixed 退行なし（±1%以内）。

---

## Step 1: S24後の再baseline（RUNS=30）

目的:
- “いまの trunk の実力” を確定し、以後の triage/実装の比較点を固定する。

コマンド:
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

作業:
- `/tmp/hz3_ssot_<git>_<stamp>/` のログを控える。
- `hakozuna/hz3/docs/PHASE_HZ3_S24_REMOTE_BANK_FLUSH_BUDGET_WORK_ORDER.md` の「S24-2: remote hint（GO）」節に、実測ログと RUNS=30 の中央値を追記する（SSOTをSSOTとして固定する）。

---

## Step 2: dist=app の “どこが遅いか” を分解（triage）

狙い:
- `mixed` の差が「tiny (16–256)」「small (257–2048)」「tail (>2048)」のどこから来ているかを切り分ける。
- ここで “新しい最適化” は入れない。ベンチ引数だけで差を見える化する。

### Triage-A: tail 比率で増幅するか（>2048 側が主犯か）

tail=5%（app相当）:
```bash
BENCH_EXTRA_ARGS="0x12345678 trimix 16 256 80 257 2048 15 2049 32768 5"
```

tail=20%（増幅）:
```bash
BENCH_EXTRA_ARGS="0x12345678 trimix 16 256 70 257 2048 10 2049 32768 20"
```

判定:
- tail比率↑で hz3↔tcmalloc の相対差が悪化 → 次は “>2048 側” の箱（medium/largeの供給・丸め・span carve）を疑う。
- tail比率↑でも差がほぼ変わらない → 次は “<=2048 側” の箱（tiny/小物）を疑う。

### Triage-B: tail 上限を落として large box を薄める（2049–16384）

```bash
BENCH_EXTRA_ARGS="0x12345678 trimix 16 256 80 257 2048 15 2049 16384 5"
```

判定:
- 上限を落とすと差が縮む → 16KB〜32KB 近傍（large box / refill / segment churn）側が疑わしい。
- 変わらない → 2KB〜16KB（特に 2KB〜8KB）側が疑わしい。

### Triage-C: “4097–8191 丸め” を増幅（sub8k が必要か）

“tailを 4097–8191 に絞って重みを上げる”:
```bash
BENCH_EXTRA_ARGS="0x12345678 trimix 16 256 70 257 2048 10 4097 8191 20"
```

判定:
- ここで差が大きく出る → 次の箱は “sub8k / span carve” が本命。

---

## Step 3: 次の箱（実装）の選び方（S26候補）

triage の結果で分岐する:

1) tailが主犯（>2048 側）:
- S26-A: 4KB超の “run=1obj” を卒業し、event-only で **span carve**（複数ページを一括確保→複数objに切る）を導入。
- S26-B: 4097–8191 の **sub8k（少数クラス）**を追加して丸め損失を消す（ただし “bin増やして全bin走査” は禁止。S24の budget/hint とセット）。

2) <=2048 が主犯:
- S26-C: tiny（16–256）を狙った最適化（size→bin table / refill batch / centralの連結 O(1) 化など）を検討。
  - hot 追加分岐/追加ロードは禁止。table化/inline で命令数削減が基本。

---

## 注意（SSOT運用）

- RUNS=10 で上下したものは “ノイズ” の可能性があるので、最終判定は RUNS=30 を原則にする。
- new flags の default を変えるときは、必ず `hakozuna/hz3/docs/BUILD_FLAGS_INDEX.md` と該当フェーズ doc に “既定値” を追記する。

---

## 実測（S25 完了サマリ）

結果（median, ops/s）:

| Test     | Config                | hz3   | tcmalloc | Gap   |
|----------|-----------------------|-------|----------|-------|
| baseline | dist=app              | 51.2M | 54.9M    | -6.7% |
| Triage-A | tail=5% (2049-32768)  | 8.68M | 8.73M    | -0.6% |
| Triage-A | tail=20% (2049-32768) | 8.59M | 8.79M    | -2.3% |
| Triage-B | limit=16384           | 8.88M | 8.85M    | +0.3% |
| Triage-C | 4097-8191 (20%)       | 8.53M | 8.62M    | -1.0% |
| Triage-D | 16385-32768 (20%)     | 8.43M | 8.63M    | -2.3% |

結論（暫定）:
- `16KB–32KB` 近傍（主に `sc=4..7`）が “効きやすい残差” の候補。
- 次の箱は供給効率（refill/segment/central）側の可能性が高い。
- 次フェーズ: `hakozuna/hz3/docs/PHASE_HZ3_S26_16K32K_SUPPLY_WORK_ORDER.md`
