# HZ4 Phase 15: RSSReturnBox（epoch + batch + hysteresis）指示書

目的: hz4 の RSS lane を「確実に返すが、thrash しない」設計に寄せ、mimalloc級の RSS release に近づける。

箱理論（Box Theory）
- **箱**: `RSSReturnBox`
- **境界1箇所**: `hz4_collect()` の epoch 境界（= owner thread の定期点検）だけで OS 返却を実行
- **戻せる**: `HZ4_RSSRETURN=0/1`（default OFF）
- **見える化**: 1行サマリ（カウンタ）+ SSOT（R=90 の ru_maxrss median）
- **Fail-Fast**: 整合性違反は abort（decommit page access 等の既存 failfast を維持）

---

## 0) 前提（現状）

- hz4 は meta separation + decommit 安全化（purge before decommit）+ delay queue まで実装済み。
- 課題は “返し方” のアルゴリズム:
  - empty event を拾う
  - すぐ返しすぎない（hysteresis）
  - まとめて返す（batch）

---

## 1) 追加フラグ（A/B）

ファイル: `hakozuna/hz4/core/hz4_config.h`

提案:
- `HZ4_RSSRETURN=0/1`（default OFF）
- `HZ4_RSSRETURN_BATCH_PAGES=8`（1 epoch で返す上限）
- `HZ4_RSSRETURN_THRESH_PAGES=32`（owner/shard 単位で溜めるしきい値）

---

## 2) データ構造（owner-thread only queue）

ファイル候補:
- `hakozuna/hz4/core/hz4_tls.h`（TLS に owner-only queue を置く）

最小:
- `empty_pages_head/tail`（SPSC, owner thread only）
- `empty_pages_count`（hysteresis 用）

注意:
- Remote/MPSC は不要（empty になるのは owner の統合点で判定する）

---

## 3) empty 判定の入口（conversion point = 1 箇所）

場所:
- `hz4_collect` の drain 終了点、または `hz4_page_used_dec` の 0 遷移後（ただし hot path 汚染は禁止）

方針:
- hot path（local free）は触らない
- `collect/refill` 境界で “used_count==0 かつ remote_head==NULL” を確認して empty queue に積む

---

## 4) epoch で batch madvise（RSSReturnBox）

場所:
- `hz4_collect()` の先頭 or 末尾に `rssreturn_tick(tls)` を挿入（境界1箇所）

処理:
- `empty_pages_count >= THRESH` のときだけ処理開始
- 最大 `BATCH` ページだけ `madvise(DONTNEED)`（or 既存 decommit API を呼ぶ）
- 処理したページは queue から外す

---

## 5) SSOT（R=90, Peak RSS）

ベースライン（RSS lane）:
```sh
make -C hakozuna/hz4 clean all \
  HZ4_DEFS_EXTRA='-DHZ4_PAGE_META_SEPARATE=1 -DHZ4_PAGE_DECOMMIT=1 -DHZ4_DECOMMIT_DELAY_QUEUE=1 -DHZ4_TCACHE_PURGE_BEFORE_DECOMMIT=1'
```

RSSReturnBox ON:
```sh
make -C hakozuna/hz4 clean all \
  HZ4_DEFS_EXTRA='-DHZ4_PAGE_META_SEPARATE=1 -DHZ4_PAGE_DECOMMIT=1 -DHZ4_DECOMMIT_DELAY_QUEUE=1 -DHZ4_TCACHE_PURGE_BEFORE_DECOMMIT=1 -DHZ4_RSSRETURN=1'
```

計測（例: ru_maxrss を /usr/bin/time -v で 3 runs median）:
```sh
for i in 1 2 3; do
  env -i PATH="$PATH" HOME="$HOME" \
    LD_PRELOAD="$PWD/hakozuna/hz4/libhakozuna_hz4.so" \
    /usr/bin/time -v \
    ./hakozuna/out/bench_random_mixed_mt_remote_malloc 16 2000000 400 16 2048 90 65536 \
    2> /tmp/hz4_rssreturn_time_${i}.txt
done
```

判定:
- GO: Peak RSS が安定に下がる（median -10% 以上など）かつ ops/s が -5% 以内
- NO-GO: RSS が下がらない or ops/s が大きく落ちる

