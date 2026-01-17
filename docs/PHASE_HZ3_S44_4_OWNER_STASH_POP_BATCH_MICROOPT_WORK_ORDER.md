# PHASE HZ3 S44-4: OwnerStashPopBatch MicroOptBox（既定改善）

目的:
- scale lane の Top ホット `hz3_owner_stash_pop_batch()` を、**r90/r50 両方で崩さず**削る。
- S97-1 bucketize は `saved_calls` を稼げても r50 が大きく落ちたため、scale 既定に入る “広域に効く改善” を優先する。

前提（SSOT）:
- `hz3_owner_stash_pop_batch` は perf で Top1（~17–21%）になりやすい。
- `HZ3_S44_PREFETCH=1` は GO、`HZ3_S44_PREFETCH_DIST=2` は r90 GO / r50 NO-GO（既定 dist=1）。
- S97-1 bucketize: r90 +3% / r50 -11%（scale 既定は NO-GO、r90 opt-in のみ）。

---

## 境界（1箇所）

この箱の境界はここだけ:
- `hakozuna/hz3/src/hz3_owner_stash.c` の `hz3_owner_stash_pop_batch()`

この箱が触ってよいもの:
- pop の内部（drain / walk / spill への退避）の命令数/分岐/ロードの形
- prefetch/unroll/fastpath など “同じ意味のまま” の実装差分

触ってはいけないもの:
- stash の外側（remote stash flush / small alloc slow / central）を巻き込む大改造
- ENV ノブ追加（hz3 は compile-time `-D` で切替）

---

## SSOT（入口）

最低限の A/B は r90 と r50 を 3run median で取る:
- r90（OOM 回避で iters を下げてOK）
- r50（通常 iters）

ベースラインは “現行 scale 既定” を使う（r90 opt-in を混ぜない）:
```sh
make -C hakozuna/hz3 clean all_ldpreload_scale
```

---

## まず観測（shape を固定）

目的:
- “どこを削れば r50 を壊さず効くか” の当たりを付ける。

観測ポイント（追加するなら atexit 1回だけ）:
- pop_calls
- want の分布（want=32 が支配か）
- drained_list の平均長 / p95（drain した list の長さ）
- got / spill の比率（S48 spill がどれだけ効いているか）
- `old_head!=NULL` fastpop がどれだけ外れているか（該当する場合のみ）

注意:
- hot path を汚さない（観測は `#if STATS` のみ、relaxed カウンタ＋atexit 1回）。

提案するフラグ（compile-time のみ）:
- `HZ3_S44_4_STATS=1`（観測: atexit 1回だけ）
- `HZ3_S44_4_WALK_NOPREV=1`（候補: `prev=cur` を消す）
- `HZ3_S44_4_SPILL_HELPER=1`（候補: spill pop の early return 形状統一）

提案する SSOT stats（`#if HZ3_S44_4_STATS`）:
- `pop_calls`（呼び出し回数）
- `want_sum32`（`want` 合計: 32-bit。*このベンチ規模なら overflow しない前提*）
- `want_32_count`（want==32 回数）
- `drained_sum` / `drained_max`（stash drain した list 長の平均/max）
- `spill_hit_count`（spill 系から 1obj 以上取れた回数）
- `fastpop_miss_count`（fastpop を試したが、結局 stash drain に落ちた回数: *定義は実装側で固定*）

出力（atexit 1回だけ、固定 1行）:
- `[HZ3_S44_4] pop_calls=N want_avg=M want_32_pct=X drained_avg=D drained_max=K spill_hit_pct=S fastpop_miss_pct=F`

---

## 候補（micro-opt の箱）

候補は “意味を変えない” ものから順に試す:

1) walk ループの形（命令数/分岐削減）
- `want<=32` に特化して軽く unroll（4 or 8）
- prefetch を維持しつつ、`hz3_obj_get_next` のロード回数を増やさない

