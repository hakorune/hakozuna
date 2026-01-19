# Phase S97-8: RemoteStash “Sort+Group” BucketBox（table-less / stack-only）指示書

## ゴール

`hz3_remote_stash_flush_budget_impl()` の `(dst,bin)` 合流（saved_calls）を取りつつ、
**TLS テーブル**（bucket=2）や **hash probe 分岐**（bucket=1）の固定費を避けて、
低スレッド帯（T=8）でも **NO-GO を解消できるか**を SSOT で確定する。

## 箱の境界（Boundary 1箇所）

- 対象関数（唯一の変更点）: `hakozuna/hz3/src/hz3_tcache_remote.c` の `hz3_remote_stash_flush_budget_impl()`
- 外側は一切変更しない（malloc/free hot path・S44/S67/S112 などは触らない）

## 方式（S97-8 / bucket=6）

- drain した `m` 個（上限 256）を **ローカル配列（stack）**へ詰める
  - `key = (dst<<8)|bin`（16bit）
- 2-pass radix sort（8bit×2）で key を整列（branch 予測に優しい）
- 同一 key が連続する区間を group 化し、list を作って `hz3_remote_stash_dispatch_list()` へ 1回で流す

狙い:
- bucket=2 の TLS（stamp/index）touch をなくす
- bucket=1 の probe loop（branch-miss）をなくす

実装メモ（重要）:
- radix の count/prefix-sum は **固定 256** で回すと r90 で固定費が支配しやすい。
- 本線実装は `HZ3_BIN_TOTAL` と `HZ3_NUM_SHARDS` に **loop を縮めて**固定費を抑える（bin/dst の実レンジに合わせる）。

## フラグ

- `HZ3_S97_REMOTE_STASH_BUCKET=6`（S97-8）
- `HZ3_S97_REMOTE_STASH_SKIP_TAIL_NULL=1`（既存と同じ; n==1 の store 省略）
- 形状観測（任意）: `HZ3_S97_REMOTE_STASH_FLUSH_STATS=1`

## A/B（推奨: 既存スイートを流用）

既存の thread sweep スクリプトを使い、treat を bucket=6 に差し替える。

```sh
RUNS=20 PIN=1 WARMUP=1 THREADS_LIST="8 16" REMOTE_PCTS="50 90" \
  BASE_DEFS_EXTRA='-DHZ3_S44_PREFETCH_DIST=2 -DHZ3_S97_REMOTE_STASH_FLUSH_STATS=1 -DHZ3_S97_REMOTE_STASH_BUCKET=0 -DHZ3_S97_REMOTE_STASH_SKIP_TAIL_NULL=1' \
  TREAT_DEFS_EXTRA='-DHZ3_S44_PREFETCH_DIST=2 -DHZ3_S97_REMOTE_STASH_FLUSH_STATS=1 -DHZ3_S97_REMOTE_STASH_BUCKET=6 -DHZ3_S97_REMOTE_STASH_SKIP_TAIL_NULL=1' \
  ./hakozuna/hz3/scripts/run_ab_s97_bucketize_threadsweep_20runs_suite.sh
```

## 判定（GO/NO-GO）

最低限の GO 条件（まずはこの2本で判断）:

- `r90, T=8`: **+1% 以上**（bucket=2 が弱い帯域なので最優先）
- `r50, T=8`: **-1% 以内**（r50 を壊さない）

補助指標（S97 SSOT）:

- `saved_med`（entries に対する saved_calls%）が 10–15% 近辺で維持されていること
- `nmax` が極端に落ちていないこと（group が成立しているか）

## 結果（2026-01-16, bucket=6 optimized）

測定:
- `bucket=2`（S97-2 direct-map + stamp）vs `bucket=6`（S97-8 sort+group）
- `RUNS=20` interleaved + paired delta median（thread sweep）

結果（median / delta_med）:

| case | bucket=2 | bucket=6 | delta_med | saved_med | 判定 |
|---|---:|---:|---:|---:|---|
| r50 T=8  | 83.17M | 88.70M | +5.21% | 13.27% | ✅ GO |
| r90 T=8  | 74.20M | 81.46M | +8.52% | 13.02% | ✅ GO |
| r50 T=16 | 125.81M | 125.43M | -0.84% | 13.26% | neutral |
| r90 T=16 | 113.02M | 98.01M | -13.03% | 12.78% | ❌ NO-GO |

結論（運用）:
- `bucket=6` は **T=8** 帯で有望（r50/r90 とも改善）。
- ただし r90 の **threads>=16** では退行しうるため、scale 既定には入れず **lane split**（.so 分離）で運用する。
  - r50: `make -C hakozuna/hz3 all_ldpreload_scale_r50_s97_8`
  - r90(T=8): `make -C hakozuna/hz3 all_ldpreload_scale_r90_pf2_s97_8_t8`
  - r90(threads>=16): `make -C hakozuna/hz3 all_ldpreload_scale_r90_pf2_s97_2`（既存）

## Fail-Fast（デバッグ時だけ）

クラッシュや整合性が疑わしいときは、まず list integrity を上げて境界で即落とす:

- `-DHZ3_LIST_FAILFAST=1` を付ける（※ `SKIP_TAIL_NULL=1` と排他なので OFF にしてから）
- または `-DHZ3_STASH_DEBUG=1` を使って boundary で ptr 範囲を fail-fast

## 記録（SSOT）

- 結果（median と delta）をこの指示書と `CURRENT_TASK.md` に追記（lane split の判断もここに残す）
