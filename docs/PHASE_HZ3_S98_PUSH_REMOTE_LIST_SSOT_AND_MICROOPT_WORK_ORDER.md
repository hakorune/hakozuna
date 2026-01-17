# PHASE HZ3 S98: small_v2_push_remote_list SSOT + MicroOptBox — Work Order

目的:
- 次のホットスポット `hz3_small_v2_push_remote_list()`（~8%）の “形状” を SSOT で固定し、
  **r50 を壊さない**範囲で固定費（分岐/ロード/計算）を削減する。

前提:
- S44-4（pop_batch）は EPF + Safe Combined を scale 既定に採用済み。
- S84（batch/aggregation）は NO-GO（push 側の固定費増が勝つ）。

---

## 境界（1箇所）

この箱の境界はここだけ:
- `hakozuna/hz3/src/hz3_small_v2.c` の `hz3_small_v2_push_remote_list()`（または同等の remote push 境界）

箱が触ってよいもの:
- push_remote_list 内の “同じ意味のまま” の命令数/分岐/ロードの形
- 事前計算の共有（例: dst/bin の decode を 1 回にする）

触ってはいけないもの:
- remote ring 構造そのものの大改造（別フェーズ）
- stash/central の境界を増やす（変換点は 1 箇所のまま）

---

## Phase 1: 観測（SSOT stats）

フラグ（compile-time only, default OFF）:
- `HZ3_S98_PUSH_REMOTE_STATS=0/1`

観測したい指標（atexit 1回だけ）:
- `calls` / `objs_total`
- `n1_calls`（n=1 支配か）
- `dst_local_calls` / `dst_remote_calls`（自分宛 vs 他owner宛）
- `ring_push_calls` / `ring_full_fallback_calls`（fallback が支配してないか）
- `sc_lt32_objs` / `sc_ge32_objs`（既知の偏り再確認）

出力（固定 1 行）:
- `[HZ3_S98_PUSH_REMOTE] calls=N objs=M n1=P local=.. remote=.. ring_full=.. sc_lt32=..`

### 観測結果（2026-01-13）

remote-heavy（`bench_random_mixed_mt_remote_malloc`）での SSOT:
- r90(T=32): `calls≈28.8M` / `n1_pct=100%` / `local_pct=0%` / `s44_ok_pct=100%` / `sc_ge32_pct≈75%`
- r50(T=32): `calls≈24.0M` / `n1_pct=100%` / `local_pct=0%` / `s44_ok_pct=100%` / `sc_ge32_pct≈75%`
- R=0 / dist(app): `calls=0`（この観測は remote push 境界のみなので正常）

示唆:
- `n=1` が 100% → “バッチ化”ではなく **固定費削減**が本命。
- `local=0%` → local 宛の分岐最適化はこのベンチでは無意味。
- `s44_ok=100%` → overflow 経路の最適化は優先度低。

### 観測結果（S98-2 refresh / dist_app 目線, 2026-01-16）

ビルド:
- `HZ3_S98_PUSH_REMOTE_STATS=1`
- `HZ3_S111_REMOTE_PUSH_N1_STATS=1`

dist_app（single-thread）:
- `bench_random_mixed_malloc_dist ... dist=app`
- `[HZ3_S98]` 行は出ず（calls=0 相当）
  - remote push 境界が踏まれていないので正常

r50（remote-heavy）:
- `bench_random_mixed_mt_remote_malloc 32 2000000 400 16 2048 50 65536`
- `[HZ3_S98] calls=15998377 objs=15998377 n1_pct=100% local_pct=0% remote_pct=100% s44_ok_pct=100% s42=0 central=0 sc_lt32_pct=24% sc_ge32_pct=75%`

示唆:
- dist_app では remote push 境界が発生せず（S98 で観測不可）。
- remote-heavy は従来通り `n=1` 支配・overflow ほぼ無し → 固定費削減が主ターゲット。

---

## Phase 2: MicroOpt 候補（安全なものから）

候補A: 事前 decode の共有
- 例: dst/bin/sc の decode を helper で 1 回だけ行い、分岐テーブルに渡す。

候補A-1: fixed-cost（初期化/分岐予測）
- `HZ3_S98_OWNER_STASH_INIT_FASTPATH=1`: `hz3_owner_stash_init()` で “init済みなら pthread_once をスキップ”。
- `HZ3_S98_PUSH_REMOTE_S44_LIKELY=1`: S44 push 成功を `__builtin_expect(...,1)` で示す。

注:
- S98-1 の micro-opt は計測が不安定で **NO-GO（scale既定に入れない）**。
- 本線の分岐増加を避けるため、micro-opt コードは `archive/research` に記録して撤去済み:
  - `hakozuna/hz3/archive/research/s98_1_push_remote_microopt/README.md`

候補B: destination prefetch（低リスク）
- “push 先の箱”（ring slot / owner stash bin）の cache line を先に prefetch して latency を隠す。
- ただし S42-1 の教訓どおり、温かい/低頻度パスでは逆効果になり得るので A/B 必須。

候補C: 分岐形状の整理
- `if (owner==my_owner)` の “同一スレッド/同一owner” 分岐を先に確定させて後段を単純化。

---

## A/B 手順（最低限）

測るもの（各 10runs median 推奨）:
- `bench_random_mixed_mt_remote_malloc`:
  - `r50` / `r90` / `R=0`
- `bench_random_mixed_malloc_dist`:
  - `dist=app` / `uniform`

GO/NO-GO:
- scale 既定に入れる条件:
  - r50 退行なし（±1% 以内）
  - r90 / R=0 / dist のいずれかが改善

10runs suite（推奨）:
- `RUNS=10 WARMUP=1 TREAT_DEFS_EXTRA='-D<YOUR_FLAG>=1' ./hakozuna/hz3/scripts/run_ab_s98_push_remote_10runs_suite.sh`

注意（計測のドリフト対策）:
- `run_ab_s98_push_remote_10runs_suite.sh` は **base/treat を 1run ごとに interleave**（奇数runで base→treat、偶数runで treat→base）して、
  “先に全部base→後で全部treat” の時間ドリフトを避ける。
- それでも結果が反転する場合は、scale既定へ入れず **opt-in の研究箱**として扱い、CPU pin/長尺iters/複数回の再現確認を先に行う。

フラグ単体の再確認（例）:
- `RUNS=20 WARMUP=1 TREAT_DEFS_EXTRA='-DHZ3_S98_PUSH_REMOTE_S44_LIKELY=1' ./hakozuna/hz3/scripts/run_ab_s98_push_remote_10runs_suite.sh`
