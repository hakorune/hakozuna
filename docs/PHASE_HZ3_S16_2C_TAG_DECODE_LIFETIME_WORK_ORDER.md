# PHASE_HZ3_S16-2C: PageTag decode “lifetime 分割”（レジスタ圧低減）Work Order

目的:
- `mixed (16–32768)` の hot（特に `hz3_free`）で、命令数そのものより **レジスタ圧（spills）**が支配している可能性に対処する。
- `tag decode` を「必要になる直前」まで遅延し、**sc/owner の生存期間を短く**して生成コードを軽くする。

背景:
- S16-2（decode式を軽くする / layout変更）は NO-GO（“式を軽くすれば勝つ”タイプではない）。
- ただし `hz3_free` の hot 経路は `tag decode + kind dispatch + arena check` が支配的で、
  **decodeを“まとめて”やると live range が長くなりやすい**（= spill の温床）。

制約:
- allocator本体のノブは compile-time `-D` のみ
- hot path に per-op stats を入れない
- 1 commit = 1 purpose

参照:
- S16: `hakozuna/hz3/docs/PHASE_HZ3_S16_MIXED_INSN_REDUCTION_WORK_ORDER.md`
- Tag 定義: `hakozuna/hz3/include/hz3_arena.h`

---

## 方針（狙い撃ち）

現状（典型）:
- `hz3_pagetag_decode_with_kind(tag, &sc, &owner, &kind);`
- その後に `if(kind==...) { ... sc/owner ... } else if(kind==...) { ... }`

この形だと:
- `sc` と `owner` が **kind判定より前に確定**し、以後の分岐を跨いで生存しがち。
- コンパイラが spill すると、命令数/ロードストアが増える（branch/cacheより効く）。

対策:
- `tag` をまず “tag のまま” 持つ。
- `kind` だけ先に取り、**分岐の内側で sc/owner を取り出す**。

---

## 実装案（最小）

### 1) decode を “分割” する（関数分割ではなく inline のまま）

擬似コード（hot/free側）:

```
tag = hz3_pagetag_load(page_idx);
if (tag == 0) fallback;

kind = tag >> 14;
if (kind == PTAG_KIND_V2) {
    sc    = tag & 0xFF;
    owner = (tag >> 8) & 0x3F;
    ... v2 free ...
    return;
}
if (kind == PTAG_KIND_V1_MEDIUM) {
    sc    = tag & 0xFF;
    owner = (tag >> 8) & 0x3F;
    ... medium free ...
    return;
}
fallback;
```

ポイント:
- `sc/owner` の decode は “case の中だけ” に閉じる（live range を最短化）。
- `HZ3_PTAG_LAYOUT_V2=1` のときは `-1` が消えるので、さらに良い。

### 2) 可能なら `switch(kind)` に寄せる

`if/else` より switch の方が `kind` の live range が明確になりやすい。
（A/B: objdumpで実生成を見る。推測禁止）

---

## A/B 計測（必須）

### SSOT
- `hakozuna/hz3/scripts/run_bench_hz3_ssot.sh` を RUNS=10 で回す
- 目標:
  - `mixed` が +5% 以上（理想）
  - 最低ライン: `mixed` が +2% 以上、かつ `small/medium` が ±0%

### perf（mixed）
- `perf stat -e instructions,cycles,branches,branch-misses`（最低限）
- 期待:
  - instructions が減る、または同等でも cycles が減る
  - branch/cachesが同等で改善するなら spill/レジスタ圧が当たり

### objdump（推測禁止）
- `objdump -drwC` で `hz3_free`（または該当の inlined block）を確認し、
  - 余計な stack spill（`mov [rsp+...]` が増えていないか）
  - decodeの位置が分岐の内側に入ったか
  を見る。

---

## GO/NO-GO

GO:
- `mixed` が +2% 以上（まずは“効いた”確認）
- `small/medium` が ±0%（回帰なし）

NO-GO:
- `mixed` が ±1% 内で動かない
- `small/medium` が落ちる

NO-GO の場合:
- revert
- `hakozuna/hz3/archive/research/s16_2c_tag_decode_lifetime/README.md` に SSOTログと perf/objdump を固定
- `hakozuna/hz3/docs/NO_GO_SUMMARY.md` に追記

