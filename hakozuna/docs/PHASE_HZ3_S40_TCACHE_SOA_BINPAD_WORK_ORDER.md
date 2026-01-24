# S40: TLS tcache “フラット化”（SoA + power-of-two bin padding）で命令数差を詰める — Work Order

Status: **GO（RUNS=30 で確定）**

- 結果固定: `hakozuna/hz3/archive/research/s40_tcache_soa_binpad/README.md`

背景（Phase 97 / S39）:
- gap の主因が **命令数差**である（IPC/ミス率では説明できない）。
- `hz3_free` hot の重い塊として **TLS bank bin lookup のオフセット計算（~8命令）**が観測された。
- S28-2A（bank row cache）は NO-GO、S39-2（coldify）は call 税で NO-GO。

仮説:
- `bank[dst][bin]` の `dst * HZ3_BIN_TOTAL + bin` が **imul/複雑な LEA** を誘発しやすい。
- tcmalloc 風の「TLSを完全にフラットにして、スケールドアドレスだけで叩く」形に寄せると、構造的に命令が減る。
- 特に stride を power-of-two にしておくと、行計算が `shl` で終わる（確実に薄くなる）。

目的:
- free/malloc hot の “bin アドレス計算” を構造的に短縮し、mixed/dist の gap を詰める。

非目標:
- PTAG の方式変更（S18-2 のように tag 形式を変えるのはこのフェーズではしない）
- runtime env knob 追加（hz3 は compile-time のみ）

---

## Box Theory

Box: **TCacheLayoutBox**
- 役割: TLS tcache のレイアウト（AoS→SoA、stride→pow2）を 1 箇所に閉じ込める
- 境界:
  - `hz3_tcache_get_*` 系アクセサ
  - `hz3_bin_push/pop`（hot の list 操作）
- ルール: `hz3_free/hz3_malloc` 側のロジックはできるだけ触らない（呼び出し形を保つ）

---

## A/B フラグ（提案）

- `HZ3_TCACHE_SOA=0/1`（header default 0 / hz3 lane default 1）
  - 0: 現状（`Hz3Bin { head,count }` の配列）
  - 1: SoA（`head[]` / `count[]` を別配列で保持）

- `HZ3_TCACHE_SOA_LOCAL=0/1`（default 0）
  - `HZ3_TCACHE_SOA=1` の分解版（local のみ SoA を切りたい時に使う）

- `HZ3_TCACHE_SOA_BANK=0/1`（default 0）
  - `HZ3_TCACHE_SOA=1` の分解版（bank のみ SoA + pad を切りたい時に使う）

- `HZ3_BIN_PAD_LOG2=0/8`（header default 0 / hz3 lane default 8）
  - 0: padding 無し（現状の `HZ3_BIN_TOTAL` を使う）
  - 8: `HZ3_BIN_PAD = 1<<8 = 256`（power-of-two stride）

注意:
- 最初は “local のみ SoA” を切って安全に進め、その後 “bank も SoA + pad” に進む。

---

## 実装段階（戻せるように小さく積む）

### Stage 1（S40-1）: local bins を SoA 化（bank は触らない）

狙い:
- `hz3_malloc` hit / local free の命令数を落とす（比較的安全）
- TLS サイズ増加を最小に抑える

変更点（例）:
- `Hz3TCache` 内の `local_bins[]` を
  - `void* local_head[]`
  - `Hz3BinCount local_count[]`
  にする

必要な境界変更:
- `hz3_tcache_get_local_bin_from_bin_index()` を “bin ポインタ” ではなく “head/count 参照” を返す形に変更するか、
  もしくは `hz3_local_bin_push/pop(bin, ptr)` を新設して call site を最小変更にする。

### Stage 2（S40-2）: bank bins を SoA 化 + bin padding（pow2 stride）

狙い:
- S39 で観測された “bank lookup の 8 命令” を構造的に消す（ここが本命）

変更点（例）:
- `bank[dst][bin]` を廃止し、次へ:
  - `void* bank_head[HZ3_NUM_SHARDS][HZ3_BIN_PAD]`
  - `Hz3BinCount bank_count[HZ3_NUM_SHARDS][HZ3_BIN_PAD]`
- `HZ3_BIN_PAD = (HZ3_BIN_PAD_LOG2 ? (1u<<HZ3_BIN_PAD_LOG2) : HZ3_BIN_TOTAL)`
- hot 側は `dst<<HZ3_BIN_PAD_LOG2` + `bin` の形に寄せる（`HZ3_BIN_PAD_LOG2=8` の場合）

event-only の注意:
- flush/scan は “実 bin 数（HZ3_BIN_TOTAL）だけ” を走査し、padding 領域を触らない。

---

## objdump / perf の合格条件（必須）

### 1) objdump tell（命令形）

狙い:
- bank bin のアドレス計算から `imul` が消える / 薄くなる。

```bash
objdump -d --no-show-raw-insn ./libhakozuna_hz3_ldpreload.so \
  | rg -n "hz3_free|hz3_pagetag32_lookup|imul|lea|shl" | head -n 200
```

### 2) throughput SSOT（RUNS=30）

```bash
# baseline
HZ3_LDPRELOAD_DEFS_EXTRA="-DHZ3_TCACHE_SOA=0 -DHZ3_BIN_PAD_LOG2=0" \
RUNS=30 ITERS=20000000 WS=400 \
./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh

# treatment
HZ3_LDPRELOAD_DEFS_EXTRA="-DHZ3_TCACHE_SOA=1 -DHZ3_BIN_PAD_LOG2=8" \
RUNS=30 ITERS=20000000 WS=400 \
./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh
```

### 3) dist=app（RUNS=30）

dist が敏感なので必須。

---

## GO/NO-GO

GO（目安）:
- mixed/medium が +1% 以上（RUNS=30 median）
- dist=app 退行なし（±0.5% 以内）

NO-GO:
- 伸びない / 退行する / TLS サイズ増が許容できない

---

## リスクと観測

リスク:
- TLS サイズ増（padding 256 は特に増える）
- 走査/flush の “無駄アクセス” が増えると逆噴射し得る

観測（最低限）:
- throughput（SSOT）
- `perf stat` の `instructions`（主目的）
- L1 miss の増減（増えた場合は layout の副作用を疑う）

---

## 結果固定（必須）

`hakozuna/hz3/archive/research/s40_tcache_soa_binpad/README.md` に:
- 実装フラグ（treatment/baseline）
- SSOT table（median/cv）
- dist=app table
- objdump tell（imul がどう変わったか）
- rollback（`HZ3_TCACHE_SOA=0` / `HZ3_BIN_PAD_LOG2=0`）
