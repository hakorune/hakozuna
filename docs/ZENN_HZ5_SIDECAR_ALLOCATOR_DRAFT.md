# Hakozuna HZ5: page/run-first な sidecar allocator 試作の設計メモ

この記事では、Cで実装しているメモリアロケータ実験プロジェクト **Hakozuna** の新しい試作系である **HZ5** について、設計方針を整理します。

Hakozuna はこれまで HZ3 / HZ4 という allocator profile を中心に育ててきました。HZ5 はその延長にありますが、単に既存の fast path を少し速くするためのブランチではありません。

HZ5 では、allocator の内部設計をもう一度見直すために、次のような方向を試しています。

- page/run-first な分類
- sidecar metadata
- fail-closed な ownership 判定
- descriptor-owned front-end
- page-oriented remote free
- profile-specific allocator lanes
- Linux / Windows の実験的な build / benchmark path

完成品というより、allocator 設計を再検討するための実験台です。

## HZ3 / HZ4 / HZ5 の違い

Hakozuna には、大きく分けていくつかの allocator profile があります。

まず **HZ3 / ACE-Alloc** は、local-heavy な allocation workload を主に意識した profile です。compact memory use や、PTAG32 による O(1) pointer-to-bin lookup などを中心にしています。

次に **HZ4** は、message-passing / remote-free profile です。remote-heavy な workload や thread 数が多いケースを意識しています。

これに対して **HZ5** は、page/run-first な sidecar allocator prototype です。

ざっくり書くと、こういう違いです。

```text
HZ3 / HZ4:
  既存 profile を磨く
  small object path や remote-free path を詰める

HZ5:
  分類、所有権、metadata 配置をもう一度考える
  page/run と descriptor を中心に設計する
```

HZ3 / HZ4 が「既存の profile を強くする」方向だとすると、HZ5 は「allocator の内部構造を別の切り口から組み直す」ための試作です。

## HZ5 の設計方針

HZ5 の中心にあるのは、**pointer を page/run 単位で分類し、その情報を sidecar descriptor に寄せる**という考え方です。

free するとき、allocator は少なくとも次のような情報を知る必要があります。

- この pointer は自分の allocator が所有しているか
- どの size class / run / page に属しているか
- local に返せるのか、remote free として扱うべきか
- どの front-end / profile に dispatch するべきか

HZ5 では、この判断を pointer そのものの近くに無理やり押し込むのではなく、page/run に対応する descriptor を中心に扱う方向を試しています。

イメージとしてはこうです。

```text
user pointer
   |
   v
page / run lookup
   |
   v
sidecar descriptor
   |
   +--> owner
   +--> profile
   +--> size class
   +--> front-end policy
   |
   v
allocation / free dispatch
```

ここで重要なのは、HZ5 が「ただ metadata を外に置く」だけの設計ではないことです。

metadata を sidecar に寄せることで、ownership、profile、dispatch policy を descriptor 側に集められます。つまり、allocator の各 front-end が勝手に判断するのではなく、descriptor を通して「この pointer をどう扱うべきか」を決めやすくします。

## なぜ sidecar metadata なのか

allocator で難しいところのひとつは、`free(ptr)` のときです。

`malloc(size)` のときは、要求 size が明示されています。しかし `free(ptr)` のときは、渡されるのは pointer だけです。allocator はその pointer から、所有者や size class や返却先を復元しなければなりません。

このため、多くの allocator では metadata の置き方が重要になります。

metadata を user allocation の近くに置く設計もあります。これは局所性や実装の単純さで有利なことがあります。一方で、HZ5 では page/run 単位の descriptor を使い、pointer から page/run を引いて、その descriptor を見に行く方向を試しています。

この設計で重視しているのは、主に次の点です。

- pointer の分類を page/run 単位で明確にする
- ownership の判断を descriptor に集約する
- profile ごとの policy dispatch を整理する
- 不明な pointer や曖昧な状態では fail-closed に倒す

ここでいう fail-closed は、「よくわからないけれど進める」ではなく、「所有権が確認できないなら安全側に倒す」という考え方です。

高速な allocator では、少しでも余分な分岐や lookup を減らしたくなります。しかし、allocator の ownership 判定は壊れると影響が大きい場所です。HZ5 では、速さだけでなく、分類の明確さも設計対象にしています。

