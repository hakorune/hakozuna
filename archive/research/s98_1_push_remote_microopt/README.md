# S98-1 push_remote_list micro-opt（NO-GO / archive）

目的:
- `hz3_small_v2_push_remote_list()`（remote-heavy で ~8% 級）周辺の固定費を削って改善を狙う。
- 低リスクの micro-opt（init fastpath / branch predict hint）で r50 を壊さないことが条件。

試したもの（本線からは撤去済み）:
- `HZ3_S98_OWNER_STASH_INIT_FASTPATH`
  - `hz3_owner_stash_init()` で `pthread_once()` を “init済みなら atomic load でスキップ”。
- `HZ3_S98_PUSH_REMOTE_S44_LIKELY`
  - `hz3_small_v2_push_remote_list()` で S44 push 成功を `__builtin_expect(...,1)` で示す。

結果:
- RUNS=10 と RUNS=20 で改善/退行が反転しうる（ノイズ/時間ドリフト/環境要因の影響が大きい）。
- r50 側の退行が出るケースもあり、scale 既定へ入れる “確勝” が取れなかった。
- よって **NO-GO（scale既定には入れない）**。

関連資料:
- 指示書: `hakozuna/hz3/docs/PHASE_HZ3_S98_PUSH_REMOTE_LIST_SSOT_AND_MICROOPT_WORK_ORDER.md`
- NO-GO 台帳: `hakozuna/hz3/docs/NO_GO_SUMMARY.md`

備考（再挑戦するなら）:
- まずは “測定導線” を固定（CPU pin / 長尺 iters / interleave / 複数回の再現）してから再評価する。
- “branch hint 系” は微小差分になりやすく、ベンチの揺らぎに埋もれやすい前提で扱う。

