# S18-2（PTAG32 flat bin index）NO-GO アーカイブ

目的:
- S17（PTAG32 dst/bin direct）をさらに薄くするために、`tag32 = flat+1`（`flat = dst * HZ3_BIN_TOTAL + bin`）へ置換し、
  `hz3_free` hot の decode/stride を削る。

GO条件:
- SSOT（`bench_random_mixed_malloc_args`）で `mixed` +1% 以上、`small/medium` ±0%

結論:
- **NO-GO**（`small/mixed` が明確に退行）

---

## 実装フラグ

- `HZ3_PTAG_DSTBIN_ENABLE=1`
- `HZ3_PTAG_DSTBIN_FLAT=1`（S18-2）

比較用:
- baseline（S17）: `HZ3_PTAG_DSTBIN_ENABLE=1`
- 併用（参考）: `HZ3_PTAG_DSTBIN_FASTLOOKUP=1` + `HZ3_PTAG_DSTBIN_FLAT=1`（S18-1+S18-2）

---

## SSOTログ（RUNS=10）

- baseline（S17）: `/tmp/hz3_ssot_6cf087fc6_20260101_183642`
- S18-2（FLATのみ）: `/tmp/hz3_ssot_6cf087fc6_20260101_184813`
- 参考（FASTLOOKUP+FLAT併用）: `/tmp/hz3_ssot_6cf087fc6_20260101_183733`

---

## 結果（median, hz3）

baseline（S17）:
- small: 100.24M
- medium: 99.44M
- mixed: 100.59M

S18-2（FLATのみ）:
- small: 95.77M（-4.46%）
- medium: 100.04M（+0.60%）
- mixed: 95.40M（-5.16%）

参考（FASTLOOKUP+FLAT）:
- small/medium は上がるが mixed は落ちるため、併用も採用せず。

---

## 再計測手順（入口）

S17 baseline:
```sh
HZ3_LDPRELOAD_DEFS='-DHZ3_ENABLE=1 -DHZ3_SHIM_FORWARD_ONLY=0 -DHZ3_SMALL_V2_ENABLE=1 -DHZ3_SEG_SELF_DESC_ENABLE=1 -DHZ3_SMALL_V2_PTAG_ENABLE=1 -DHZ3_PTAG_V1_ENABLE=1 -DHZ3_PTAG_DSTBIN_ENABLE=1' \
HZ3_CLEAN=1 RUNS=10 ./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh
```

S18-2（FLAT）:
```sh
HZ3_LDPRELOAD_DEFS='-DHZ3_ENABLE=1 -DHZ3_SHIM_FORWARD_ONLY=0 -DHZ3_SMALL_V2_ENABLE=1 -DHZ3_SEG_SELF_DESC_ENABLE=1 -DHZ3_SMALL_V2_PTAG_ENABLE=1 -DHZ3_PTAG_V1_ENABLE=1 -DHZ3_PTAG_DSTBIN_ENABLE=1 -DHZ3_PTAG_DSTBIN_FLAT=1' \
HZ3_CLEAN=1 RUNS=10 ./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh
```

---

## 復活条件（やり直すなら）

- `flat` が有利になる “生成コード形（objdump tell）” の確度が上がった場合のみ。
- 例: `dst/bin` decode を消しても、`flat` の bounds/debug/check や周辺の分岐が増えていないことを先に確認してから再挑戦。