## Windows port の現在地

HZ5 は Linux 側の allocator profile 実験から始まっていますが、現在は Windows native build / benchmark path も整備しています。

ただし、ここは慎重に書いておきたいところです。

HZ5 はまだ experimental です。Windows 全体で汎用的に既存 allocator を置き換える、という段階ではありません。

今の位置づけは、次のようなものです。

- Windows native build の導線を整備する
- Windows 上でも profile-specific な benchmark を取れるようにする
- remote-heavy / local-heavy / mixed など、workload ごとの傾向を見る
- Linux で見ていた設計が Windows でどう変わるかを観察する

特に allocator benchmark は環境差が大きいです。

OS、compiler、thread 数、allocation size 分布、remote free の比率、RSS の見方などで結果が変わります。そのため、HZ5 の結果を見るときは「Windows で全部速い」という言い方ではなく、「この profile / この benchmark lane ではこういう傾向がある」という形で扱うのが安全です。

## benchmark を見るときの注意

allocator の benchmark は、1つの数字だけで判断しにくいです。

throughput が高くても RSS が大きすぎることがあります。local-heavy な workload では速くても、remote-free が増えると急に苦しくなることがあります。small object では良くても、large allocation や mixed workload で違う傾向になることもあります。

そのため、HZ5 では benchmark を見るときに、少なくとも次の軸を分けて考えています。

- local-heavy か remote-heavy か
- small object 中心か mixed size か
- throughput を見るのか RSS を見るのか
- thread 数を増やしたときにどう変わるか
- Linux と Windows で同じ傾向になるか

allocator の評価で危ないのは、「この1ケースで速かったので最強」と言ってしまうことです。

HZ5 は、そういう主張をするためのものではありません。むしろ、profile ごとに allocator の形を変えると何が起きるかを見るための実験です。

## DOI を付けた理由

今回、HZ5 には Zenodo で DOI を付けました。

- HZ5 Zenodo record: https://zenodo.org/records/20411598
- HZ5 DOI: https://doi.org/10.5281/zenodo.20411598
- HZ5 all-version DOI: https://doi.org/10.5281/zenodo.20411597

また、HZ3 / HZ4 側にも別の artifact DOI があります。

- HZ3/HZ4 Zenodo record: https://zenodo.org/records/20411402
- HZ3/HZ4 DOI: https://doi.org/10.5281/zenodo.20411402
- HZ3/HZ4 all-version DOI: https://doi.org/10.5281/zenodo.18305952

GitHub repository は日々更新されます。これは開発には便利ですが、「この時点の paper / source / artifact を引用したい」という用途では少し困ります。

Zenodo DOI を付けると、その時点の成果物を固定できます。

今回の HZ5 artifact には、主に次のものを含めています。

- HZ5 source
- design notes
- benchmark / reproducibility artifacts
- English paper PDF
- Japanese paper PDF

HZ3/HZ4 と HZ5 を同じ DOI に混ぜなかったのも意図的です。

HZ3/HZ4 は既存 profile の成熟と比較を中心にした artifact です。一方、HZ5 は sidecar allocator prototype として別の設計線を持っています。読者や引用者にとっても、分けておいた方が意味が明確になります。

## 今後

HZ5 は、これで完成というより、ようやく外から見える形に整理できた段階です。

今後やりたいことは、主に次のあたりです。

- Windows HZ5 path の整理
- Linux / Windows benchmark coverage の追加
- profile ごとの安定性分類
- HZ3 / HZ4 との比較整理
- paper / README / artifact の同期
- 再現手順のさらなる整理

allocator は、細かい実装差で大きく挙動が変わる領域です。だからこそ、単に「速い」と言うより、どの workload で、どの profile が、どの制約のもとで有効なのかを整理する必要があります。

HZ5 は、そのための実験台です。

page/run-first な分類、sidecar descriptor、fail-closed ownership、profile-specific dispatch。これらがどこまで有効かは、まだ検証が必要です。

ただ、少なくとも今回の整理で、HZ5 は「リポジトリの中にある試作フォルダ」から、「設計意図と DOI を持った research artifact」になりました。

ここからまた、少しずつ育てていきます。

