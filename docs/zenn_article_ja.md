---
title: "「箱だ！」で設計したらmimalloc超えちゃった話"
emoji: "📦"
type: "tech"
topics: ["memory", "allocator", "performance", "cpp", "設計"]
published: false
---

# 「箱だ！」で設計したらmimalloc超えちゃった話

## はじめに

こんにちは。メモリアロケータを作ってたら、なんかmimalloc超えちゃいました。

使った設計哲学は **Box Theory（箱理論）**。すごくシンプルなやつです。

## Box Theoryって何？

よく聞かれるので、Q&A形式で説明しますね。

**Q: Box Theoryって何？**
A: 箱だよ

**Q: なぜ境界集約するの？**
A: 箱だから！境界作ったら責務が分かれて、デバッグの線引きができて、開発が加速するんだよね

**Q: なぜ可逆性が大事なの？**
A: 箱だからだよ！箱なら入れ替えればいいじゃん

**Q: NO-GOの判断基準は？**
A: ベンチ！数字が全て

...これだけです。シンプルでしょ？

## もうちょっと真面目に説明すると

Box Theoryには4つの原則があります。

### 1. 境界集約

ホットパス（速くしたいところ）と制御層（判断するところ）の境界を最小限に。

```
箱の中 = 高速処理。触らない。
箱の境界 = 制御・判断。ここだけいじる。
```

境界を1箇所に集約すると、「どこを変えればいいか」が明確になります。

### 2. 可逆性

最適化は全部フラグで切り替えられるようにしてます。

```bash
# 普通にビルド
make all_ldpreload_scale

# 新機能試したい時はフラグ追加するだけ
make all_ldpreload_scale HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_NEW_FEATURE=1'
```

ダメだったら戻せばOK。箱を入れ替えるだけ。

### 3. 可観測性

常時ログは重いので、終了時に一発だけ統計を吐きます（SSOT方式）。

これで再現性も確保できて、「あの時のあのビルド」が追跡できます。

### 4. Fail-Fast

バグは箱の境界で捕まえます。箱の中まで入れない。

おかしい状態を見つけたら即終了。デバッグが楽になります。

## hakozuna (hz3) の構造

で、この哲学でメモリアロケータ作ったのがこちら：

![アーキテクチャ図](https://raw.githubusercontent.com/hakorune/hakozuna/main/docs/images/architecture_overview.png)

3つの層（箱）に分かれてます：

| 層 | やること |
|----|----------|
| **Hot Path** | 最速で割り当て・解放。TLSで完結 |
| **Cache Layer** | バッファリング。Owner StashとかRemoteStash Ring |
| **Central Layer** | スレッド間で共有するところ |

各層が「箱」になってて、境界を超える時だけコストを払う設計です。

## ベンチマーク結果

で、どうなったかというと：

| ベンチマーク | 条件 | vs mimalloc |
|-------------|------|-------------|
| **Larson** | T=8-16 | **+15%** 🎉 |
| **memcached** | T=4 | **+10%** 🎉 |
| **MT remote** | T=8 R=90% | **+28%** 🎉 |
| random_mixed | - | 同じくらい |

特に「remote-free」（スレッドAで確保してスレッドBで解放）が多い状況で強いです。

### なんで勝てたの？

正直、全部わかってるわけじゃないんですが...

- **Owner Stash**: remote freeを一旦バッファリングしてmutex競合を避けた
- **RemoteStash Ring**: TLSのサイズを92%削減
- **Full Drain Exchange**: atomic exchangeで一括回収

箱で分離してたから、それぞれ独立に最適化できたのが大きいかなと。

## 失敗もいっぱいした（NO-GO事例）

Box Theoryのいいところは「戻せる」こと。

実は20件以上の最適化が不採用（NO-GO）になってます：

| やろうとしたこと | 結果 | 学び |
|-----------------|------|------|
| SegMath最適化 | NO-GO | 「絶対速いでしょ」が実測で否定された |
| Page-Local Remote | NO-GO | 合成ベンチだけ速くて他が遅くなった |
| PGO（プロファイル最適化） | NO-GO | 条件変わると逆に遅くなった |

**ベンチで負けたら戻す。** これだけ守ってます。

## hakoruneプロジェクトについて

hakozuna (hz3) は [hakorune](https://github.com/hakorune) プロジェクトの一部です。

hakoruneはBox Theoryで作ってるプログラミング言語で、MIR変換後にVMでもLLVMでも動きます。

コンパイラは...まだ苦戦中です（笑）

## まとめ

```
Q: どうやってmimalloc超えたの？
A: 箱で分けたら超えちゃった

Q: 設計の判断基準は？
A: ベンチ！

Q: うまくいかなかったら？
A: 箱を入れ替えるだけ
```

シンプルな哲学でも、ちゃんと結果が出るんだなーと思いました。

## リンク

- **GitHub**: https://github.com/hakorune/hakozuna
- **論文 (日本語)**: https://github.com/hakorune/hakozuna/blob/main/docs/paper/main_ja.pdf
- **論文 (English)**: https://github.com/hakorune/hakozuna/blob/main/docs/paper/main_en.pdf

---

*ちなみにこのプロジェクト、AIのClaudeと一緒に作りました。設計哲学は僕、実装はAI。これも一種の「箱」...かもしれない？*
