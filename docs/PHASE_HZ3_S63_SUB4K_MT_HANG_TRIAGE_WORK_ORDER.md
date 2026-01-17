# PHASE_HZ3_S63: Sub4k MT Hang Triage Box

Status: READY

Note: `HZ3_S63_SUB4K_MT_TRIAGE` は 2026-01-10 の掃除で削除済み（本ドキュメントは履歴用）。

Date: 2026-01-09  
Track: hz3  
Previous:
- S62-1b: Sub4kRunRetireBox **GO**（single-thread）
- MT hang observed when `HZ3_SUB4K_ENABLE=1` and threads>=2

---

## 0) 目的（SSOT）

`HZ3_SUB4K_ENABLE=1` の multi-thread で発生するハングを **境界 1 箇所**の計測で特定する。  
hot path を汚さず、thread-exit / central / lock 境界で止まりどころを固定する。

---

## 1) 最小再現

```
HZ3_SUB4K_ENABLE=1 ./hakozuna/bench/linux/hz3/bench_burst_idle_v2 1000 1 1 0 2
```

sub4k サイズ帯を明示的に踏みたい場合（obj_min/obj_max を指定）:
```
HZ3_SUB4K_ENABLE=1 ./hakozuna/bench/linux/hz3/bench_burst_idle_v2 1000 1 1 0 2 2304 3840
```

失敗時は timeout で切る（SSOT 用）:
```
timeout 10s HZ3_SUB4K_ENABLE=1 ./hakozuna/bench/linux/hz3/bench_burst_idle_v2 1000 1 1 0 2
```

---

## 2) 箱割り（境界 1 箇所 / hot path 0）

Box: `S63_Sub4kMTTriageBox`

Boundary（cold / 1 箇所）:
- `hz3_tcache_destructor()` 内の sub4k drain/flush 周辺

補助観測点（必要時のみ）:
- `hz3_sub4k_central_pop_batch_ext()` / `hz3_sub4k_central_push_list_ext()`  
  ※ counter only（常時ログ禁止）

---

## 3) Instrumentation（最小 / 戻せる）

compile-time flag:
- `HZ3_S63_SUB4K_MT_TRIAGE` (default OFF)

Stats（atexit one-shot）:
```
[HZ3_S63_SUB4K_MT] dtor_enter=... dtor_exit=...
  alloc_calls=... pop_calls=... pop_objs=... push_calls=... push_objs=...
  lock_contended=... (optional)
```

Fail-fast:
- dtor_enter なのに dtor_exit が出ない → destructor 内でハング
- pop/push カウンタが伸びない → 呼び出し前でハング

---

## 4) 実装ガイド（最小差分）

1) `hz3_config.h` に flag 追加
```
#ifndef HZ3_S63_SUB4K_MT_TRIAGE
#define HZ3_S63_SUB4K_MT_TRIAGE 0
#endif
```

2) `hz3_sub4k.c` に counter（atomic）
- pop/push の入口で `INC`
- pop/push の obj 数を `ADD`
- hot path を避けたい場合は **central API のみ**で計測

3) `hz3_tcache.c` の destructor で enter/exit を計測
- sub4k drain 直前で `dtor_enter++`
- destructor 末尾で `dtor_exit++`

4) atexit で 1回だけ dump

---

## 5) 調査手順

1. baseline 再現確認（HZ3_SUB4K_ENABLE=1）
2. S63 ON で再現 → atexit stats で停止位置を特定
3. 必要なら `gdb -p` で BT

---

## 6) GO / NO-GO

GO:
- どの境界で止まるか特定できる（dtor / pop / push）

NO-GO:
- counters が出ず原因位置が不明 → 追加の境界を 1 箇所だけ増やす

---

## 7) 次の分岐

- dtor 内ハング → lock 順序 / double-lock を疑う
- pop/push 無限待ち → central lock の競合 / lock 掴みっぱなしを疑う
- sub4k 以外 → TLS / remote / owner への波及を確認（別箱）
