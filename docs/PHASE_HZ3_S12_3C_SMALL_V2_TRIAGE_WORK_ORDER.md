# PHASE_HZ3_S12-3C: Small v2（self-describing）トリアージ指示書

目的:
- `PHASE_HZ3_S12_3A_SMALL_V2_AB.md` の NO-GO（全面退行）の原因を切り分ける。
- v2 の「segmap不要」方向性を捨てる/育てるを、**計測に基づいて**決める。

前提（混同防止）:
- v1/v2 は **hz3 の small 実装**（Small v1 / Small v2）のこと。
- hakozuna 本線（`hakozuna/`）の small 実装とは別。

参照:
- A/B SSOT: `hakozuna/hz3/docs/PHASE_HZ3_S12_3A_SMALL_V2_AB.md`
- triage plan: `hakozuna/hz3/docs/PHASE_HZ3_S12_3B_SMALL_V2_TRIAGE_PLAN.md`

---

## 成功条件（GO/NO-GO）

GO（段階的）:
1. medium/mixed が v1 近傍に戻る（v2 ON の退行が ±0% 以内）
2. small の退行が半減（例: -15% → -7% 以内）

NO-GO:
- 1,2 を満たせない場合は v2 を研究箱として凍結（default OFF 維持）し、v1 を育てる。

---

## 実装タスク（1 commit = 1 purpose）

### Task 1: v2 判定の実態をカウントする（stats、hot には常時ログを入れない）

狙い:
- medium/mixed 退行が「v2判定が毎回走ってる」せいか確認する。

やること:
- `hakozuna/hz3/include/hz3_config.h` に `HZ3_S12_V2_STATS` を追加（default 0）。
- `HZ3_S12_V2_STATS=1` のときだけ、以下の counters を TLS か global に積む（atomicは不要、exit dumpでOK）。
  - `s12_v2_seg_from_ptr_calls`
  - `s12_v2_seg_from_ptr_hit`（hz3 arena 内だった）
  - `s12_v2_seg_from_ptr_miss`（arena 外だった）
  - `s12_v2_small_v2_enter`（Small v2 本体に入った）
- `atexit` で 1 行（または数行）ダンプする（`HZ3_SHIM_FORWARD_ONLY=0` でのみ）。

注意:
- hot path の命令列を汚さないため、`HZ3_S12_V2_STATS=0` の時は **完全に消える**よう `#if` で囲う。

### Task 2: hot path から `pthread_once` を除去する（arena API を fast/safe に分割）

狙い:
- v2 ON 時に `free()` で `pthread_once()` を踏んでいたら即死するので、slow 側に隔離する。

やること:
- `hakozuna/hz3/include/hz3_arena.h` を分割:
  - `hz3_arena_init_slow()`（`pthread_once` を内包してOK）
  - `hz3_arena_contains_fast(ptr)`（**pthread_once を呼ばない**）
    - 未初期化なら `false` を返す（false negative OK、フォールバックに落ちるだけ）。
    - 例: `if (g_arena_base == NULL) return false;` のような早期return。
- `hz3_seg_from_ptr()` / `hz3_free()` 側は `*_fast` を使う（slow init 呼びは禁止）。
- slow 側（segment確保、tcache ensure_init 等）で `hz3_arena_init_slow()` を必ず呼ぶ。

### Task 3: v2 free の “メモリタッチ” を減らす（優先度: 中）

狙い:
- v2 small が v1 より遅い原因が「冷えやすい load の増加」なら、読む箇所を減らす。

候補:
- `page header` に owner を持たせて、free が seg hdr を読まない（seg hdr は debug/slow のみ）。
- seg hdr の magic/owner 検証を debug/slow に寄せ、release/hot では range+used のみ（fail-fastは別箱で）。

### Task 4: Arena PageTagMap Box（勝ち筋候補 / 1-load ptr→(sc,owner,k ind)）

狙い:
- small v2 の hot path から `used[idx]` / `page_hdr` を追放し、`range check + tag load` に寄せる。
- medium/mixed への固定費漏れも “miss 側は比較だけ” に縮める。

参照:
- 設計: `hakozuna/hz3/docs/PHASE_HZ3_S12_4_SMALL_V2_PAGETAGMAP_PLAN.md`

---

## ビルド / ベンチ（SSOT）

### A/B build（例）

v1:
```bash
make -C hakozuna/hz3 clean
CFLAGS="-O3 -fPIC -Wall -Wextra -Werror -std=c11 \
  -DHZ3_ENABLE=1 -DHZ3_SHIM_FORWARD_ONLY=0 \
  -DHZ3_SMALL_V2_ENABLE=0 -DHZ3_SEG_SELF_DESC_ENABLE=0" \
  make -C hakozuna/hz3 all_ldpreload
```

v2:
```bash
make -C hakozuna/hz3 clean
CFLAGS="-O3 -fPIC -Wall -Wextra -Werror -std=c11 \
  -DHZ3_ENABLE=1 -DHZ3_SHIM_FORWARD_ONLY=0 \
  -DHZ3_SMALL_V2_ENABLE=1 -DHZ3_SEG_SELF_DESC_ENABLE=1" \
  make -C hakozuna/hz3 all_ldpreload
```

SSOT run:
```bash
SKIP_BUILD=1 RUNS=10 ITERS=20000000 WS=400 \
  ./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh
```

---

## 成果物（更新/追記）

- 結果（A/B + counters）を `hakozuna/hz3/docs/PHASE_HZ3_S12_3A_SMALL_V2_AB.md` に追記（同一条件のみ）。
- v2 を育てない場合でも、**default OFF のまま**コード/ドキュメントを残し、研究箱として扱う。
