# hz3 GO Boxes（採用/推奨 台帳）

このドキュメントは、`hz3` で **運用上の“正”**として扱う lane / Box / preset を一覧化します。

- NO-GO（研究箱アーカイブ）: `hakozuna/hz3/docs/NO_GO_SUMMARY.md`
- hard-archive の強制OFF台帳: `hakozuna/hz3/docs/HZ3_ARCHIVED_BOXES.md`
- フラグ詳細（索引）: `hakozuna/hz3/docs/BUILD_FLAGS_INDEX.md`
- 現在の採用状態（SSOT）: `CURRENT_TASK.md`

最終同期: 2026-02-12（`CURRENT_TASK.md` 準拠）

## 判定の用語

- **GO(default)**: 既定lane（通常運用の正）。
- **GO(lane)**: lane/preset として提供（用途依存なので既定にはしない）。
- **GO(opt-in)**: 研究/検証用に opt-in（A/B でのみ使う）。

---

## GO(default)

| Lane | Build | Output | 備考 |
|---|---|---|---|
| scale | `make -C hakozuna/hz3 clean all_ldpreload_scale` | `./libhakozuna_hz3_scale.so` | `./libhakozuna_hz3_ldpreload.so` は scale への symlink |

注記:
- `make -C hakozuna/hz3 all_ldpreload` は互換 alias で、実体は `all_ldpreload_scale`。
- 過去フェーズ文書の `all_ldpreload` は、現在は原則 `all_ldpreload_scale` と読み替える。

---

## GO(default knobs)

| Knob | Default | 意図 | Source |
|---|---:|---|---|
| `HZ3_S190_MISS_TRIGGERED_REMOTE_FLUSH` | `1` | medium central miss 時の remote flush retry | `hakozuna/hz3/include/config/hz3_config_rss_memory_part6_s188_to_s211_medium.inc` |
| `HZ3_S190_FLUSH_BUDGET_BINS` | `32` | S190 retry flush の既定budget | `hakozuna/hz3/include/config/hz3_config_rss_memory_part6_s188_to_s211_medium.inc` |
| `HZ3_S193_DEMAND_SCAVENGE` | `1` | large reclaim を demand 駆動で実行 | `hakozuna/hz3/include/config/hz3_config_rss_memory_part2_s50_large_cache.inc` |
| `HZ3_S207_TARGETED_REUSE` | `1` | large high-band の miss 駆動 reuse admission | `hakozuna/hz3/include/config/hz3_config_rss_memory_part3_s207_large_reuse.inc` |
| `HZ3_S210_MEDIUM_OWNER_RESCUE_LITE` | `1` | medium owner rescue の軽量ゲート | `hakozuna/hz3/include/config/hz3_config_scale_part7_flush_logic.inc` |
| `HZ3_S212_LARGE_UNMAP_DEFER_PLUS` | `1` | large unmap defer+（free path/defer drain） | `hakozuna/hz3/include/config/hz3_config_rss_memory_part2_s50_large_cache.inc` |
| `HZ3_S218_LARGE_SUPPLY_FLOOR_BOOST` | `1` | high-band large 供給floorを miss 駆動で底上げ | `hakozuna/hz3/include/config/hz3_config_rss_memory_part3_s207_large_reuse.inc` |
| `HZ3_S222_CENTRAL_ATOMIC_FAST` | `1` | medium central の atomic fastpath | `hakozuna/hz3/include/config/hz3_config_scale_part8_modern.inc` |

注記:
- hard-archived の box（`S220/S221/S223` など）はこの表に載せない。
- default判定は `CURRENT_TASK.md` の gate（`RUNS=21`, `guard>=-1.0%`）に従う。

---

## GO(lane)

| Lane | Build | Output | 目的 |
|---|---|---|---|
| fast | `make -C hakozuna/hz3 clean all_ldpreload_fast` | `./libhakozuna_hz3_fast.so` | ベンチ/探索用（既定laneは汚さない） |
| r50 | `make -C hakozuna/hz3 all_ldpreload_scale_r50` | `./libhakozuna_hz3_scale_r50.so` | MT remote 比率が低めの想定 |
| r90 | `make -C hakozuna/hz3 all_ldpreload_scale_r90` | `./libhakozuna_hz3_scale_r90.so` | MT remote-heavy 想定 |
| tolerant | `make -C hakozuna/hz3 all_ldpreload_scale_tolerant` | `./libhakozuna_hz3_scale_tolerant.so` | 超高スレッド数向け（衝突許容） |

## GO(opt-in build lanes)

| Lane | Build | Output | 目的 |
|---|---|---|---|
| p32 | `make -C hakozuna/hz3 all_ldpreload_scale_p32` | `./libhakozuna_hz3_scale_p32.so` | PTAG32-only（`HZ3_NUM_SHARDS>63` の検証） |
| mem_mstress | `make -C hakozuna/hz3 all_ldpreload_scale_mem_mstress` | `./libhakozuna_hz3_scale_mem_mstress.so` | RSS重視メモリレーン |
| mem_large | `make -C hakozuna/hz3 all_ldpreload_scale_mem_large` | `./libhakozuna_hz3_scale_mem_large.so` | malloc-large 速度重視レーン |
| s64_purge0 | `make -C hakozuna/hz3 all_ldpreload_scale_s64_purge0` | `./libhakozuna_hz3_scale_s64_purge0.so` | S64 purge delay=0 研究レーン |
| s64_p5b | `make -C hakozuna/hz3 all_ldpreload_scale_s64_p5b` | `./libhakozuna_hz3_scale_s64_p5b.so` | S64 P5b（RSS研究） |
| s65_purge0 | `make -C hakozuna/hz3 all_ldpreload_scale_s65_purge0` | `./libhakozuna_hz3_scale_s65_purge0.so` | S65 aggressive purge 研究レーン |

---

## SSOT（入口）

| 用途 | Script | 備考 |
|---|---|---|
| hz3 SSOT（lane triage） | `hakozuna/hz3/scripts/run_bench_hz3_ssot.sh` | `.env` で条件固定。まずここを正にする |
| hz3 SSOT A/B | `hakozuna/scripts/ssot_ab_hz3.sh` | sha1固定・stdout+stderr収集 |
| MT-REMOTE 5者比較 | `hakozuna/scripts/ssot_mt_remote_matrix.sh` | allocator取り違え防止 |
