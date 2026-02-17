# HZ4 RSS Phase 0: OS Stats → Segment Count Reality Check

目的:
- hz4 の Peak RSS が高い原因を **推測せず**に数で確認する。
- まず「segment がどれだけ増えているか」を SSOT で可視化する。

結論（現状の観測）:
- hz4 は `hz4_os_seg_release()` を呼んでいない（= seg は基本的に増えっぱなし）。
- SSOT (T=16/R=90) の Peak RSS は、ほぼ `seg_acq * 1MB` の線形で支配される可能性が高い。

---

## Box Theory
- 変更は OS Box に閉じる（`hz4_os.c`）。
- hot path を汚さない（segment/page acquire/release のみカウント）。
- 出力は atexit **1行だけ**（SSOT のログを汚さない）。

---

## 1) ビルド（OS stats をON）

```sh
make -C hakozuna/hz4 clean all HZ4_DEFS_EXTRA='-DHZ4_OS_STATS=1'
```

---

## 2) 実行（SSOT）

```sh
LD_PRELOAD=./hakozuna/hz4/libhakozuna_hz4.so \
  ./hakozuna/out/bench_random_mixed_mt_remote_malloc 16 2000000 400 16 2048 90 65536
```

RSS とセットで取るなら:

```sh
env LD_PRELOAD=./hakozuna/hz4/libhakozuna_hz4.so \
  /usr/bin/time -v ./hakozuna/out/bench_random_mixed_mt_remote_malloc 16 2000000 400 16 2048 90 65536
```

注意:
- `atexit` は stdout flush より先に走るため、`[HZ4_OS_STATS]` 行が bench の stdout より前に見えることがある（正常）。
- `env LD_PRELOAD=... /usr/bin/time -v ...` は `time` 本体にも preload が効くため、`[HZ4_OS_STATS]` が 2 回出ることがある（1回目が子プロセス/bench、2回目が親/time）。

---

## 3) 観測点（SSOT 1行）

```
[HZ4_OS_STATS] seg_acq=<n> seg_rel=<n> page_acq=<n> page_rel=<n> large_acq=<bytes> large_rel=<bytes>
```

解釈:
- `seg_acq` が増え続け、`seg_rel=0` なら **RSS が下がらないのは自然**。

---

## 4) 代表ログ（短縮）

- short run (iters=300k, R=90) での例:
  - `[HZ4_OS_STATS] seg_acq=320 seg_rel=0 ...`
  - `Maximum resident set size ≈ 336,256 KB`
  - ⇒ seg 数と RSS が近い（1MB/seg の線形に近い）。

---

## 5) 次の箱（Phase 1 候補）

この Phase 0 は「原因の同定」だけ。

次は hz4 の設計方針に沿って **CollectBox 境界**から積む:

- **Option A (本命)**: Decommit-safe な page 管理
  - “decommit しても内部リンクが壊れない” free list/metadata へ（mimalloc系の設計）。
  - ページの空判定 → `madvise`/reclaim の導線を作る。
- **Option B (暫定)**: segment レベルの retire/release
  - seg 内のページが全て idle/empty のときだけ seg を返す（UAF/取りこぼしを避けるため条件は厳格）。

いずれも hot path に入れず、境界（`hz4_collect()` / `hz4_remote_flush()`）に集約する。

