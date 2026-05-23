Hakozuna 公開リポジトリ案内 (日本語)
====================================

この公開リポジトリには、安定運用向けの2つのアロケータ実装と、
研究用 sidecar が含まれています。

- hakozuna/     : hz3 (local-heavy 向け)
- hakozuna-mt/  : hz4 (remote-heavy / 高スレッド向け)
- hakozuna-hz5/ : HZ5 (exact over-aligned profile 向け研究 sidecar)

対応プラットフォーム
--------------------

- Ubuntu/Linux の公開エントリポイント: linux/
- Ubuntu/Linux arm64 の明示的な入口: linux/build_linux_arm64_release_lane.sh
- Windows native の公開エントリポイント: win/
- macOS の公開エントリポイント: mac/
- Windows の build / bench ガイド: docs/WINDOWS_BUILD.md
- HZ5 Linux exact profile runner: linux/run_linux_hz5_local2p_focus.sh

推奨プロファイル選択
--------------------

- 迷ったら hz3 を使う
- Redis系・local-heavy ワークロードは hz3
- cross-thread free が多い remote-heavy ワークロードは hz4
- HZ5 は既定 allocator ではなく、64K/a8192 exact profile の研究・付録行として扱う

最新版の結果と論文
------------------

- ベンチ結果: docs/benchmarks/2026-02-18_PAPER_BENCH_RESULTS.md
- Ubuntu/Linux arm64 の結果: docs/benchmarks/2026-03-21_LINUX_ARM64_PRELOAD_OWNERSHIP_FIX_RESULTS.md
- 公開PDF（英語）: docs/paper/main_en.pdf
- 公開PDF（日本語）: docs/paper/main_ja.pdf
- ローカル論文ワークスペース: private/paper/
- 公開PDFは現在も v3.2 時点の論文改訂を指します
- v3.3 は Linux arm64 対応と ownership-routing バグ修正を中心にした
  source / artifact release です
- 最新の Zenodo アーカイブ（v3.3）: https://zenodo.org/records/19139939
- DOI（v3.3）: https://doi.org/10.5281/zenodo.19139939

HZ5 Linux 付録プロファイル
--------------------------

現時点の HZ5 Linux は、64K allocation / align=8192 の exact-overaligned
条件に限定した付録プロファイルとして扱います。

- hz5-local2p-linkflags: low-final-RSS local/mixed speed profile
- hz5-local2p-rssretain2048tls: retained-cache RSS-throughput profile
- hz5-local2p-remotebatch: producer/consumer remote-free profile

これらは用途別 profile であり、HZ5 単体が一般用途 allocator として
全条件を支配する、という主張にはしません。

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
