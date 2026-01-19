# ChatGPT Pro Question Pack: S32（dist=app gap の主因＝TLS init check + dst compare）

目的:
- hz3（`hakmem/hakozuna/hz3`）が dist=app で tcmalloc に ~8% 負ける理由が perf で確定した。
- **hot path を太らせず**に、残差を詰める「設計案 + 実装順 + A/B条件」を提案してほしい。

---

## 1) 現状の SSOT（RUNS=30）

S29（hz3 lane / LD_PRELOAD / seed固定）:

| Workload | hz3 | tcmalloc | Delta |
|---|---:|---:|---:|
| dist=app | 49.06M | 53.15M | -7.69% |
| uniform | 62.73M | 65.66M | -4.47% |
| tiny-only (16–256) | 8.50M | 8.63M | -1.56% |

S30（tail triage）:
- tail-only（2049–4095 / 4096–8191 / 8192–16384 / 16385–32768）はすべて ~-0.6% で横並び
- app形（80/15/5）で tail レンジだけ差し替えても ~-0.4%
- **結論: tail が主犯ではない**

参照:
- `hakozuna/hz3/docs/PHASE_HZ3_S29_DISTAPP_GAP_REFRESH_PERF_WORK_ORDER.md`
- `hakozuna/hz3/docs/PHASE_HZ3_S30_RESULTS.md`

---

## 2) perf（S31）での主因（確定）

`perf record` / `perf annotate` の結論（dist=app）:

| Hotspot | Overhead | 影響 |
|---|---:|---|
| TLS init check | malloc の ~20% | `cmpb + je` が毎回走る |
| dst 比較 | free の ~8% | `cmp %al,%fs:...` が毎回走る |

Allocator 合計:
- hz3: ~23%
- tcmalloc: ~15.5%
- 差分: ~7.5%（≒ dist=app の -7.69% gap）

参照:
- `hakozuna/hz3/docs/PHASE_HZ3_S31_PERF_HOTSPOT_RESULTS.md`

---

## 3) 現在の “正” プロファイル（hz3 lane デフォルト）

重要:
- `hakozuna/hz3/include/hz3_config.h` は **共通デフォルト**（lane 間で共有する値）を持つ
- lane 固有の差分は **Makefile**（`hakozuna/hz3/Makefile`）で決める

`make -C hakmem/hakozuna/hz3 all_ldpreload` の既定（要点）:
- `HZ3_ENABLE=1`
- `HZ3_SHIM_FORWARD_ONLY=0`
- `HZ3_PTAG_DSTBIN_ENABLE=1`（free: PTAG32 dst/bin direct）
- `HZ3_PTAG_DSTBIN_FASTLOOKUP=1`（free: 1-shot lookup）
- `HZ3_PTAG32_NOINRANGE=1`（dist/uniform の固定費削減；tinyはほぼ不変）
- `HZ3_LOCAL_BINS_SPLIT=1`（tiny改善でGO）
- `HZ3_REALLOC_PTAG32=1`, `HZ3_USABLE_SIZE_PTAG32=1`（PTAG32-first）

参照:
- `hakozuna/hz3/Makefile`
- `hakozuna/hz3/docs/BUILD_FLAGS_INDEX.md`

---

## 4) 関連コード位置（読んでほしい最小セット）

TLS init check:
- `hakozuna/hz3/include/hz3_tcache.h`
  - `hz3_tcache_ensure_init()` が `t_hz3_cache.initialized` を毎回チェックして slow init へ
- `hakozuna/hz3/src/hz3_hot.c`
  - `hz3_malloc` / `hz3_free` / `hz3_realloc` / `hz3_usable_size` などが `hz3_tcache_ensure_init()` を呼ぶ箇所がある
- `hakozuna/hz3/src/hz3_tcache.c`
  - `hz3_tcache_ensure_init_slow()`（round-robin shard assign / destructor / epoch init 等）

dst compare:
- `hakozuna/hz3/src/hz3_hot.c`
  - `if (dst == t_hz3_cache.my_shard)` で local bins へ push（S28-2C）
  - remote は bank へ push → event-only flush

PTAG32 lookup:
- `hakozuna/hz3/include/hz3_arena.h`
- `hakozuna/hz3/src/hz3_arena.c`

---

## 5) 制約（SSOT/箱理論）

必須:
- allocatorの挙動切替は **compile-time `-D`** のみ（envノブ禁止）
- hot path に **新しい分岐/新しい共有load** を増やさない（増やすなら A/B で証明）
- 失敗/試行は `hakozuna/hz3/archive/research/<topic>/README.md` に固定し、`hakozuna/hz3/docs/NO_GO_SUMMARY.md` に索引
- GO/NO-GO判定は原則 **RUNS=30**

許容:
- init-only（constructor/once/madvise など）はOK（hot 0命令なら歓迎）
- event-only（epoch/refill/flush）の追加はOK

---

## 6) 質問（答えてほしいこと）

### Q1: TLS init check を “毎回チェック” から排除する最小設計は？

狙い:
- `hz3_malloc` hot path から `cmpb/jcc` を消して、tcmallocと同じく「TLSは常に存在」扱いに寄せたい。

要件:
- 新しい per-op check を増やさない（`pthread_getspecific` の常用は禁止寄り）
- thread exit cleanup（destructor）は維持したい（少なくともSSOTのRSS/孤児化は悪化させない）
- 未初期化TLSでも **安全に** bin head を読める/空扱いできる形にしたい

期待する回答形式:
- 2案まで（推奨順位つき）
- それぞれの「ホットの命令数/ロード数」見立て
- 変更ファイルと主要関数（どこをどう変えるか）
- A/B フラグ（例: `HZ3_TCACHE_INIT_FAST=1`）案

### Q2: dst compare（`dst == my_shard`）の固定費を削る “箱を増やさない” 方法は？

前提:
- local bins split は全体でGO（性能的には残したい）

候補の方向性:
- “local/remote を判定してから push” ではなく、push先の計算を工夫して compare を薄くする
- compare は残すが TLS load を消す（例: my_shard の配置 / TLS model / 依存鎖短縮）

### Q3: dist=app の gap が “tail主犯なし” なのに大きい矛盾の説明

S30の結果から、サイズレンジの単独では差が小さいのに、app全体で差が大きい。
この矛盾が「混在固定費」「init check」「dst compare」で説明できるかを言語化してほしい。

### Q4: GO/NO-GO の測定セット（最小）

提案したS32施策を判定するための “最小” のベンチを指定してほしい:
- dist=app（RUNS=30）
- uniform（RUNS=30）
- tiny-only（RUNS=30）
に加えて、必要なら何を追加するか（例: 257–2048 only）。

---

## 7) 参考（distベンチの分布）

`bench_random_mixed_malloc_dist` の `app` は固定 80/15/5:
- 16–256: 80%
- 257–2048: 15%
- 2049–max: 5%

実装:
- `hakozuna/bench/bench_random_mixed_malloc_dist.c`
