# S21: PTAG32（dst/bin）一本で `free/realloc/usable_size` を完結させる（箱の統一）

## 目的

S17 で **`free()` の hot path** は `range check → PTAG32 load → dst/bin push` まで削れた。
次は「設計の正（ptr分類）」を増やし、**`realloc()` / `hz3_usable_size()` も PTAG32 だけで完結**させる。

狙い:
- ptr分類の正を 1 本化（`range check → tag32 load → bin`）
- `realloc/usable_size` の segmap/PTAG16/segmeta 参照を減らす（固定費削減 + SSOTの単純化）
- “増やして統一”はしない（箱を厚くしない）

制約（SSOT）:
- allocator 挙動の切替は **compile-time `-D` のみ**（envノブ禁止）
- hot path を太らせない（共有ロック/統計/追加lookupは event-only へ）
- false positive（他アロケータ ptr を hz3 と誤認）は NG
  - false negative（hz3 ptr を “わからない” 扱いで fallback）は OK（ただし leak/no-op になる箇所は failfast を優先）

---

## 現状（S17/S18-1の到達点）

### すでに統一できているもの
- `hz3_free()` は PTAG32（dst/bin直結）で「push先」を即決できる（local/remote分岐も event-only flush 側へ押し出し済み）。

### まだ混在しているもの（解消対象）
- `hz3_realloc()` と `hz3_usable_size()` は、経路によって PTAG16 / segmap / segmeta / large box 判定に分岐している。
  - v2-only / PTAG32 で “どこに push するか” は分かるが、`realloc/usable_size` では「サイズ復元」が必要で混在しがち。

---

## コア設計（PTAG32からサイズを復元する）

前提:
- `HZ3_BIN_TOTAL == HZ3_SMALL_NUM_SC + HZ3_NUM_SC`
  - 例（現状）: `HZ3_SMALL_NUM_SC=128`, `HZ3_NUM_SC=8` → `HZ3_BIN_TOTAL=136`

### bin→kind
- `bin < HZ3_SMALL_NUM_SC` → small（16–2048 の small sc）
- `bin >= HZ3_SMALL_NUM_SC && bin < HZ3_BIN_TOTAL` → medium（4KB–32KB の sc）

### bin→usable_size（復元）
- small: `size = hz3_small_sc_to_size(bin)`
- medium: `sc = bin - HZ3_SMALL_NUM_SC`, `size = hz3_sc_to_size(sc)`

※ `dst` は `free` では必要だが、`usable_size/realloc` の「サイズ復元」には不要（ただし debug invariant 用には使って良い）。

---

## 実装方針（Box Theory）

### Hot-only（増やさない）
- `ArenaRangeCheckBox` と `PTAG32LoadBox` は **既存 helper に集約**し、`realloc/usable_size` も同じ helper を呼ぶ。
- `bin→size` は **static inline** で、枝とロードを増やさない（テーブル化は後段）。

### Event-only（触らない／または set/clear の整合だけ見る）
- tag32 の set/clear は、これまで通り「ページを hz3 が管理し始める瞬間に set、破棄/返却で clear」。
- `realloc/usable_size` は “読むだけ”。

---

## 追加するAPI（推奨）

`hakmem/hakozuna/hz3/include/hz3_arena.h`:
- `hz3_pagetag32_lookup_fast(...)` を **free/realloc/usable_size で共通利用**（すでに存在するなら再利用）

`hakmem/hakozuna/hz3/include/hz3_tcache.h` または新規 `hz3_bin.h`:
- `static inline int hz3_bin_to_usable_size(uint16_t bin, size_t* out)`
  - `bin` が範囲外なら 0（failfast時は abort でも可）

---

## 実装手順（1 commit = 1 purpose）

### S21-1: `bin→usable_size` の SSOT を追加（挙動変更なし）

目的:
- “binからサイズが一意に復元できる” をコード上の SSOT にする。

