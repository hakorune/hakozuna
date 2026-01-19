# S35: dst compare（`dst == my_shard`）コストの切り分けと削減（S33後）

Status: TODO

目的:
- S33（GO）で `hz3_free` の `bin < 128` range check を削除し、dist=app が大きく改善した。
- その後の perf で `hz3_free` 内の **`dst == my_shard` compare** が最大ホットスポットとして残っている。
- ここを “推測” ではなく A/B で切り分け、**消せるなら消す**／無理なら次の箱へ進む。

参照:
- S33: `hakozuna/hz3/docs/PHASE_HZ3_S33_FREE_LOCAL_BIN_RANGE_CHECK_REMOVAL_WORK_ORDER.md`
- S32: `hakozuna/hz3/docs/PHASE_HZ3_S32_RESULTS.md`（row_off NO-GO）
- S34: `hakozuna/hz3/docs/PHASE_HZ3_S34_POST_S33_GAP_REFRESH_AND_NEXT_BOX_WORK_ORDER.md`
- hz3 flags: `hakozuna/hz3/docs/BUILD_FLAGS_INDEX.md`

---

## 背景（現状の構造）

`HZ3_PTAG_DSTBIN_ENABLE=1` の hot free は PTAG32 で `(dst,bin)` を得て、push 先を決める。

`HZ3_LOCAL_BINS_SPLIT=1` のときだけ、free hot で

```
if (dst == t_hz3_cache.my_shard) local_bins[bin] へ push
else                           bank[dst][bin]   へ push
```

という分岐が入り、これが `hz3_free` の固定費として見えている（S34 perf）。

注意:
- S32-2（row_off）は **NO-GO**。branch を消す代わりに load/アドレス計算を増やすと負ける。
- よって S35 は「branchless にする」ではなく、まず **branch 自体が本当に“払う価値があるか”**を見極める。

---

## 方針（箱理論）

- hot path に “新しい箱” を足さない（層を厚くしない）。
- A/B は compile-time `-D` のみ（allocator本体の env ノブ禁止）。
- GO/NO-GO の主判定は RUNS=30。
- NO-GO は `hakozuna/hz3/archive/research/...` に固定し、`NO_GO_SUMMARY.md` へ索引化。

---

## Step 1（必須）: A/Bで「local bins split の価値」を再確認

### 重要（測定の落とし穴）

`hakozuna/hz3/scripts/run_bench_hz3_ssot.sh` は既定で **再ビルド**するため、手動ビルドした `HZ3_LOCAL_BINS_SPLIT=0` が
スクリプト実行で **Makefile既定（split=1）に上書き**されると、A/B が同一設定になって測定が無効になる。

対策（どちらでもOK）:

1) **スクリプトに `HZ3_LDPRELOAD_DEFS` を渡す**（推奨・簡単）
   - `run_bench_hz3_ssot.sh` は `HZ3_LDPRELOAD_DEFS` を受け取り、ビルドに反映し、ログヘッダにも記録する。

2) **`SKIP_BUILD=1` でスクリプトの自動ビルドを無効化**して、手動ビルドした `.so` をそのまま計測する
   - “確実に同じ `.so` を測る” のに向く（A/Bの自動化は少し手間）。

### A: 現状（baseline）

hz3 lane 既定（`hakozuna/hz3/Makefile:HZ3_LDPRELOAD_DEFS`）のまま。

### B: `HZ3_LOCAL_BINS_SPLIT=0`（dst compare を“構造ごと”消す）

狙い:
- `dst == my_shard` compare を hot から消す（local/remote を分けない）。
- 代わりに local も `bank[my_shard][bin]` に集約する（2D index は残る）。

やり方（推奨）:
- `run_bench_hz3_ssot.sh` 実行時に `HZ3_LDPRELOAD_DEFS=...` を渡し、スクリプト側にビルドさせる。

例:
```bash
cd /mnt/workdisk/public_share/hakmem
RUNS=30 ITERS=20000000 WS=400 \
BENCH_BIN=./hakozuna/out/bench_random_mixed_malloc_dist \
BENCH_EXTRA_ARGS="app" \
TCMALLOC_SO=/lib/x86_64-linux-gnu/libtcmalloc_minimal.so \
HZ3_LDPRELOAD_DEFS='-DHZ3_ENABLE=1 -DHZ3_SHIM_FORWARD_ONLY=0 \
  -DHZ3_SMALL_V2_ENABLE=1 -DHZ3_SEG_SELF_DESC_ENABLE=1 \
  -DHZ3_SMALL_V2_PTAG_ENABLE=1 -DHZ3_PTAG_V1_ENABLE=1 \
  -DHZ3_PTAG_DSTBIN_ENABLE=1 -DHZ3_PTAG_DSTBIN_FASTLOOKUP=1 \
  -DHZ3_PTAG32_NOINRANGE=1 -DHZ3_REALLOC_PTAG32=1 -DHZ3_USABLE_SIZE_PTAG32=1 \
  -DHZ3_TCACHE_INIT_ON_MISS=1' \
./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh
```

測定（RUNS=30）:
- dist=app（`bench_random_mixed_malloc_dist ... app`）
- uniform（`bench_random_mixed_malloc_args`）
- tiny-only（16–256）

GO条件（Bが勝つなら）:
- dist=app: +0.5% 以上
- uniform/tiny-only: 退行なし（±1%）

NO-GO:
- dist=app が動かない/退行、または uniform/tiny-only が明確に退行

この Step 1 で、`dst compare` を “消すべきか/残すべきか” の方向が確定する。

---

## Step 2（任意）: perfで「compareが本当に支配か」を再確認

`perf annotate` は “行のコスト” が誤読されることがあるため、`perf stat` も併用する。

- `perf stat -e cycles,instructions,branches,branch-misses`
- B（split=0）で branches が減っても cycles が増えるなら、compare ではなく “bank index の load/計算” が支配。

---

## Step 3（BがNO-GOだった場合の次の箱）

split を残す（dst compare を受け入れる）方向になったら、次は “compare以外” を削る。

候補（S36）:
- `PTAG32 dst/bin 抽出` の命令形（decode/型）を `objdump` ベースで詰める
- `bin index sign-extension`（`movslq`）が出ている箇所を “unsigned化” で潰す
  - 例: `uint16_t bin` を `int` にキャストする前に `uint32_t` へ拡張しておく、など

※ row_off の再挑戦はしない（S32-2 NO-GOで確定）。