1b) S44-4 EPF（early prefetch: 低リスク）
- stash head を spill check より前に prefetch して、後段の walk/leftover のレイテンシを隠す。
- 期待が外れても意味は変わらない（preload が無駄になるだけ）。
- ただし workload によっては prefetch がノイズになるので A/B 必須。
  - `-DHZ3_S44_4_EARLY_PREFETCH=1`
  - `-DHZ3_S44_4_EARLY_PREFETCH_DIST=2` は研究（next も投機 prefetch）。

1c) S44-4: Quick empty check skip（低リスク寄り）
- spill check 後の `atomic_load(headp)` を削り、drain の `NULL` で空を検出する。
- `fastpop_miss_pct≈0%` の workload で `1 load/call` 削減が狙い。
- `-DHZ3_S44_4_SKIP_QUICK_EMPTY_CHECK=1`

1c-2) S44-4: Quick empty check (reuse EPF hint)（低リスク寄り）
- EPF（early prefetch）で既に読んだ `head_hint` を使い、post-spill の quick empty load を “非空パスでは省略” する。
- `head_hint==NULL` のときだけ confirm load を 1 回行い、空なら drain をスキップする。
- `-DHZ3_S44_4_QUICK_EMPTY_USE_EPF_HINT=1`

1d) S44-4: Walk prefetch uncond（低リスク寄り / x86 only）
- walk loop の `if(next)` を消して `__builtin_prefetch(next)` を無条件化（dist=1 の分岐固定費を削る）。
- `-DHZ3_S44_4_WALK_PREFETCH_UNCOND=1`

2) drain の形（r50 を壊しやすいので慎重）
- `atomic_exchange` を変えずに周辺の分岐/早期 return を減らす
- bounded drain（CAS detach）は既に NO-GO 履歴あり。再挑戦するなら “観測で CAS retry が実際どうか” を先に取る

3) spill の形（r50 に効く可能性はあるが副作用も大）
- 既存の S48 spill を維持し、追加の store を増やさない方向のみ

---

## A/B 手順（最低限）

ビルド（scale 既定を基準にする）:
```sh
make -C hakozuna/hz3 clean all_ldpreload_scale \
  HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_S44_4_EARLY_PREFETCH=0'
cp ./libhakozuna_hz3_scale.so /tmp/s44_4_off.so

make -C hakozuna/hz3 clean all_ldpreload_scale \
  HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_S44_4_STATS=1'
cp ./libhakozuna_hz3_scale.so /tmp/s44_4_stats.so

make -C hakozuna/hz3 clean all_ldpreload_scale \
  HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_S44_4_EARLY_PREFETCH=1'
cp ./libhakozuna_hz3_scale.so /tmp/s44_4_epf.so

make -C hakozuna/hz3 clean all_ldpreload_scale \
  HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_S44_4_SKIP_QUICK_EMPTY_CHECK=1 -DHZ3_S44_4_WALK_PREFETCH_UNCOND=1'
cp ./libhakozuna_hz3_scale.so /tmp/s44_4_qskip_pfuncond.so

make -C hakozuna/hz3 clean all_ldpreload_scale \
  HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_S44_4_QUICK_EMPTY_USE_EPF_HINT=1 -DHZ3_S44_4_WALK_PREFETCH_UNCOND=1'
cp ./libhakozuna_hz3_scale.so /tmp/s44_4_qhint_pfuncond.so
```

注記（2026-01-13 以降）:
- scale lane 既定で `HZ3_S44_4_EARLY_PREFETCH=1` が有効になったため、baseline を作るなら `-DHZ3_S44_4_EARLY_PREFETCH=0` を使う。

