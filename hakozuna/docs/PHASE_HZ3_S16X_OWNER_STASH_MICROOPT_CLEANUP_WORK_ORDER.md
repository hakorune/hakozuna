# PHASE HZ3 S16x OwnerStash MicroOpt Cleanup（アーカイブ＋本線掃除）

目的:
- NO-GO が確定した S16x 系 micro-opt の分岐を **本線から除去**して見通しを回復する。
- 記録は `archive/research` に移し、再評価は **opt-in** でのみ行える状態に固定する。

対象（NO-GOで確定、アーカイブ推奨）
- **S166** `HZ3_S166_S112_REMAINDER_FASTPOP`
- **S167** `HZ3_S167_WANT32_UNROLL`
- **S168** `HZ3_S168_S112_REMAINDER_SMALL_FASTPATH`
- **S171** `HZ3_S171_S112_BOUNDED_PREBUF`
- **S172** `HZ3_S172_S112_BOUNDED_PREFILL`

非対象（本線に残す）
- **S169** `HZ3_S169_S112_BOUNDED_DRAIN` → opt-inレーン用途として残す
- **S170** `HZ3_S170_S112_BOUNDED_STATS` → 計測箱として残す（必要なときだけ有効）

## 1) 事前準備（ログ固定）
既存の A/B 結果を README に残すため、ログや数値を箇条書きで控える。
（例: S166/S167/S168/S171/S172 の r90/r50, ops/sec, instructions など）

## 2) 研究箱アーカイブを作成
新規フォルダ:
- `hakozuna/hz3/archive/research/s16x_owner_stash_microopt/`

作成ファイル:
- `README.md`（必須）

README には以下を記載:
- 目的（S16x micro-opt を試したが NO-GO）
- 変更点（各 S16x の狙いと実装箇所）
- GO 条件（例: r90/r50 ともに -1%以内）
- 結果（NO-GO の根拠）
- SSOTログ（`/tmp/...` のパス）
- 復活条件（例: T=16/R=90 で明確勝ちが出たら再評価）

## 3) 本線コードの掃除（分岐削除）
対象ファイル:
- `hakozuna/hz3/src/owner_stash/hz3_owner_stash_pop.inc`

削除するブロック（例）:
- `#if HZ3_S166_S112_REMAINDER_FASTPOP` ～ `#endif`
- `#if HZ3_S167_WANT32_UNROLL` ～ `#endif`
- `#if HZ3_S168_S112_REMAINDER_SMALL_FASTPATH` ～ `#endif`
- `#if HZ3_S171_S112_BOUNDED_PREBUF` ～ `#endif`
- `#if HZ3_S172_S112_BOUNDED_PREFILL` ～ `#endif`

※ S169/S170 の本体は残す。

## 4) フラグの移設（archive へ）
対象ファイル:
- `hakozuna/hz3/include/hz3_config_scale.h`
  - S166/S167/S168/S171/S172 の定義を削除
- `hakozuna/hz3/include/hz3_config_archive.h`
  - 各フラグを `#error` 付きで追加（再有効化防止）
  - 参照先を `archive/research/s16x_owner_stash_microopt/README.md` に揃える

## 5) ドキュメント更新
- `hakozuna/hz3/docs/NO_GO_SUMMARY.md`
  - 「S16x OwnerStash MicroOpt（NO-GO）」として追加
  - アーカイブ README へのリンクを追加
- `CURRENT_TASK.md`
  - 「S16x 系はアーカイブ済み」を追記

## 6) ビルド確認（最低限）
```sh
make -C hakozuna/hz3 clean all_ldpreload_scale
```

## 7) 簡易スモーク（任意）
```sh
LD_PRELOAD=./hakozuna/hz3/out/libhakozuna_hz3_scale.so \
  ./hakozuna/out/bench_random_mixed_mt_remote_malloc 16 2000000 400 16 2048 90 65536
```

## 期待する効果
- pop.inc の分岐数削減 → 読みやすさ改善
- NO-GO の再有効化事故を防止（archive 側で #error）