内容:
- `hz3_bin_to_usable_size(bin)`（inline）追加
- debug で `bin < HZ3_BIN_TOTAL` の static_assert / failfast

GO:
- `make -C hakmem/hakozuna/hz3 all_ldpreload` が通る

---

### S21-2: `hz3_usable_size()` を PTAG32-first にする（A/B gate）

新フラグ案:
- `HZ3_USABLE_SIZE_PTAG32=0/1`（default 0）

変更:
1. `ptr==NULL` → 0
2. `tag32 lookup` 成功 → `bin` 抽出 → `bin→size` を返す
3. `in_range && tag32==0` は **バグ**として扱う
   - `HZ3_PTAG_FAILFAST=1` なら abort
   - release は `0` を返す（安全側、ただし “ありえない” なのでログは不要）
4. `not in arena` は large box を試して 0（or 既存実装踏襲）

GO/NO-GO:
- SSOTで性能は見ない（usable_size はベンチに入らないことが多い）
- correctness: smoke（realloc/hybrid用）で crash しない

---

### S21-3: `hz3_realloc()` を PTAG32-first にする（A/B gate）

新フラグ案:
- `HZ3_REALLOC_PTAG32=0/1`（default 0）

変更（概念）:
1. `realloc(NULL,n)` / `realloc(p,0)` は既存規約通り
2. PTAG32 lookup 成功 → `old_size = bin→size`
   - `size <= old_size` → `ptr` を返す
   - `size > old_size` → `new=hz3_malloc(size)` → `memcpy(min(old_size,size))` → `hz3_free(ptr)`
3. `in_range && tag32==0` は **バグ**
   - debug: abort
   - release: `NULL` を返して停止側に倒す（foreign realloc に渡さない）
4. arena外:
   - `hz3_large_usable_size(ptr)` が非0なら large realloc path
   - それ以外は `hz3_next_realloc(ptr,size)`

GO/NO-GO:
- `hz3` preload + 代表的バイナリ（/bin/ls 等）で crash しない
- small/medium/mixed SSOT を 1回だけ回し（RUNS=3程度）、明確な退行がないこと（性能よりまず正しさ）

---

## 収束（統一）フェーズ（最後にやる）

S21-2/3 が安定したら:
- `HZ3_SMALL_V2_PTAG_ENABLE`（PTAG16）や segmap 参照を、`usable_size/realloc` から段階的に外す。
- “増やして統一”はしない（PTAG32で十分になった瞬間に PTAG16側を削る）。

---

## 計測（統一が効いたかの確認）

統一の効果は性能より「設計の正が一本になったか」で見る。

チェック項目:
- `hz3_free` / `hz3_realloc` / `hz3_usable_size` が “arena in-range” のとき **同じ lookup helper**を通る
- `realloc/usable_size` で segmap を踏むケースが “arena外” に限定される
- failfast は debug のみ（hot に統計/ログは入れない）

---

## 現状ステータス（重要）

- `HZ3_USABLE_SIZE_PTAG32=1` / `HZ3_REALLOC_PTAG32=1` は **まだデフォルト化しない**（A/B用）。
- 以前は `LD_PRELOAD=./libhakozuna_hz3_ldpreload.so /bin/ls` segfault の事例があったため、実アプリ互換（foreign pointer / realloc / loader再入）を先に安定化する必要がある。
- 安定化（S21-2）で `in_range && tag32==0` を “停止側（NULL/0）” ではなく **slow path fallthrough** に変更し、`/bin/ls` 等の smoke が通る状態になった。
  - 指示書: `hakozuna/hz3/docs/PHASE_HZ3_S21_2_PTAG32_REALLOC_USABLESIZE_STABILITY_WORK_ORDER.md`

## 参考

- S17: PTAG dst/bin direct（`free` hot を最短化）
- S18-1: FASTLOOKUP（range check + tag load を 1-shot helper）
- S20-1: PTAG prefetch（NO-GO）→ “hot +1命令”は small を落としやすい（教訓）
