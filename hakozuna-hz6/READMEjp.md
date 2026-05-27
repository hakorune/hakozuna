# Hakozuna HZ6

HZ6 は、HZ5 の sidecar allocator 研究を引き継ぐ transfer-first の後継です。
いまは R1 の実行可能な seed まで進んでいて、契約モジュール、toy の contract
validation front、Large128 の transfer-first front seed があります。
HZ6 の最初のベンチマーク実行は済ませましたが、HZ3/HZ4/HZ5 を含む
横並び表はまだ出していません。

英語版 README: [README.md](README.md)

## 位置づけ

```text
HZ3:
  thin local/TLS speed ideas

HZ4:
  remote handoff and page/span cache ideas

HZ5:
  descriptor ownership, fail-closed route safety, and low-RSS profiles

HZ6:
  route-safe / transfer-first / RSS-aware allocator family
```

HZ6 は HZ3 / HZ4 / HZ5 の内部実装をそのまま移植するものではなく、
有効だった考え方を最初からレイヤとして分けて扱う設計です。

## 核となる方針

- Route lookup は最初の safety gate にする
- Remote free は fast profile では bounded transfer cache を優先する
- Owner inbox は strict / fallback / owner-bound profile 用に残す
- Hot path では diagnostic counter や policy state を読まない
- Refill / drain / source / release / policy は slow path に置く
- Fail-closed ownership は維持するが、fast remote profile では batch
  boundary の検証を使ってよい
- RSS の制御は設計の中心に置く

## R1 Smoke

最初のコードは小さく保っています。ここで見ているのは性能ではなく、
モジュール境界と transfer-first の state transition です。

```bash
./hakozuna-hz6/linux/build_hz6_r1_smokes.sh
```

期待する出力:

```text
hz6-r1-core-contract-smoke ok
hz6-r1-route-smoke ok
hz6-r1-transfer-contract-smoke ok
hz6-r1-source-contract-smoke ok
hz6-r1-allocator-smoke ok
hz6-r1-prefill-smoke ok
hz6-r1-sourceblock-smoke ok
hz6-r1-transfer-smoke ok
hz6-r1-reclaim-smoke ok
hz6-r1-safety-smoke ok
```

## ベンチ状況

HZ6 には最初の benchmark run がありますが、まだ HZ3 / HZ4 / HZ5 を含む
比較表はありません。R1 seed から性能結論を出す段階ではないので、次は同じ
マシン・同じ runner で横並び比較するのがよいです。

Snapshot:

```text
docs/HZ6_R1_BENCHMARK_20260528.md
docs/HZ6_R1_BROAD_TRENDS_20260528.md
```

いまの R1 傾向:

```text
strict:
  HZ6-only runner の local-only では一番強い

rss:
  HZ6-only runner の remote / reuse では全サイズで一番強い

speed / remote:
  profile の意図は残すが、現 R1 の knob では broad single-process sweep で
  rss をまだ超えていない
```

## サイズ対応の境界

R1 はまだ完成した large-object allocator ではありません。現時点では、
実行可能な設計 seed として次の front 境界を持っています。

```text
toy:
  <= 4KiB contract-validation front

midpage:
  >4KiB..32KiB seed front

local2p:
  exact 64KiB seed front

large128:
  >32KiB..128KiB except exact 64KiB seed front

>128KiB:
  R1 では未対応
```

そのため次の HZ6 設計は、「すべての large size を数値調整する」ではなく、
Large128 seed を LargeSpan family に育てるのが筋です。

```text
L1:
  128K ownerless CentralSpanPool を proof target として固める

L2:
  256K / 512K / 1M span class を同じ RouteLayer,
  TransferLayer, SourceLayer, ScavengeLayer contract の上に追加する

L3:
  ordinary malloc preload coverage を作り、HZ3/HZ4/HZ5 と横並びで測る
```

L2 までは、HZ6 の大きなサイズの結果は `Large128 seed` として扱い、
広い large-object allocator の主張にはしない方針です。

## 非目標

- HZ3 / HZ4 / HZ5 の実装ファイルをそのまま HZ6 の出発点にしない
- `VirtualQuery` のような OS query を hot free path に置かない
- いきなり単一の universal profile を成功条件にしない
- owned-invalid / double-free の失敗を silent fallback にしない