ベンチ（3run median）:
```sh
# r90（OOM 回避のため iters=2,000,000 に落としてOK）
for lib in s44_4_off s44_4_epf s44_4_qskip_pfuncond; do
  echo "=== $lib r90 ==="
  for i in 1 2 3; do
    env -u LD_PRELOAD LD_PRELOAD=/tmp/${lib}.so \
      ./hakozuna/out/bench_random_mixed_mt_remote_malloc 32 2000000 400 16 2048 90 65536 2>&1 | tee /tmp/${lib}_r90_${i}.log
  done
done

# r50（通常 iters=3,000,000）
for lib in s44_4_off s44_4_epf s44_4_qskip_pfuncond; do
  echo "=== $lib r50 ==="
  for i in 1 2 3; do
    env -u LD_PRELOAD LD_PRELOAD=/tmp/${lib}.so \
      ./hakozuna/out/bench_random_mixed_mt_remote_malloc 32 3000000 400 16 2048 50 65536 2>&1 | tee /tmp/${lib}_r50_${i}.log
  done
done
```

---

## GO / NO-GO

GO:
- r90/r50 の median が両方改善、または r90 改善 + r50 退行なし（±1% 以内）
- perf で `hz3_owner_stash_pop_batch` の self% が下がる

NO-GO:
- r50 が 1% 超 落ちる（scale 既定には入れない）
- *絶対 NO-GO*: r50 が 5% 以上落ちる
- 退行の理由が “追加 store/ハッシュ探索/分岐増” 系（S97-1 と同じ形）

---

## 結果（2026-01-13）

### 観測 (HZ3_S44_4_STATS=1)

| 指標 | r50 | r90 |
|------|-----|-----|
| pop_calls | 2.5M | 3.1M |
| want_32_pct | 100% | 100% |
| drained_avg | 3.4 | 3.4 |
| spill_hit_pct | 12.7% | 11.6% |
| fastpop_miss_pct | 0.0% | 0.0% |

### WALK_NOPREV (NO-GO)

| 条件 | Baseline | NOPREV | 変化 |
|------|----------|--------|------|
| r90 | ~102M | ~88M | **-14% NO-GO** |
| r50 | ~130M | ~132M | +1.5% |

原因: `prev = cur` は register 代入で安価、`out[got-1]` は memory read で高価。drained_avg=3.4 のため walk loop 反復が短く prev オーバーヘッドが元々小さい。

### EARLY_PREFETCH (GO)

| 条件 | Baseline | EPF | 変化 |
|------|----------|-----|------|
| r90 | 89.75M | 98.0M | **+9.2% GO** |
| r50 | 121.0M | 123.5M | **+2.1% GO** |

**scale 既定に採用**: `HZ3_S44_4_EARLY_PREFETCH=1`（`hakozuna/hz3/include/hz3_config_scale.h:110`）

### Safe Combined (GO)

| 組み合わせ | r90 | r50 | R=0 | 判定 |
|-----------|-----|-----|-----|------|
| EPF_HINT + UNCOND | **+2.8%** | **±0.0%** | **+5.7%** | **GO** |
| SKIP_QUICK + UNCOND | +14.2% | -1.4% | +2.6% | NO-GO（r50退行） |

**scale 既定に採用**:
- `HZ3_S44_4_QUICK_EMPTY_USE_EPF_HINT=1`
- `HZ3_S44_4_WALK_PREFETCH_UNCOND=1`

追加確認（10runs suite）:
- `SKIP_QUICK + UNCOND` は 10runs × 6cases で `mt_remote_r50_small` が **-3.71%** → scale 既定 **NO-GO** を確定。
- suite: `hakozuna/hz3/scripts/run_ab_s44_skipquick_10runs_suite.sh`

---

## 参照

- S44 指示書（既存設計/導線）: `hakozuna/hz3/docs/PHASE_HZ3_S44_OWNER_SHARED_STASH_WORK_ORDER.md`
- S97-1（r90 opt-in）: `hakozuna/hz3/docs/PHASE_HZ3_S97_1_REMOTE_STASH_BUCKET_BOX_WORK_ORDER.md`
- A/B suite（10runs, 複数ベンチ）: `hakozuna/hz3/scripts/run_ab_s44_skipquick_10runs_suite.sh`
