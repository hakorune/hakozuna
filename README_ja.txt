Hakozuna 公開リポジトリ案内 (日本語)
====================================

この公開リポジトリには、安定運用向けのアロケータ実装と、
研究用 allocator family が含まれています。

- hakozuna/     : hz3 (local-heavy 向け)
- hakozuna-mt/  : hz4 (remote-heavy / 高スレッド向け)
- hakozuna-hz5/ : HZ5 (低 RSS / fail-closed / descriptor-owned profile family 向け研究 sidecar)
- hakozuna-hz6/: HZ6 (route safety / 明示的 ownership state / speed-RSS profile lane を持つ selected-family prototype)
- hakozuna-hz8/: HZ8 (低い post-workload RSS / fail-closed ownership / KeepRefill / preload hardening を持つ推奨 balanced allocator line)
- hakozuna-hz10/: HZ10 (macro/shim speed research candidate。公開推奨 line はまだ HZ8)

allocator profile map
---------------------

Hakozuna には、metadata と ownership の流し方が違う allocator line があります。

| Line | Focus | Metadata / routing model | 読み方 |
|------|-------|--------------------------|--------|
| HZ3 / ACE-Alloc | local-heavy allocation / compact fast path | lookup-first: PTAG32 / table-oriented pointer-to-bin routing | ACE-Alloc の本線 |
| HZ4 | remote-heavy / message-passing workload | remote-free-first: page-local metadata / remote queue / pending collect | remote-free 実験線 |
| HZ5 | page/run-first sidecar allocator prototype | ownership/policy-first: page/run descriptor が owner / profile / dispatch policy を決める | 低 RSS / fail-closed 研究線 |
| HZ6 | speed/RSS balance と明示的 safety contract | RouteLayer + descriptor + SourceLayer + FrontCache | selected-family 後継線 |
| HZ8 | 推奨 balanced line | fail-closed ownership + owner-stable remote free + KeepRefill pressure control | 現在の公開 allocator line |
| HZ10 | macro/shim speed research | page-based fail-closed substrate + preload shim + orphan adoption work | speed-oriented な研究候補 |

短く言うと:

- HZ3 は lookup-first。
- HZ4 は remote-free-first。
- HZ5 は ownership/policy-first。
- HZ6 は contract-first で、selected/default lane と profile-only lane を分ける。
- HZ8 は通常利用時に最初に選ぶ balanced allocator line。
- HZ10 は新しい macro/shim 研究候補だが、公開推奨 line はまだ HZ8。

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

- 現在の公開 line で迷ったら HZ8 から試す
- hz3/hz4/HZ5/HZ6 は設計系譜とベンチ比較基準として扱う
- HZ8 は tcmalloc を全面的に置き換える主張ではなく、balanced low-RSS line として扱う

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
- HZ8 論文 Zenodo アーカイブ:
  https://zenodo.org/records/21084279
- HZ8 論文の DOI:
  https://doi.org/10.5281/zenodo.21084279
- HZ8 論文 series 全体の DOI:
  https://doi.org/10.5281/zenodo.21084278
- HZ8 論文向け Ubuntu matrix:
  hakozuna-hz8/docs/HZ8_PAPER_PUBLIC_MATRIX_UBUNTU_X86_64.md

代表 MT snapshot
----------------

新しい横比較は `bench_matrix_malloc` の統合 matrix を使います。HZ10 を含む
現在の snapshot は throughput と post-workload RSS を同じセルに載せています。

| Row | HZ3 ops/s / post RSS | HZ4 ops/s / post RSS | HZ8 ops/s / post RSS | HZ10 ops/s / post RSS |
|-----|----------------------:|----------------------:|----------------------:|-----------------------:|
| guard_local0 | 156.85M / 12.35 MiB | 49.01M / 131.11 MiB | 207.05M / 2.00 MiB | 137.53M / 26.38 MiB |
| main_local0 | 149.31M / 15.11 MiB | 28.82M / 169.99 MiB | 117.94M / 3.50 MiB | 118.18M / 33.75 MiB |
| main_interleaved_r50 | 16.86M / 163.77 MiB | 12.28M / 258.10 MiB | 10.84M / 4.33 MiB | 20.18M / 79.50 MiB |
| main_interleaved_r90 | 10.20M / 205.82 MiB | 9.75M / 275.23 MiB | 7.04M / 4.62 MiB | 12.60M / 89.62 MiB |
| small_interleaved_remote90 | 12.95M / 130.92 MiB | 11.13M / 144.87 MiB | 14.70M / 2.95 MiB | 14.96M / 43.75 MiB |
| medium_interleaved_r50 | 15.43M / 148.37 MiB | 8.68M / 237.98 MiB | 9.84M / 3.83 MiB | 19.87M / 69.62 MiB |

現在の full table:
docs/benchmarks/20260707_allocator_line_integrated_hz3_hz4_hz8_hz10_r10/README.md

読み方:

- HZ8 は公開推奨 balanced line。統合 matrix では HZ 系の中で post RSS が最も低い。
- HZ10 は speed-oriented な研究候補。remote/interleaved throughput は強いが、
  この行列では post RSS が数十 MiB になる。
- 古い HZ3/HZ4/HZ5/HZ6/HZ8 の snapshot は
  docs/benchmarks/ALLOCATOR_HISTORY_SNAPSHOTS.md に移しています。

過去の HZ5/HZ6 詳細は history snapshot 側を参照してください。

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
