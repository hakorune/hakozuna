# Question Pack: Learning Layer Memory Budget (hz3 scale lane)

目的:
- hz3 scale lane (HZ3_NUM_SHARDS=32) で **メモリ使用量を抑制**する箱を追加したい。
- hot path は汚さない（event-only / init-only のみ）。
- 学習層の ON/OFF (FROZEN / OBSERVE / LEARN) に対応。

現状の観測:
- 16GB arena で `arena_alloc_full` が発生し `alloc failed`。
- OOM shot で **medium (4KB-32KB) 全 sc が均等に消費**されている。
- S42/S44 は犯人ではない（OFFにしても arena full は出る）。

制約 (Box Theory):
- 境界は 1 箇所（alloc miss / epoch / refill 等の event-only に集約）。
- A/B で即戻せる（compile-time + ENV）。
- Fail-fast / one-shot log を最小限に。

参考: 他 allocator の考え方

選択肢A: tcmalloc 方式 (大きな仮想予約 + mprotect)
```
void* arena = mmap(NULL, 256GB, PROT_NONE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
// 使用時だけ mprotect で commit
mprotect(arena + offset, segment_size, PROT_READ|PROT_WRITE);
```
利点: 連続アドレスで PTAG/arena index が維持できる
懸念: mprotect の固定費

選択肢B: jemalloc 方式 (必要時 mmap / 解放時 munmap)
```
void* seg = mmap(NULL, segment_size, PROT_READ|PROT_WRITE,
                 MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
```
利点: 物理メモリ最小化
懸念: 連続性が崩れて PTAG/arena index が複雑化

--

質問 (設計の分岐が決まるもの)

1) ゴール指標:
   - RSS / arena used slots / mmap count / other?
   - 目標上限や帯域の推奨値は？

2) モード:
   - FROZEN / OBSERVE / LEARN の定義と切替方法は？
   - ENV 名 (HAKMEM_MODE など) は何が良い？

3) Box 形:
   - MemoryBudgetBox をどこに差し込む？
     (epoch / refill / slow alloc / thread exit)
   - hot path に触らず実装する最小境界は？

4) 方式選択:
   - A (large reserve + mprotect) と B (dynamic mmap) のどちらが hz3 と相性が良い？
   - PTAG/arena index を壊さない方法は？

5) Budget policy:
   - cap 方式 (固定 / 動的 / 学習) のおすすめは？
   - どの構造を trim/release すべき？ (central / owner stash / segment)

6) 可視化:
   - one-shot ログに何を出すと有効？
   - 観測カウンタ (alloc, release, arena usage) の最小セットは？

7) 失敗時の戻し方:
   - Fail-fast 条件は何が良い？
   - ON/OFF の最小フラグ分割は？

想定の実装対象 (必要ならここから選んで):
- `hakozuna/hz3/src/hz3_arena.c`
- `hakozuna/hz3/src/hz3_small_v2.c`
- `hakozuna/hz3/src/hz3_tcache.c`
- `hakozuna/hz3/src/hz3_segment.c`

希望:
- まずは scale lane 専用で導入 (fast lane は不変)。
- hot path に増分なし、event-only で安全に切替可能な設計を希望。
