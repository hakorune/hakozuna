# ChatGPT Pro Prompt: S100 FreeLeaf/FastMeta（mimalloc vs hz3_free）

目的:
- `hz3_free()` の hot path の固定費（レジスタ圧/間接参照/分岐）を落として **leaf に寄せる**。
- 指標: `perf annotate` で `hz3_free` の prologue/epilogue（push/pop）や call を減らし、
  mt-remote / cache-thrash/scratch(T=16) で退行なく改善。

観測（要点）:
- `hz3_free` は `push %r12/%rbp/%rbx` を含む prologue/epilogue があり、`mimalloc free` は leaf っぽい（push無し）。
- hz3 は metadata lookup が `arena_base -> page_idx -> ptag32 load -> dst/bin/sc` という **別テーブル + 間接参照**で、
  mimalloc は pointer math で segment を逆算し、segment header 参照へ直行している。

制約（箱理論）:
- 変更は “箱” 化し、境界は `hz3_free()` 冒頭（またはその直下の helper 1箇所）に集約する。
- いつでも戻せる（compile-time flag で A/B）。
- 観測は atexit 1行の SSOT（常時ログ禁止）。
- fail-fast は opt-in（release 既定に混ぜない）。

---

## 提案したい方向（短期）

### A) FreeLeafBox: slow path を cold/noinline に追放して leaf 化

狙い:
- hot path で callee-saved を使わない（= push/pop を消す）ために、call を含む経路をまとめて “外” に逃がす。

候補:
- `hz3_free()` は
  - 最初に **最頻の成功パスだけ**をインラインで完結
  - miss / not-inited / remote / large / fallback は `__attribute__((noinline,cold))` の helper にまとめて `goto slow;`

質問:
1) Cコードの形（if順序、変数のスコープ、helper分割、attribute）として、
   GCC/Clang が leaf っぽいコード生成をしやすい “定石” は？
2) `__builtin_expect` の入れ方や、`likely/unlikely` マクロの位置はどこが効く？
3) “hot path を leaf にしたいが、同一関数内に call が残るとレジスタ保存が入る” 問題を、
   どう分割すれば避けられる？（例: tail-call、goto slow、return早期化、LTO前提など）

### B) FastMetaBox: PTAG lookup を踏まない “page header” 判定の fast path

hz3 には small_v2 page header があり、ページ先頭に `magic/owner/sc` を保持している。

候補案:
- `hz3_free()` 冒頭で
  - arena in-range を cheap に確認（既存の range check でもOK）
  - `page_base = ptr & ~(page_size-1)` → `Hz3SmallV2PageHdr* ph = (Hz3SmallV2PageHdr*)page_base`
  - `if (ph->magic == HZ3_PAGE_MAGIC)` なら small_v2 と確定して
    - `owner/sc` を ph から読み、local なら TLS bin push、remote なら remote stash push（または small_v2_free_remote_slow）
  - 外れたら従来の PTAG32 lookup へフォールバック

質問:
4) この “page header 先行判定” は一般に有利？（PTAGテーブル間接参照 vs 同ページ先頭ロード）
5) `free(ptr)` が他アロケータ由来（arena外）でも安全にしたい。range check + magic check で十分？
6) `Hz3SmallV2PageHdr` を読むことで D-cache miss が増える懸念は？（free対象がコールドな場合）

---

## こちらの現状コード（参照箇所）

- `hz3_free` 実装（巨大な compile-time 分岐）:
  - `hakozuna/hz3/src/hz3_hot.c`
- PTAG32 fast lookup:
  - `hakozuna/hz3/include/hz3_arena.h` の `hz3_pagetag32_lookup_hit_fast()`
- small_v2 の page header & free fast path:
  - `hakozuna/hz3/src/hz3_small_v2.c` の `Hz3SmallV2PageHdr` と `hz3_small_v2_free_fast()`
- remote stash push:
  - `hakozuna/hz3/src/hz3_remote_stash.c`

---

## hz3_free の annotate 抜粋（例）

（`perf annotate --symbol hz3_free --stdio` の抜粋。prologueで callee-saved 保存が入っている）

```
0000000000005090 <hz3_free>:
  test %rdi,%rdi
  je   ...
  push %r12
  mov  %rdi,%r12
  push %rbp
  push %rbx
  ...
  mov  (%rdx,%rax,4),%ebx   ; ptag32 load
  ...
  jne  remote_path
  mov  %fs:...,%rax
  mov  %rax,(%r12)          ; obj->next = old_head
  addl $0x1,%fs:...         ; count++
  mov  %r12,%fs:...         ; head = obj
  pop  %rbx
  pop  %rbp
  pop  %r12
  ret
```

この push/pop を消したい（または削減したい）。

---

## 期待する答え

- leaf 化のための C 構造（具体的な分割案、attributeの使い分け、変数の寿命の切り方）
- page header fast path の安全設計（range/magic/owner/scの扱い、フォールバック設計）
- A/B の測り方（perf annotateで push/pop が消えた確認、run順ドリフト回避など）

