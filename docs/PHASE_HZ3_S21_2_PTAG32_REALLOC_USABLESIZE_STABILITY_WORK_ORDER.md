# S21-2: PTAG32-first `realloc/usable_size` 安定化（実アプリ互換を先に固める）

目的:
- `HZ3_REALLOC_PTAG32=1` / `HZ3_USABLE_SIZE_PTAG32=1` を将来デフォルト化できるように、
  実アプリ（例: `/bin/ls`）での segfault を **再現→原因分解→修正**する。

前提（SSOT）:
- allocator の挙動切替は compile-time `-D` のみ（envノブ禁止）。
- `malloc/free` の hot path を汚さない（この作業は realloc/usable_size のみ。benchのhotには通常入らない）。
- false positive（foreign ptr を hz3 と誤認）は NG。

参照:
- 既存指示書（設計）: `hakozuna/hz3/docs/PHASE_HZ3_S21_PTAG32_UNIFY_REALLOC_USABLESIZE_WORK_ORDER.md`
- hz3 build/flags: `hakozuna/hz3/docs/BUILD_FLAGS_INDEX.md`

---

## 症状

- `HZ3_REALLOC_PTAG32=1` / `HZ3_USABLE_SIZE_PTAG32=1` を有効にしたビルドで、
  `LD_PRELOAD=./libhakozuna_hz3_ldpreload.so /bin/ls` が segfault する事例がある。
- これが解消できるまで **デフォルト化は禁止**。

---

## 0) 最小再現（必須）

### ビルド（例）

`HZ3_LDPRELOAD_DEFS` で上書きして “有効化した状態” を確実に作る:

```bash
cd hakmem
make -C hakozuna/hz3 clean all_ldpreload \
  HZ3_LDPRELOAD_DEFS='-DHZ3_ENABLE=1 -DHZ3_SHIM_FORWARD_ONLY=0 -DHZ3_PTAG_DSTBIN_ENABLE=1 -DHZ3_PTAG_DSTBIN_FASTLOOKUP=1 -DHZ3_REALLOC_PTAG32=1 -DHZ3_USABLE_SIZE_PTAG32=1'
```

### 実行

```bash
cd hakmem
LD_PRELOAD=./libhakozuna_hz3_ldpreload.so /bin/ls >/dev/null
```

GO:
- `/bin/ls` が安定して完走する（RUNSは不要。まずクラッシュしないこと）。

---

## 1) A/B で原因を切り分ける（realloc vs usable_size）

### A: `usable_size` のみ

- `-DHZ3_USABLE_SIZE_PTAG32=1 -DHZ3_REALLOC_PTAG32=0`

### B: `realloc` のみ

- `-DHZ3_USABLE_SIZE_PTAG32=0 -DHZ3_REALLOC_PTAG32=1`

判定:
- A で落ちる → `usable_size` 由来（サイズ復元の過大見積もり / foreign ptr 誤認など）
- B で落ちる → `realloc` 由来（old_size 推定、コピー長、fallback再入など）
- 両方落ちる → 共通の ptr分類/size復元の問題

---

## 2) “過大見積もり” を fail-fast で潰す（debug only）

典型的なクラッシュ要因:
- `old_size`（推定 usable）を **実際より大きく見積もる**と、
  `realloc` が「そのまま返す」or `memcpy` が範囲外 read をしてメモリ破壊につながる。

方針:
- **推定 usable は常に under-estimate（安全側）**に寄せる。
- debug build だけ “shadow check” を入れて、推定が過大なら即 abort。

例（設計）:
- small: `usable = sc_size`（exact）
- medium: `usable <= class_bytes` の範囲で保守的に（ヘッダ/整列が不明なら減算して under-estimate）
- large: PTAG32で完結させず `hz3_large_usable_size()` 等の既存経路で正確に

---

## 3) ptr分類の安全策（foreign ptr）

