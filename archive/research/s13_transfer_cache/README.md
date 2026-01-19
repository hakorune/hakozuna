# S13: TransferCache（NO-GO）

目的:
- tcmalloc との差が残る `medium/mixed` の shared path 固定費を削る。
- hot path（TLS bin push/pop）を一切触らず、slow/event 側だけで改善する。

参照（設計と指示書）:
- 設計: `hakozuna/hz3/docs/PHASE_HZ3_S13_TRANSFERCACHE_DESIGN.md`
- 指示書: `hakozuna/hz3/docs/PHASE_HZ3_S13_TRANSFERCACHE_WORK_ORDER.md`

実装（今回やったこと）:
- `TransferCache`（per-(shard,sc)）を追加し、alloc slow path で pop を試す。
- epoch の bin trim で push を試す（full 時は central fallback）。
- compile-time flags で A/B:
  - `-DHZ3_TC_ENABLE=0/1`
  - `-DHZ3_TC_STATS=0/1`

結果（SSOTの要約）:
- `small` が -1〜-2% 程度の退行傾向
- `medium/mixed` は ±0% 程度で、目標の +2% を達成できず
- 判定: **NO-GO**

観測（なぜ効かなかったか）:
- MT-remote (T=4, R=50, RUNS=1) の stats で、TC が実質使われていないことを確認:
  - ログ: `/tmp/ssot_all_tc_stats_20260101_121937`
  - `[hz3] TC stats: pop_calls=5 pop_hit=0 pop_miss=5 push_calls=0 push_ok=0 push_full=0`
- 解釈:
  - pop は呼ばれるが **hit が 0**
  - push が **0** なので、TC に入る流量が無い
  - この実装だと「epoch trim が発生したときだけ push」なので、SSOT/remote 条件では TC が枯渇しやすい
  - 結果として shared path の固定費削減に繋がらず、チェック/分岐の固定費だけが残る

対応:
- S13 TransferCache 実装コードは本線から除去（“研究箱は残さない” ルールに従う）。
- 設計/指示書 docs は残し、再挑戦時の入口にする。

復活条件（やるなら）:
- TC への流入点を増やす（hotを触らず、event/slowだけで）:
  - `hz3_central_push_list()` 入口で「TC push を先に試す」など
  - inbox drain overflow / segment recycle など、実際に “余り” が出る場所から供給する
- それでも “層が厚くなるだけ” なら潔く NO-GO 維持

