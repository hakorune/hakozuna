# Question Pack: S45 arena_alloc_full 再発（scale lane）

## 目的
`arena_alloc_full` / `alloc failed` が再発。速度を維持しつつ、メモリ使用量を抑えたまま枯渇を避ける設計案を相談したい。

## 背景 / 直近変更
- scale lane を既定化（Makefileで `all_ldpreload` を scale lane に変更）。
- S45 に **emergency flush** を追加（arena 枯渇時のみ、tcache/inbox → central）。
- S45 の reclaim は **central からのみ** medium run を回収。

## 再現条件
- バイナリ: `hakozuna/out/bench_random_mixed_mt_remote_malloc`
- 実行:
  ```sh
  LD_PRELOAD=./libhakozuna_hz3_ldpreload.so \
    ./hakozuna/out/bench_random_mixed_mt_remote_malloc 32 5000000 400 16 32768 50 65536
  ```
- ビルド: `HZ3_OOM_SHOT=1`（ログ取得用）

## 事象
- `alloc failed` が発生。
- `HZ3_OOM` ログで `arena_alloc_full` を確認。
- OS OOM kill は無し（dmesg には OOMなし）。

## ログ（抜粋）
- `hakozuna/hz3/docs/oom_runs/hz3_oom_run1_all.log`
- `hakozuna/hz3/docs/oom_runs/hz3_oom_run2_all.log`

## 実測まとめ（2回）
- run1: ops/s=148,527,488 / reclaim_calls=26 / reclaim_pages=1109 / alloc_failed=8 / OOM sc=4 (20480)
- run2: ops/s=143,828,795 / reclaim_calls=18 / reclaim_pages=693 / alloc_failed=5 / OOM sc=4 (20480)

補足:
- `HZ3_MEM_BUDGET_MAX_RECLAIM_PAGES=4096` だが、実回収は 693〜1109 pages 程度。
- emergency flush により reclaim は動くが、量的に足りない。

## 現行の境界（arena 枯渇時）
`hz3_arena_alloc()`:
1. `hz3_mem_budget_emergency_flush()`
2. `hz3_mem_budget_reclaim_medium_runs()`（central → segment）
3. `hz3_mem_budget_reclaim_segments()`（free_pages==512 の segment 回収）
4. 失敗時 `HZ3_OOM` / alloc failed

## 制約（Box Theory）
- **hot path を汚さない**（event-only, arena 枯渇境界でのみ）
- my_shard 制約を守る（他 shard の segment を触らない）
- S45 を壊さず、戻せる A/B で導入

## 相談したいポイント（設計）
1. **回収量不足の解消**
   - `HZ3_MEM_BUDGET_MAX_RECLAIM_PAGES` を増やすだけで良いか？
   - arena 枯渇時に **reclaim を複数回ループ**するのは安全か？

2. **回収対象の拡張**
   - owner_stash / tcache の滞留を **central へ押し出す追加経路**が必要か？
   - remote stash の flush を arena 枯渇時に強めるべきか？

3. **メモリ使用量の抑制と速度の両立**
   - reclaim を強めると速度が落ちる可能性あり。
   - 小さい overhead で「使っていない run を捨てる」最小設計は？

4. **デフォルト値の方向性**
   - 16GB arena を維持しつつ枯渇を避ける設計
   - もしくは 32GB に増やして「保険」にするべきか？

## 関連コード
- `hakozuna/hz3/src/hz3_arena.c`
- `hakozuna/hz3/src/hz3_mem_budget.c`
- `hakozuna/hz3/include/hz3_mem_budget.h`
- `hakozuna/hz3/src/hz3_inbox.c`
- `hakozuna/hz3/include/hz3_inbox.h`
- `hakozuna/hz3/src/hz3_tcache.c`
- `hakozuna/hz3/include/hz3_config.h`
- `hakozuna/hz3/docs/PHASE_HZ3_S45_MEMORY_BUDGET_BOX.md`
