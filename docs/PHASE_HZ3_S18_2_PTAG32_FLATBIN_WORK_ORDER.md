# PHASE_HZ3_S18-2: PTAG32 “flat bin index” (decode/stride削減) Work Order

目的:
- S17（dst/bin直結）をさらに薄くし、`hz3_free` hot の decode と bank address 計算を削る。
- 「tag32→(dst,bin) decode」ではなく、**tag32=flat+1** として “tagをそのまま配列index” にする。

前提:
- PTAGは32-bit固定（16-bit圧縮はNO-GO）。
- `HZ3_BIN_TOTAL` と `HZ3_NUM_SHARDS` は compile-time 定数。
- hot path は `range check + tag load + push` を守る（追加分岐禁止）。

参照:
- S17: `hakozuna/hz3/docs/PHASE_HZ3_S17_PTAG_DSTBIN_DIRECT_WORK_ORDER.md`

---

## 方針

現状（S17）:
- `tag32` から `dst` と `bin` を取り出す（shift/mask）
- `bank[dst][bin]` の行ストライド計算が入る（dst*stride + bin）

提案（S18-2）:
- `flat = dst * HZ3_BIN_TOTAL + bin`
- `tag32 = flat + 1`（0は not ours）
- free hot:
  - `flat = tag32 - 1`
  - `bin_ptr = bank_base + flat`（1回の加算）

これで hot から:
- dst/bin の decode を消せる
- dst*stride のアドレス計算を消せる

---

## 実装タスク

### S18-2A: compile-time gate

例:
- `HZ3_PTAG_DSTBIN_FLAT=0/1`（default 0）

### S18-2B: tag encode/decode helper を追加

- `hz3_pagetag32_encode_flat(dst, bin)` → `uint32_t(tag32)`
- `hz3_pagetag32_flat(tag32)` → `uint32_t(flat)`

debug only:
- `flat < (HZ3_NUM_SHARDS * HZ3_BIN_TOTAL)` の assert

### S18-2C: tag set 境界を更新

- small v2 page の tag set
- medium run の tag set

両方が “flat+1” を書くように統一する。

### S18-2D: hot free を flat 経由で push

- `Hz3Bin* base = &t_hz3_cache.bank[0][0];`
- `hz3_bin_push(base + flat, ptr);`

（`bank` は2次元だが連続領域なのでこの形が成立する前提を明文化）

### S18-2E: event flush の走査

bank flush（event-only）は:
- 2DのままでもOK
- flat で回してもOK（どちらでも hot には影響しない）

---

## A/B 計測

SSOT (RUNS=10):
- mixed +1%（理想 +2%）
- small/medium ±0%

perf stat（mixed）:
- `instructions` の減少が狙い（decode/stride削減）。

objdump tell:
- `shr/and` の tag decode が消える（または減る）
- `imul`/複雑な `lea` が減る

---

## GO/NO-GO

GO:
- mixed +1% 以上（RUNS=10 median）
- small/medium ±0%

NO-GO:
- mixed ±1% 内
- small/medium が落ちる

NO-GO の場合:
- revert
- `hakozuna/hz3/archive/research/s18_2_pagetag32_flatbin/README.md` に SSOT/perf/objdump を固定
- `hakozuna/hz3/docs/NO_GO_SUMMARY.md` に追記

---

## 結果（NO-GO, RUNS=10）

SSOTログ:
- baseline（S17）: `/tmp/hz3_ssot_6cf087fc6_20260101_183642`
- S18-2（FLATのみ）: `/tmp/hz3_ssot_6cf087fc6_20260101_184813`
- 参考（FASTLOOKUP+FLAT併用）: `/tmp/hz3_ssot_6cf087fc6_20260101_183733`

median（hz3）:
- baseline（S17）: small 100.24M / medium 99.44M / mixed 100.59M
- S18-2（FLAT）: small 95.77M（-4.46%）/ medium 100.04M（+0.60%）/ mixed 95.40M（-5.16%）

判定:
- `small/mixed` が明確に退行のため **NO-GO**。

アーカイブ:
- `hakozuna/hz3/archive/research/s18_2_pagetag32_flatbin/README.md`
