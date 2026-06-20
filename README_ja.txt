Hakozuna 公開リポジトリ案内 (日本語)
====================================

この公開リポジトリには、安定運用向けの2つのアロケータ実装と、
研究用 allocator family が含まれています。

- hakozuna/     : hz3 (local-heavy 向け)
- hakozuna-mt/  : hz4 (remote-heavy / 高スレッド向け)
- hakozuna-hz5/ : HZ5 (低 RSS / fail-closed / descriptor-owned profile family 向け研究 sidecar)
- hakozuna-hz6/: HZ6 (route safety / 明示的 ownership state / speed-RSS profile lane を持つ selected-family prototype)

allocator profile map
---------------------

Hakozuna には、metadata と ownership の流し方が違う4つの allocator line があります。

| Line | Focus | Metadata / routing model | 読み方 |
|------|-------|--------------------------|--------|
| HZ3 / ACE-Alloc | local-heavy allocation / compact fast path | lookup-first: PTAG32 / table-oriented pointer-to-bin routing | ACE-Alloc の本線 |
| HZ4 | remote-heavy / message-passing workload | remote-free-first: page-local metadata / remote queue / pending collect | remote-free 実験線 |
| HZ5 | page/run-first sidecar allocator prototype | ownership/policy-first: page/run descriptor が owner / profile / dispatch policy を決める | 低 RSS / fail-closed 研究線 |
| HZ6 | speed/RSS balance と明示的 safety contract | RouteLayer + descriptor + SourceLayer + FrontCache | selected-family 後継線 |

短く言うと:

- HZ3 は lookup-first。
- HZ4 は remote-free-first。
- HZ5 は ownership/policy-first。
- HZ6 は contract-first で、selected/default lane と profile-only lane を分ける。

同じ malloc/free API でも、free(ptr) のときに pointer の正体をどう復元するか、
そして ownership をどこへ流すかで allocator の性格は大きく変わります。

対応プラットフォーム
--------------------

- Ubuntu/Linux の公開エントリポイント: linux/
- Ubuntu/Linux arm64 の明示的な入口: linux/build_linux_arm64_release_lane.sh
- Windows native の公開エントリポイント: win/
- macOS の公開エントリポイント: mac/
- Windows の build / bench ガイド: docs/WINDOWS_BUILD.md
- HZ5 Linux exact profile runner: linux/run_linux_hz5_local2p_focus.sh
- HZ5 Linux general profile runner: linux/run_hz5_hakmem_compare.sh

推奨プロファイル選択
--------------------

- 迷ったら hz3 を使う
- Redis系・local-heavy ワークロードは hz3
- cross-thread free が多い remote-heavy ワークロードは hz4
- HZ5 は既定 allocator ではなく、低 RSS / fail-closed な研究 profile family として扱う

最新版の結果と論文
------------------

- ベンチ結果: docs/benchmarks/2026-02-18_PAPER_BENCH_RESULTS.md
- HZ5/HZ6 を含む MT snapshot: RUNS=10 / T=16 / Ubuntu native /
  2026-05-26 and 2026-06-21
- Ubuntu/Linux arm64 の結果: docs/benchmarks/2026-03-21_LINUX_ARM64_PRELOAD_OWNERSHIP_FIX_RESULTS.md
- 公開PDF（英語）: docs/paper/main_en.pdf
- 公開PDF（日本語）: docs/paper/main_ja.pdf
- ローカル論文ワークスペース: private/paper/
- 最新の hz3/hz4 Zenodo アーカイブ（v3.4）:
  https://zenodo.org/records/20753903
- hz3/hz4 v3.4 の DOI:
  https://doi.org/10.5281/zenodo.20753903
- hz3/hz4 ACE-Alloc artifact series 全体の DOI:
  https://doi.org/10.5281/zenodo.18305952
- HZ5 Zenodo アーカイブ（3.5-hz5）:
  https://zenodo.org/records/20753950
- HZ5 3.5-hz5 の DOI:
  https://doi.org/10.5281/zenodo.20753950
- HZ5 sidecar allocator series 全体の DOI:
  https://doi.org/10.5281/zenodo.20411597
- HZ6 Zenodo アーカイブ:
  https://zenodo.org/records/20753968
- HZ6 の DOI:
  https://doi.org/10.5281/zenodo.20753968
- HZ6 selected-family allocator series 全体の DOI:
  https://doi.org/10.5281/zenodo.20753967

代表 MT snapshot
----------------

同じマシンの Ubuntu-native MT snapshot です。HZ5 は単一既定 profile ではなく
profile family なので、Best HZ5 と採用 row を分けて示します。HZ6 は
`2026-06-21_LINUX_X86_64_HZ6_REMOTE_ALLOCATOR_COMPARE_FULL_R10.md` の
full allocator frontier run から追加し、`guard_r0` は同じマシン・同じ runner の
selected-row rerun で補完しています。この表での HZ6 は throughput winner ではなく、
RSS が低く平坦な production line として読むのが安全です。

