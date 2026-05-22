# Hakozuna HZ5 Source Map

このドキュメントは `hakozuna-hz5` の各ソースファイルが、パフォーマンスクリティカルな**Hot Path**に属するか、それとも状態監視や初期化用の**Diagnostic / Cold Path**に属するかを整理したマップです。

## Hot Path (Performance Critical)
ここに含まれるコードは、アロケーション・デアロケーションのクリティカルパスであり、インライン化や分岐予測の最適化、マジックナンバーの排除などを優先すべき領域です。

* `core/hz5_tcache.c`: スレッドローカルなキャッシュの取得と返却（最速パス）。
* `core/hz5_run.c`: `tcache` ミス時の run (ページブロック) レベルの割り当てと解放。
* `core/hz5_segment.c`: 新規セグメントの割り当てや、ランのメタデータ管理。頻繁に呼ばれるビットマップ操作を含む。

## Cold Path / Diagnostic
ここに含まれるコードは、初期化時やスレッド終了時、バックグラウンドでのメモリ回収、または統計情報の取得など、パフォーマンス上の影響が比較的小さい領域です。

* `core/hz5_owner.c`: スレッドの所有権（Owner Token）の管理と終了処理 (Destructor)。
* `core/hz5_remote.c`: リモートスレッドからの解放予約（Remote Free）のキューイングと、非同期な回収 (Drain) 処理。
* `core/hz5_stats.c`: メモリアロケータ内部の各種診断用統計データ (Diagnostics) の集計・出力。
* `win/hz5_p1_smoke.c`: スモークテスト用のテストランナー。

## Design Notes

* `docs/HZ5_CONTROL_PLANE_DESIGN.md`: P45dr 後の HZ5 control-plane 設計メモ。P25 bridge を speed layer、P43 segment source を source layer、P40 release を source-demotion intent、OPEN/DRAIN/CLOSED admission を control plane として扱う。
