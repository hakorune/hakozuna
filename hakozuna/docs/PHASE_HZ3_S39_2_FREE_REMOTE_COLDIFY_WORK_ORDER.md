# S39-2: hz3_free の remote 経路を cold 化して “bank lookup 命令” を hot から追放（Work Order）

背景（S39 Step1）:
- `hz3_free` hot path は概ね ~45 命令で、支配的 self%（~17%）。
- ターゲット候補 A/B/C は “既に済み / 退化方向”。
- 残っている重い塊は **TLS bank_bin lookup のオフセット計算（~8 命令）**。
- ただし bank row cache（S28-2A）は NO-GO。

仮説:
- compiler が `dst==my_shard` の分岐に対して、bank 側のアドレス計算を hot 側に残している（CMOV/共通化/ライブレンジ等）。
- remote 経路を **noinline + cold helper** に隔離すれば、local hot から bank lookup 計算を追放できる。

目的:
- `hz3_free` の local hot（dst==my_shard）を “短い一本道” にし、命令数/依存を削る。
- remote 経路の call は許容（ST では稀、MT は別評価）。

---

## A/B フラグ

- `HZ3_FREE_REMOTE_COLDIFY=0/1`（default 0）
  - 0: 現状維持（inline のまま local/remote を同一関数で処理）
  - 1: remote 経路を `__attribute__((noinline, cold))` helper に移動

---

## 実装方針（境界 1 箇所）

対象:
- `hakozuna/hz3/src/hz3_hot.c` の `hz3_free()` 内、`HZ3_LOCAL_BINS_SPLIT` ブロック

形（イメージ）:
- local は必ず `hz3_tcache_get_local_bin_from_bin_index(bin)` へ直行
- remote は helper へ（helper 内で `hz3_tcache_get_bank_bin(dst, bin)` を実行）

```c
__attribute__((noinline, cold))
static void hz3_free_remote_push(uint8_t dst, uint32_t bin, void* ptr) {
    hz3_bin_push(hz3_tcache_get_bank_bin(dst, bin), ptr);
}

// in hz3_free hot:
if (__builtin_expect(dst == t_hz3_cache.my_shard, 1)) {
    hz3_bin_push(hz3_tcache_get_local_bin_from_bin_index(bin), ptr);
    return;
}
hz3_free_remote_push(dst, bin, ptr);
return;
```

注意:
- **新しい TLS フィールドは足さない**（S28-2A の NO-GO を踏まえ、構造を変えない）
- 正しさは変えない（remote を outbox 化するのは別フェーズで検討）

---

## 計測（必須）

### 1) objdump で “bank lookup 計算” が hot から消えたか

```bash
objdump -d --no-show-raw-insn ./libhakozuna_hz3_ldpreload.so \
  | rg -n "hz3_free|hz3_free_remote_push|imul|lea|cmov" | head -n 200
```

期待:
- `hz3_free` hot 側から `dst*stride` っぽい計算が薄くなる
- remote helper 側に寄る

### 2) SSOT（RUNS=30）

```bash
# baseline
HZ3_LDPRELOAD_DEFS_EXTRA="-DHZ3_FREE_REMOTE_COLDIFY=0" \
RUNS=30 ITERS=20000000 WS=400 \
./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh

# treatment
HZ3_LDPRELOAD_DEFS_EXTRA="-DHZ3_FREE_REMOTE_COLDIFY=1" \
RUNS=30 ITERS=20000000 WS=400 \
./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh
```

### 3) perf（self% が動いたか）

```bash
env -i PATH="$PATH" HOME="$HOME" \
  LD_PRELOAD=./libhakozuna_hz3_ldpreload.so \
  perf record -g -- \
  ./system_bench_random_mixed 20000000 400 1

perf report --stdio | rg -n "hz3_free|hz3_free_remote_push" | head -n 80
```

---

## GO/NO-GO

GO（目安）:
- mixed/medium が +1% 以上（RUNS=30 median）
- dist=app 退行なし（必要なら別途 RUNS=30）

NO-GO:
- 伸びない / 退行する（特に MT を悪化させる場合は NO-GO 扱い）

---

## 結果固定（必須）

`hakozuna/hz3/archive/research/s39_2_free_remote_coldify/README.md` に:
- baseline/treatment のログパス
- SSOT table
- objdump の差分要点（hot から何が消えたか）
- rollback（`HZ3_FREE_REMOTE_COLDIFY=0`）

