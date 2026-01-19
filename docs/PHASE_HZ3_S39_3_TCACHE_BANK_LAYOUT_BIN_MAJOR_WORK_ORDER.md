# S39-3: TLS bank bin lookup の命令数削減 — bank 配列を bin-major にする（Work Order）

背景:
- S39 Step1: `hz3_free` hot path で重い塊が「TLS bank_bin lookup のオフセット計算（~8命令）」として観測された。
- S39-2（remote coldify）は call/ret 税が勝って NO-GO。
- S28-2A（bank row cache）は NO-GO。
- S18-2（PTAG32 flat bin）は tag エンコード変更が絡み NO-GO。

仮説:
- 現在の `bank[dst][bin]` は stride が `HZ3_BIN_TOTAL`（≒140）で、`dst*stride + bin` が **imul/複雑な lea** になりやすい。
- `bank[bin][dst]`（bin-major）に変えると stride が `HZ3_NUM_SHARDS`（=8）になり、`bin*8 + dst` は **shift+add** で作れる。
- tag decode はそのまま（dst/bin を使う）。変えるのは **TLS 配列レイアウトだけ**。

目的:
- free hot の bank lookup 命令数を削る（特に remote / bank push）。
- 変更は compile-time A/B で導入し、即 rollback できるようにする。

---

## A/B フラグ

- `HZ3_TCACHE_BANK_BIN_MAJOR=0/1`（default 0）
  - 0: 現状 (`bank[dst][bin]`)
  - 1: 新案 (`bank_by_bin[bin][dst]`)

注意:
- hz3 は env ノブ禁止。`HZ3_LDPRELOAD_DEFS_EXTRA` で差分だけ渡す。

---

## 実装スコープ（箱と境界）

Box: **TCacheBankLayoutBox**
- 境界: `hz3_tcache_get_bank_bin(dst, bin)` / `hz3_tcache_get_bank_bin_flat(flat)` の 2つだけを差し替える
- ルール:
  - 呼び出し側の `hz3_free` / `hz3_malloc` を汚さない
  - “配列レイアウト差” は tcache 内部に閉じ込める

対象ファイル（目安）:
- `hakozuna/hz3/include/hz3_types.h`（`Hz3TCache` の bank 配列）
- `hakozuna/hz3/include/hz3_tcache.h`（アクセサ）
- `hakozuna/hz3/src/hz3_tcache.c`（初期化/flush で bank を走査する箇所）

---

## 実装方針（例）

### 1) TLS 配列を 2種持つ（A/B）

- baseline:
  - `Hz3Bin bank[HZ3_NUM_SHARDS][HZ3_BIN_TOTAL];`
- treatment:
  - `Hz3Bin bank_by_bin[HZ3_BIN_TOTAL][HZ3_NUM_SHARDS];`

### 2) アクセサで吸収（境界1箇所）

- `hz3_tcache_get_bank_bin(dst, bin)`:
  - baseline: `&t_hz3_cache.bank[dst][bin]`
  - treatment: `&t_hz3_cache.bank_by_bin[bin][dst]`

- `hz3_tcache_get_bank_bin_flat(flat)`:
  - `flat = dst * HZ3_BIN_TOTAL + bin`（既存の意味を維持）
  - treatment 側は `bin = flat % HZ3_BIN_TOTAL`, `dst = flat / HZ3_BIN_TOTAL` を避けたいので、
    **flat を作る側**（free hot）では引き続き `dst/bin` から取る（S18-2 の “flat tag” は使わない）。
  - つまり S39-3 では `*_flat` は “event-only” でしか使わない前提でもOK。

---

## objdump 期待値（必須）

狙い:
- `hz3_free` hot の bank bin address 計算から `imul` が消える / 薄くなる。

```bash
objdump -d --no-show-raw-insn ./libhakozuna_hz3_ldpreload.so \
  | rg -n "hz3_free|imul|lea|shl|add" | head -n 200
```

---

## A/B 計測（RUNS=30）

```bash
# baseline
HZ3_LDPRELOAD_DEFS_EXTRA="-DHZ3_TCACHE_BANK_BIN_MAJOR=0" \
RUNS=30 ITERS=20000000 WS=400 \
./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh

# treatment
HZ3_LDPRELOAD_DEFS_EXTRA="-DHZ3_TCACHE_BANK_BIN_MAJOR=1" \
RUNS=30 ITERS=20000000 WS=400 \
./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh
```

追加（必須）:
- dist=app（RUNS=30, seed固定）も同条件で測る（S39-2 で dist が敏感だったため）。

---

## GO/NO-GO

GO（目安）:
- mixed/medium が +1% 以上（RUNS=30 median）
- dist=app 退行なし（±0.5% 以内）

NO-GO:
- 伸びない / 退行する / 実装が大きすぎる割に効果が薄い

---

## 結果固定（必須）

`hakozuna/hz3/archive/research/s39_3_tcache_bank_bin_major/README.md` に:
- baseline/treatment のログパス
- SSOT table（median/cv）
- objdump の要点（imul がどう変わったか）
- rollback（`HZ3_TCACHE_BANK_BIN_MAJOR=0`）

