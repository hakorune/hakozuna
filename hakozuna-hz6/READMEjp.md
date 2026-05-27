# Hakozuna HZ6

HZ6 は、HZ5 の sidecar allocator 研究を引き継ぐ transfer-first の後継です。
いまは R1 の実行可能な seed まで進んでいて、契約モジュール、toy の contract
validation front、Large128 の transfer-first front seed があります。
ただし、まだ HZ6 のベンチマークは一度も実施していません。現状は smoke-only
です。

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

HZ6 はまだベンチマークしていません。今の結果は smoke-only です。
R1 seed から性能結論を出さず、まずは prototype path を固定してから、HZ3 / HZ4 /
HZ5 と同じマシンで比較するのがよいです。

## 非目標

- HZ3 / HZ4 / HZ5 の実装ファイルをそのまま HZ6 の出発点にしない
- `VirtualQuery` のような OS query を hot free path に置かない
- いきなり単一の universal profile を成功条件にしない
- owned-invalid / double-free の失敗を silent fallback にしない
