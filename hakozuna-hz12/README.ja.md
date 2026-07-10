# Hakozuna HZ12

HZ12 は advisory ownership と再生可能な span を研究する、Windows-first の
standalone allocator research line です。公開推奨allocatorでもHZ8の置換でも
ありません。初期span coreはHZ11の実証済みWindows baselineをHZ12フォルダ内へ
複製・`hz12_*`化したものですが、HZ12はHZ11のsourceをcompile/linkしません。

```text
HZ8:
  公開推奨のbalanced allocator
  低RSS、fail-closed ownership、実用的なcross-thread safety

HZ11:
  speed-firstのownerless front-cache / transfer-cache / span研究

HZ12:
  standalone HZ12 span substrateとhz12_* API
  flush粒度のadvisory ownership
  bounded owner inboxとreclaimable span
  低RSSを主目的、cross-owner throughputを副証拠にする研究
```

HZ12ではfreeごとにownerを調べません。ownershipは安全性のauthorityでは
なく、flush/drain/reclaimのcold pathでだけ使うlocality metadataです。

最初に読むものは `current_task.md` と `docs/` のL0文書です。

L2-B ではretired ownerのbounded inboxをadopterがownerless HZ12 pathへ
引き渡せることを専用smokeで確認しています。span reclaim/decommitはまだ
有効化していません。

L2-Cではspanごとのalloc/free/liveを診断しています。mixed workloadでは
live=0でもpartial spanが残るため、これだけを根拠にreclaimしないことも
確認済みです。

sourceはowner shadow、bounded inbox、span accounting、read-only reclaim
gateを別moduleに分離しています。L2-Dではfull spanをfreeしてもfront cache
256、returned sink 768、current span参照とrouteが残るため、gateを閉じたまま
にできることを確認しています。

L2-Eでは専用single-thread smokeに限り、front cache、returned sink、current
span、routeの順にdetachし、gateが開くことを確認しました。decommit/reuseは
まだ有効化していません。

L2-Fではdetach済み64 KiB spanをWindows `MEM_DECOMMIT`し、payloadが
`MEM_COMMIT`から`MEM_RESERVE`へ変わることを確認しました。arena予約は維持し、
reuse/recommitはまだ実装していません。

L2-Gでは固定64-entry depotを追加し、同じaddressをrecommitしてからaccounting、
route、current spanを順に復元できることを専用smokeで確認しました。通常の
malloc/free laneにはまだ接続していません。

L2-Hでは同一process内8-cycleと64-entry cap境界を確認しました。65個目だけが
`put_full`となり、depotは上限64を超えません。counterはdepot slow path専用です。

L3-Aではgeneration付きowner registryを追加しました。4-thread repeat-20で
ACTIVE/RETIRING/DEAD、slot再利用、旧tokenのstale publish拒否を確認しています。
通常のmalloc/freeやowner inboxにはまだ接続していません。

L3-Bでは`slot + generation`にbindする診断専用token inboxを追加しました。2 producer
のrepeat-20で、ACTIVE/RETIRING batchの受理、DEAD/stale batchのownerless fallback、
drain後のreplacement generation再bindを確認しています。通常の`hz12_malloc`/
`hz12_free`はowner lookupもtoken counterも実行しません。

L3-Cではretire開始時に登録済みproducerのslot/generationを固定するowner epochを
追加しました。repeat-20で全producerがepochをackするまではDEADに進めず、ack後にだけ
`RETIRING -> DEAD`となることを確認しています。これはcold-path専用の診断coordinatorで、
通常allocatorのbarrierにはしません。

L3-Dではowner epochとtoken inboxの空状態を合わせたretire gateを追加しました。
repeat-20でcheckpoint前、checkpoint後でもfinal-flush objectが残る間は閉じ、正確な
inbox drain後にだけ開くことを確認しています。通常のallocation/free経路には未接続です。

L4-Aでは2 producerが稼働中にtoken inboxへpublishしownerがdrainした後、final batchと
checkpointを経てretireするpaced live diagnosticを追加しました。repeat-5でoverflow、
registry reject、generation rejectはいずれも0でした。性能測定や通常allocatorの挙動ではなく、
lifecycle evidenceとしてのみ扱います。

L4-Bでは1 producer / 1 consumer・100% cross-ownerのtoken-aware pipelineを
追加しました。repeat-5でtoken rejectとoverflowは0でしたが、overflow-free controlの
eager drainによりowner-id L1 controlより約13%遅くなりました。安全なtoken routingの
evidenceとして保持し、性能profileには昇格しません。

HZ11は独立したselected lineとして固定します。ownership/adoption/reclaimの
以後の変更はすべてHZ12内で行い、HZ11へ戻しません。