| Lane | hz3 | hz4 | mimalloc | tcmalloc | Best HZ5 | HZ6 |
|------|-----|-----|----------|----------|----------|-----|
| main_r0 | 292.15M | 85.63M | 146.73M | 318.82M | 157.44M | 16.88M |
| main_r50 | 31.46M | 62.32M | 14.26M | 64.87M | 79.43M | 15.08M |
| main_r90 | 22.31M | 67.14M | 7.72M | 45.42M | 62.31M | 10.99M |
| guard_r0 | 318.98M | 156.68M | 258.19M | 375.71M | 149.00M | 189.48M |
| cross128_r90 | 2.78M | 27.66M | 3.52M | 7.21M | 22.39M | 6.38M |

profile-selection notes:

- HZ5 採用 row: main_r0 と guard_r0 は hz5-pagerun64-main、
  main_r50 と cross128_r90 は hz5-large128-transfer128、main_r90 は
  hz5-pagerun64-cross128。
- HZ6 採用 row: main_r0/main_r50/main_r90 は HZ6 full R10 の
  local0/remote50/remote90、guard_r0 は selected-row guard rerun、
  cross128_r90 は full R10 の cross128_r90。
- HZ6 peak RSS median: main_r0 67.38 MiB、main_r50 69.50 MiB、
  main_r90 72.07 MiB、guard_r0 65.88 MiB、cross128_r90 68.91 MiB。

HZ5 Linux profile family
------------------------

HZ5 Linux は、現在は2つの紙面向け範囲を持ちます。

1. 明示 route 特化の証拠としての exact Local2P 付録行:

- hz5-local2p-linkflags: low-final-RSS local/mixed speed profile
- hz5-local2p-rssretain2048tls: retained-cache RSS-throughput profile
- hz5-local2p-remotebatch: producer/consumer remote-free profile

2. 低 RSS / fail-closed / descriptor-owned allocation を見る Linux full-preload general profile:

- hz5-pagerun64-main / hz5-pagerun64-cross128: MidPage PageRun64 profile
- hz5-large128-rss: 低 RSS LargeFront profile
- hz5-large128-source16: LargeFront 128K throughput 比較 lane
- hz5-large128-transfer128: transfer-cache 診断 lane

HZ5 は profile family として扱います。mid/main/cross の remote-pressure 行では
tcmalloc より大幅に低い RSS で強い行がありますが、hz3/hz4 や tcmalloc を単一 profile で
広く置き換えるという主張にはしません。

HZ6 Windows selected-family snapshot
------------------------------------

HZ6 は Windows-native selected-family runner と profile-specific lane で測っているため、
Ubuntu MT 表とは分けて示します。

| Lane | Selected HZ6 row | ops/s | Peak RSS |
|------|------------------|------:|---------:|
| random_mixed small | sameownerfast-descavail-noboost-route4k | 45.755M | 4,968 KB |
| random_mixed medium | sameownerfast-descavail-noboost-route4k | 42.408M | 4,964 KB |
| random_mixed mixed | sameownerfast-descavail-noboost-route4k | 41.306M | 4,964 KB |
| mixed_ws balanced | mixedclean-front16k-sourcerun-desc17k-source2k-route17k-linearwrap-loopcarry | 66.922M | 111,244 KB |
| mixed_ws wide_ws | mixedclean-front16k-sourcerun-desc17k-source2k-route17k-linearwrap-loopcarry | 21.853M | 140,708 KB |

selected-small では SourceBlockRoute dynmap が balanced (+8.46%) や
large_slice_16k (+16.48%) で改善し、selected-family / profile-only の分離は維持します。

HZ6 selected-family branch
--------------------------

HZ6 は selected-family allocator prototype です。

- route safety / descriptor ownership / SourceLayer / FrontCache contract を主要設計として扱う
- selected/default lane と profile-only lane は意図的に分離する
- RSS governance と speed/RSS balance profile を評価対象にする
- ベンチ結果は workload / platform 固有の evidence であり、普遍的な allocator ranking ではない
- 現在の設計メモは hakozuna-hz6/ に置く

最小実行例
----------

1) hz3

cd hakozuna/hz3
make clean all_ldpreload_scale
LD_PRELOAD=./libhakozuna_hz3_scale.so ./your_app

2) hz4

cd ../hz4
make clean all
LD_PRELOAD=./libhakozuna_hz4.so ./your_app

補足
----

- プロファイル切替の詳細は PROFILE_GUIDE.md を参照
- 公開情報の起点は README.md
- Linux の arm64 は x86_64 と分けて扱う
- macOS は別レーンとして扱う
