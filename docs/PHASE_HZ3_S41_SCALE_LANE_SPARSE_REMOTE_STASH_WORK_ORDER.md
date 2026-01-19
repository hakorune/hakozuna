# S41: hz3-scale（32threads）導入 — Sparse RemoteStashBox（TLSをshards非依存にする）Work Order

Status: **COMPLETE (GO)**

結果:
- 実装サマリ: `hakozuna/hz3/docs/PHASE_HZ3_S41_IMPLEMENTATION_STATUS.md`
- ST SSOT: `hakozuna/hz3/docs/PHASE_HZ3_S41_STEP2_RESULTS.md`

目的:
- **fast lane（<=8 threads, 100M+ ops/s）を絶対に守りつつ**、hz3 を **32 threads** でスケールさせる。
- `HZ3_NUM_SHARDS` を “max threads” ではなく **owner(heap) 数**として扱い、threads はランタイム無制限で動く設計に寄せる。

背景（現状の課題）:
- 現状は TLS に `bank[dst][bin]` と `outbox[owner][sc]` を持つため、`HZ3_NUM_SHARDS` を増やすと TLS が線形に肥大化する。
- 32 threads を狙って shard を増やしたいが、**TLSの爆発**（特に S40 の `HZ3_BIN_PAD=256` を含む）で fast lane が崩れやすい。

方針（Box Theory）:
- Remote まわりを **RemoteStashBox** として 1 箇所に閉じ込める（境界APIを固定）。
- 実装は 2 レーンで分け、**fast を凍結**してから scale を積む。

---

## 0) SSOT（設計の正）

1) fast lane は現状維持（S40 既定ON）で固定する
- `hakozuna/hz3/Makefile` の lane 既定（現状）

2) scale lane は “TLSをshards依存にしない” remote stash を導入してから `HZ3_NUM_SHARDS=32` にする

3) collision は安全性ではなく主に性能問題として扱う（観測/Fail-Fastは init-only）
- `HZ3_SHARD_COLLISION_SHOT / FAILFAST`（既に実装済み）

---

## 1) 箱割り（境界 1 箇所化）

### Box: RemoteStashBox（新設）

責務:
- `hz3_free` の remote 経路（dst != my_shard）を、hot を汚さずに “どこへ溜めるか” を管理する。
- event-only（refill/epoch/destructor）で owner へ flush する。

境界API（この形を守る）:
- `hz3_remote_stash_push(dst, bin, ptr)`（hot）
- `hz3_remote_stash_flush_budget(budget)`（event-only）
- `hz3_remote_stash_flush_all()`（event-only）

実装（2レーン）:
- **DenseTLSStash（fast lane）**
  - 現状の `bank[dst][bin]`（TLS）＋既存の flush 実装（dst/bin 走査）
  - `HZ3_NUM_SHARDS=8` を既定に固定（fast を死守）
- **SparseOutboxRing（scale lane）**
  - TLS は `outbox_ring[N]`（`{dst, bin, ptr}`）だけ
  - flush は ring を drain して `inbox/central` へまとめて投入（event-only）
  - `HZ3_NUM_SHARDS=32` を採用して owner 空間を広げる（collision を減らす）

---

## 2) 実装順（fastを守るための工程）

### Step 1: fast lane を凍結（混線防止）

狙い:
- 既存の `libhakozuna_hz3_ldpreload.so`（fast）を “触らない正” として固定し、以後の比較を壊さない。

やること:
- Makefile に **fast/scale で別 .so を吐くターゲット**を追加する（例: `libhakozuna_hz3_fast.so` / `libhakozuna_hz3_scale.so`）。
- `run_bench_hz3_ssot.sh` に `HZ3_SO` 上書きで lane を選べる導線があるので、それを使う（既存）。

合格条件:
- fast lane の SSOT（small/medium/mixed + dist=app）が現状と同等である（再現性を壊さない）。

### Step 2: RemoteStashBox の “sparse 実装” を追加（scale の核）

狙い:
- TLS から `bank[dst][bin]` / `outbox[dst][sc]` 依存を外す。

やること（最小）:
- `hz3_remote_stash_push()` は ring へ push（満杯なら event-only flush をトリガする形にする）
- `hz3_remote_stash_flush_*()` で ring を drain して
  - small v2: `hz3_small_v2_push_remote_list(dst, sc, ...)`
  - sub4k: `hz3_sub4k_push_remote_list(dst, sc, ...)`
  - medium: `hz3_inbox_push_list(dst, sc, ...)`
  に流す（現状の “dstbin_flush_one” の分岐と同じ構造を再利用）

注意（hot を汚さない）:
- できるだけ `dst/bin decode` 以外を増やさない（call税を避ける）
- budget flush は event-only 入口（alloc_slow/epoch）から呼ぶ

### Step 3: scale lane を `HZ3_NUM_SHARDS=32` へ

狙い:
- owner 空間を広げて collision と contention を下げる（32threads を本命にする）

やること:
- scale lane の build だけ `-DHZ3_NUM_SHARDS=32` を付ける
- PTAG32 の dst は 8bit なので tag 形式は変えない（ただし既存の `_Static_assert` に従う）

### Step 4: 32threads SSOT（新設 or 既存導線に追加）

狙い:
- 32threads で fast vs scale vs mimalloc vs tcmalloc を同一条件で比較できるようにする。

やること:
- まずは “既存ベンチの MT 版” を 1 本追加する（最小）
  - tiny/small/medium/mixed のどれか（まず mixed 推奨）
  - seed 固定、RUNS=30 median
- 次に dist=app MT 版（時間が許す範囲）

GO/NO-GO（scale lane）目安:
- 32threads で tcmalloc との差が縮む（あるいは勝つ）方向に動く
- fast lane の ST 性能が落ちない（fast を巻き込まない）

---

## 3) 予定（目安）

- Day 1: Step 1（fast/scale の .so 分離 + SSOT 1回）
- Day 2: Step 2（RemoteStashBox + sparse ring 実装、STで動作/SSOT）
- Day 3: Step 3（scale を `HZ3_NUM_SHARDS=32` に上げる、STで sanity）
- Day 4: Step 4（32threads SSOT 導線 + A/B）

（NO-GO の場合は必ず archive してビルドから隔離）

---

## 4) Rollback（必須）

- fast lane は常に現状に戻せる（fast .so を使う）
- scale lane の変更は `HZ3_SO`（ロードする .so）切替で即OFFにできる
- 追加フラグは compile-time で A/B し、既定は lane に閉じ込める

---

## 5) 現在地（2026-01-03）

完了:
- Step 1: Makefile で fast/scale の `.so` 分離（混線防止）
- Step 2: sparse ring（RemoteStashBox: SparseOutboxRing）実装 + hot 統合
- Step 3: scale lane に `HZ3_NUM_SHARDS=32` を適用（TLS bloat 回避）

未完:
- Step 4: MT ベンチ（32 threads）で scale の “本命” を確定（GO/NO-GO）→ ✅完了

結果:
- `hakozuna/hz3/docs/PHASE_HZ3_S41_STEP4_MT_REMOTE_BENCH_WORK_ORDER.md`