原則:
- “arena in-range” は hz3 管理領域のはずだが、移行期・バグ時に `tag32==0` があり得る。

挙動（推奨）:
- debug: `in_range && tag==0` は abort（`HZ3_PTAG_FAILFAST=1`）
- release: `in_range && tag==0` は **foreign へは渡さない**（realloc は `NULL`、usable_size は `0`）
  - ここで RTLD_NEXT に渡すと、hz3管理ポインタを foreign allocator に渡して破壊するリスクがあるため。

---

## 4) 完了条件（デフォルト化の前提）

最低条件:
- `HZ3_REALLOC_PTAG32=1` / `HZ3_USABLE_SIZE_PTAG32=1` で
  - `LD_PRELOAD=... /bin/ls` が安定完走
  - `LD_PRELOAD=... /bin/echo` / `/bin/true` も完走

次の条件（任意）:
- `run_bench_hz3_ssot.sh`（RUNS=10）で small/medium/mixed が明確に退行しない
  - realloc/usable_size の hot 侵入がある場合のみ評価する（通常は影響しにくい）

---

## 状態（2026-01-01）: GO（安定化完了）

報告（要約）:
- `/bin/ls` の segfault は現状再現しない（複数回完走）。
- `in_range && tag32==0` の扱いを “停止側（NULL/0）” ではなく **slow path fallthrough** に変更し、実アプリ互換を優先。

実装方針（SSOT）:
- `tag32 lookup` が成功 → PTAG32-first で処理（fast）
- `in_range && tag32==0` → **slow path** で追加分類（segmap/seg_hdr 等）
- `not in arena` → foreign（RTLD_NEXT）へ（「確実に非hz3」のときだけ）

注記:
- ここでの “GO” は「実アプリ互換の最低ライン（smoke）」達成。
- デフォルト化は、もう少し広い smoke（例: `/bin/find`, `/usr/bin/git`, `cmake` 等）を通してから判断する。

### 実測ログ（smoke / SSOT）

ビルド（フラグON）:
- `make -C hakozuna/hz3 clean all_ldpreload HZ3_LDPRELOAD_DEFS="... -DHZ3_REALLOC_PTAG32=1 -DHZ3_USABLE_SIZE_PTAG32=1"`

smoke（LD_PRELOAD）:
- OK: `/bin/true`, `/bin/echo`, `/bin/ls`, `/usr/bin/env -i`, `/usr/bin/find --version`, `/usr/bin/git --version`, `/usr/bin/cmake --version`, `/usr/bin/python3 -V`

SSOT（uniform, RUNS=10）ログ:
- `/tmp/hz3_ssot_6cf087fc6_20260101_232532`
  - hz3: small `102.04M` / medium `103.40M` / mixed `96.46M`

SSOT（dist=app, RUNS=30, seed固定）ログ:
- `/tmp/hz3_ssot_6cf087fc6_20260101_232906`
  - hz3: small `53.27M` / medium `56.47M` / mixed `51.61M`
  - tcmalloc: small `57.90M` / medium `57.67M` / mixed `56.43M`

### デフォルト化後の再確認（2026-01-01）

`hakozuna/hz3/Makefile` の `HZ3_LDPRELOAD_DEFS` に `HZ3_REALLOC_PTAG32=1` / `HZ3_USABLE_SIZE_PTAG32=1` を追加した状態で再計測:

- SSOT（uniform, RUNS=5）ログ: `/tmp/hz3_ssot_6cf087fc6_20260101_233423`
  - hz3: small `99.07M` / medium `104.09M` / mixed `101.20M`
- SSOT（dist=app, RUNS=10, seed固定）ログ: `/tmp/hz3_ssot_6cf087fc6_20260101_233443`
  - hz3: small `53.60M` / medium `54.61M` / mixed `53.17M`
  - tcmalloc: small `57.41M` / medium `57.21M` / mixed `56.67M`
