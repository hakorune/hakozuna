Hakozuna 公開リポジトリ案内 (日本語)
====================================

この公開リポジトリには、安定運用向けの2つのアロケータ実装と、
研究用 sidecar が含まれています。

- hakozuna/     : hz3 (local-heavy 向け)
- hakozuna-mt/  : hz4 (remote-heavy / 高スレッド向け)
- hakozuna-hz5/ : HZ5 (低 RSS / fail-closed / descriptor-owned profile family 向け研究 sidecar)

HZ6 は現時点では future work の名前です。より広い tcmalloc-like な
class-transfer throughput を追う場合の transfer-first 後継線として扱います。

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
- HZ5 を含む MT snapshot: RUNS=10 / T=16 / Ubuntu native / 2026-05-26
- Ubuntu/Linux arm64 の結果: docs/benchmarks/2026-03-21_LINUX_ARM64_PRELOAD_OWNERSHIP_FIX_RESULTS.md
- 公開PDF（英語）: docs/paper/main_en.pdf
- 公開PDF（日本語）: docs/paper/main_ja.pdf
- ローカル論文ワークスペース: private/paper/
- 最新の hz3/hz4 Zenodo アーカイブ（v3.4）:
  https://zenodo.org/records/20411402
- hz3/hz4 v3.4 の DOI:
  https://doi.org/10.5281/zenodo.20411402
- hz3/hz4 ACE-Alloc artifact series 全体の DOI:
  https://doi.org/10.5281/zenodo.18305952
- HZ5 Zenodo アーカイブ（3.5-hz5）:
  https://zenodo.org/records/20411598
- HZ5 3.5-hz5 の DOI:
  https://doi.org/10.5281/zenodo.20411598
- HZ5 sidecar allocator series 全体の DOI:
  https://doi.org/10.5281/zenodo.20411597

代表 MT snapshot
----------------

同じマシン・同じ runner で取り直した RUNS=10 / T=16 の代表表です。
HZ5 は単一既定 profile ではなく profile family なので、Best HZ5 と採用 row を分けて示します。

| Lane | hz3 | hz4 | mimalloc | tcmalloc | Best HZ5 | HZ5 row |
|------|-----|-----|----------|----------|----------|---------|
| main_r0 | 292.15M | 85.63M | 146.73M | 318.82M | 157.44M | hz5-pagerun64-main |
| main_r50 | 31.46M | 62.32M | 14.26M | 64.87M | 79.43M | hz5-large128-transfer128 |
| main_r90 | 22.31M | 67.14M | 7.72M | 45.42M | 62.31M | hz5-pagerun64-cross128 |
| guard_r0 | 318.98M | 156.68M | 258.19M | 375.71M | 149.00M | hz5-pagerun64-main |
| cross128_r90 | 2.78M | 27.66M | 3.52M | 7.21M | 22.39M | hz5-large128-transfer128 |

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

HZ6 future branch
-----------------

HZ6 は future work としての transfer-first 設計案です。

- class transfer cache を owner inbox 上の診断 lane ではなく、最初から主要な箱として扱う
- RSS governor を control plane の一部にする
- 学習 / policy layer は malloc/free hot path に入れない
- strict-safety profile と speed profile の契約分離が必要かを検証する

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
