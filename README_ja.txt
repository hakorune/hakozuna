Hakozuna 公開リポジトリ案内 (日本語)
====================================

この公開リポジトリには、2つのアロケータ実装が含まれています。

- hakozuna/     : hz3 (local-heavy 向け)
- hakozuna-mt/  : hz4 (remote-heavy / 高スレッド向け)

対応プラットフォーム
--------------------

- Ubuntu/Linux の公開エントリポイント: linux/
- Ubuntu/Linux arm64 の明示的な入口: linux/build_linux_arm64_release_lane.sh
- Windows native の公開エントリポイント: win/
- macOS の公開エントリポイント: mac/
- Windows の build / bench ガイド: docs/WINDOWS_BUILD.md

推奨プロファイル選択
--------------------

- 迷ったら hz3 を使う
- Redis系・local-heavy ワークロードは hz3
- cross-thread free が多い remote-heavy ワークロードは hz4

最新版の結果と論文
------------------

- ベンチ結果: docs/benchmarks/2026-02-18_PAPER_BENCH_RESULTS.md
- Ubuntu/Linux arm64 の結果: docs/benchmarks/2026-03-21_LINUX_ARM64_PRELOAD_OWNERSHIP_FIX_RESULTS.md
- 公開PDF（英語）: docs/paper/main_en.pdf
- 公開PDF（日本語）: docs/paper/main_ja.pdf
- ローカル論文ワークスペース: private/paper/
- Zenodo v3.2: https://zenodo.org/records/19120414
- DOI: https://doi.org/10.5281/zenodo.19120414

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
