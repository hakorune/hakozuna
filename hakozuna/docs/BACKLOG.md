# hz3 Backlog（再測定 / TODO）

## 再測定（計測手順の修正後）

- S28-4（LTO A/B）を、`run_bench_hz3_ssot.sh` の `HZ3_MAKE_ARGS` 対応後の手順で再測定して結果を固定する。
  - 対象: `hakozuna/hz3/docs/PHASE_HZ3_S28_4_LTO_AB_WORK_ORDER.md`
  - 手順: `RUNS=30` + seed固定、`HZ3_MAKE_ARGS="HZ3_LTO=0/1"` で SSOT 実行
  - 出力: `/tmp/hz3_ssot_*` ログパス、median表、GO/NO-GO判定

- S38-1（bin lazy count）の A/B を、`HZ3_LDPRELOAD_DEFS_EXTRA` で再測定して確定する。
  - 背景: `HZ3_LDPRELOAD_DEFS="-D<差分だけ>"` で既定プロファイルが落ち、`HZ3_ENABLE=0` になって測定が壊れる事故が起きた。
  - 対象: `hakozuna/hz3/docs/PHASE_HZ3_S38_1_BIN_LAZY_COUNT_WORK_ORDER.md`
  - 手順: `RUNS=30` + seed固定、`HZ3_LDPRELOAD_DEFS_EXTRA="-DHZ3_BIN_LAZY_COUNT=0/1"` で SSOT 実行

## 次フェーズ（lane分離 + 32threads scale）

- S41: hz3-scale（32threads）を導入する（fast lane を死守しつつ、TLSをshards非依存にする）
  - 対象: `hakozuna/hz3/docs/PHASE_HZ3_S41_SCALE_LANE_SPARSE_REMOTE_STASH_WORK_ORDER.md`
  - ✅ Step 1-3 完了（fast/scale `.so` 分離、sparse ring 実装、scale lane `HZ3_NUM_SHARDS=32`）
  - ✅ Step 4: MT ベンチ確定（small-range/mixed-range とも GO）
    - 実装サマリ: `hakozuna/hz3/docs/PHASE_HZ3_S41_IMPLEMENTATION_STATUS.md`
    - ST 結果: `hakozuna/hz3/docs/PHASE_HZ3_S41_STEP2_RESULTS.md`
    - 指示書: `hakozuna/hz3/docs/PHASE_HZ3_S41_STEP4_MT_REMOTE_BENCH_WORK_ORDER.md`
    - 注意: mixed-range は scale lane を **16GB arena** で測定（4GB arena だと segment 枯渇）
    - 4GB arena 固定で成立させる案は別トラックで検討

## 次フェーズ（remote-heavy small gap）

- S42: Small Transfer Cache Box（central負担の削減）
  - 指示書: `hakozuna/hz3/docs/PHASE_HZ3_S42_SMALL_TRANSFER_CACHE_WORK_ORDER.md`
  - 目的: T=32/R=50/16–2048 の gap を詰める（hot path は汚さない）

- S44-4: OwnerStashPopBatch MicroOptBox（既定改善）
  - ✅ 完了（2026-01-13）:
    - `HZ3_S44_4_EARLY_PREFETCH=1` を scale lane 既定に採用（r90 +9.2% / r50 +2.1%）
    - `HZ3_S44_4_QUICK_EMPTY_USE_EPF_HINT=1` + `HZ3_S44_4_WALK_PREFETCH_UNCOND=1` を scale lane 既定に採用（r90 +2.8% / r50 ±0.0% / R=0 +5.7%）
  - 指示書/結果: `hakozuna/hz3/docs/PHASE_HZ3_S44_4_OWNER_STASH_POP_BATCH_MICROOPT_WORK_ORDER.md`

- S43: Small Borrow Box（alloc miss 時に remote stash から借りる）
  - **NO-GO**（設計不一致: TLS stash は借り先として不適合）
  - アーカイブ: `hakozuna/hz3/archive/research/s43_small_borrow_box/README.md`

- S44: OwnerSharedStashBox（owner 別共有 stash の新設計）
  - 目的: S43 の代替。owner 宛の戻りを直接参照できる箱を検討
  - ChatGPT Pro 相談用の質問パックを作成してから着手
  - 指示書: `hakozuna/hz3/docs/PHASE_HZ3_S44_OWNER_SHARED_STASH_WORK_ORDER.md`
  - 質問パック: `hakozuna/hz3/docs/QUESTION_PACK_S44_OWNER_SHARED_STASH.md`
  - **SOFT GO**（+8.8% / 退行なし、scale lane でデフォルト有効化）

- S44-2: OwnerSharedStash パラメータ調整
  - 目的: `T=32 R=50 16–2048` を +10% 以上に押し上げる
  - ノブ: `HZ3_S44_DRAIN_LIMIT`, `HZ3_S44_STASH_MAX_OBJS`

## Post-parity（tcmalloc 追従後）

- S45: Learning/Policy Box（省メモリ・省エネ）
  - 条件: tcmalloc parity 後に着手（hot path は汚さない）
  - 方針: event-only の policy box で bin/transfer/owner stash の上限や trim を制御
  - デフォルト: FROZEN（学習OFF、固定ポリシー）
  - いつかやる（実ワーク向け）:
    - “ベンチ常勝” ではなく “ワークロードの相（remote-heavy等）に合わせてルート/プリセットを切替” する箱を検討する。
    - 入口は SSOT カウンタ（例: S97 の `saved_calls/entries`、S85 の slow 内訳、S74 の flush/refill stats）を観測し、低頻度に意思決定する（hot path は knob を読むだけ）。
    - 例: S97-1 bucketize は r90 では勝つが r50 では大きく負けるため、将来は実ワークでの相判定で opt-in できる形が良い。

## 省メモリ（RSS削減）トラック

- S53-3: LargeCacheBudgetBox mem-mode 2プリセット（opt-in）
  - ドキュメント: `hakozuna/hz3/docs/PHASE_HZ3_S53_LARGE_CACHE_BUDGET.md`
  - `mem_mstress`（RSS重視）/ `mem_large`（malloc-large速度重視）の 2 lane を提供

- S54: SegmentPageScavengeBox（OBSERVE → FROZEN）
  - OBSERVE: 統計のみ（`madvise` なし）。segment/medium 側に RSS削減ポテンシャルがあるかを切り分ける。
  - ドキュメント: `hakozuna/hz3/docs/PHASE_HZ3_S54_SEGMENT_PAGE_SCAVENGE_OBSERVE.md`
  - OBSERVE 実測: `max_potential_bytes` が ~1MB 級（数MB未満）→ **現状は Phase 2（FROZEN / 実madvise）は NO-GO**（費用対効果が薄い）。

- S55: RetentionPolicyBox（TLS/central/segment 滞留の FROZEN）
  - 目的: tcmalloc 比で大きい RSS gap を「cache 滞留の上限」で詰める（hot は汚さない、event-only）。
  - 方針: まず OBSERVE で “どの層が保持しているか” を 1-shot で可視化し、次に FROZEN で cap/trim を入れる。
  - 指示書: `hakozuna/hz3/docs/PHASE_HZ3_S55_RETENTION_POLICY_BOX_WORK_ORDER.md`
  - OBSERVE 実測（2026-01-06）: `arena_used_bytes`（=segment/slot 使用量 proxy）が 97–99% 支配的。
    - 次: FROZEN は **segment を開き過ぎない/返す**方向（S47/S49/S45境界）を最優先にする。
  - S55-2（FROZEN: OpenDebtGate）: **RSS目的では NO-GO**
    - level 遷移は成立したが、`mstress` の mean RSS は概ね -0〜-1% 程度で目標 -10% 未達。
  - S55-2B（FROZEN: Epoch Repay）: **RSS目的では NO-GO**
    - `mstress` で `gen_delta==0` が継続し、返却が進まない（full-free segment が作れない）。
  - 次フェーズ候補（S55-3）:
    - “full-free segment を作れない断片化”を前提に、別の返却方式（page subrelease / out-of-band freelist 等）を検討。
    - 指示書（案）: `hakozuna/hz3/docs/PHASE_HZ3_S55_3_MEDIUM_RUN_SUBRELEASE_WORK_ORDER.md`
    - 予定: S55-3 を OS PageAdviceBox で hybrid化（Linux/WindowsでAPI差分を薄く閉じる）
    - 現状（2026-01-06）: `mstress` steady-state では RSS -5〜-6% 程度で **目標 -10% 未達（NO-GO）**。
  - S55-3B（遅延 subrelease / purge delay）: **NO-GO（steady-state）/ アーカイブ済み**
    - 指示書/結果: `hakozuna/hz3/docs/PHASE_HZ3_S55_3B_STEP3_NO_GO_AND_NEXT_WORK_ORDER.md`

## 次の本命（RSS本線: segment “開き散らかし”抑制）

- S56: ActiveSegmentSet + Pack Best-Fit（segmentを増やしにくい割当へ）
  - 目的: `arena_used_bytes`（=開いたsegment数 proxy）支配に対して、割当側で散らばりを減らす。
  - 指示書: `hakozuna/hz3/docs/PHASE_HZ3_S56_ACTIVE_SEGMENT_SET_AND_PACK_BESTFIT_WORK_ORDER.md`
  - 2026-01-07: S56-1A（Pack Best-Fit K=2）は “bounded実装ミス” で SSOT が大きく退行 → S56-1B（K候補で即終了）へ修正。
    - S56-1B（暫定）: small/medium は改善〜同等、mixed は +9.8% 退行が残る。
    - RSS は同一ワークロードでの A/B を揃えて再測定が必要（baseline/treatment でベンチが分裂したため）。

## hz3 large 次キュー（hz4 lock 後）

- S238: LargeDirectHeaderLookupBox（planned）
  - 目的: `hz3_large_take()` の hash bucket 走査コストを減らし、large free の固定費を下げる。
  - 境界: `hz3_large_take()` を唯一の変換点にする（Box理論の境界1箇所）。
  - 安全条件:
    - direct path は `magic` と `user_ptr` の一致を必須にする。
    - `hz3_large_aligned_alloc` 由来の pointer は固定 offset 直引きに乗せない（必ず fallback 可）。
    - fallback の hash map 経路を維持し、判定失敗時は即座に戻す。
  - 成功条件（screen）:
    - `cross128` または `malloc-large` で改善（RUNS 固定の median 比較）
    - `guard/main` で明確な退行なし
  - 指示書:
    - `hakozuna/hz3/docs/PHASE_HZ3_S238_LARGE_DIRECT_HEADER_LOOKUP_WORK_ORDER.md`
