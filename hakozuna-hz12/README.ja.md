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

L4-Cではtoken drain intervalをsweepしました。intervalを広げると見かけの速度は
上がりますが、Windowsではbounded fallbackを再現性よく0にできませんでした。これは
NO-GO policy evidenceとして固定し、HZ12のdefaultには昇格しません。

L5-Aではgeneration-awareなspan-owner side tableとread-only reclaim scanを追加しました。
repeat-20でforeign-cache scope不明時は0 bytes、既存L2 gateが開いた後だけ正確に64 KiB、
owner slot再利用後の旧generation spanはstaleとして0 bytesになることを確認しています。

wide_ws-like診断ではphysical full-span candidateが中央値72.88 MiB、peak RSSが
中央値82.00 MiBでした。foreign cache scopeは未証明なので、完全にreclaimableなbytesは
引き続き0として扱います。

L5-Bでは全workerが明示class-flush checkpointとepoch ackを実行します。wide_ws-like
repeat-5でforeign-cache blockerは0となり、physical candidateは中央値80.75 MiBでした。
残るblockerはreturned sinkとrouteで、reclaimable bytesはまだ0です。

L5-Cではquiescentな64-span固定budgetを追加しました。repeat-3で毎回64/64 span、
失敗0、正確に4.00 MiBをreclaimableへ移しました。decommit/depotはまだ実行しません。

L5-Dでは同じ64 spanをdecommitしました。repeat-3で毎回4.00 MiBを解放し、working-set
RSSは毎回3.99 MiB低下しました。depotと自動policyはまだ有効化していません。

L5-Eではその固定64 spanだけを既存depotへ接続します。同一addressでのrecommit、
accounting reset、route復元、owner generation一致、cycle後のdepot空状態を必須とし、
通常のallocation/free policyには接続しません。

Windows repeat-3では毎回put/take 64/64、4.00 MiB recommit、address mismatch 0、
owner-generation mismatch 0、depot残数0でした。これはbounded lifecycleの証拠であり、
自動depot policyではありません。

L5-Fでは二周目cycleまでrepeat-3で完了しました。64 spanすべてをfull carve、touch、
通常free、再detach、再decommitし、depotへ戻しました。各runでclass構成に応じた
9,984..10,048 slotをtouchし、再び4.00 MiBをdecommit、address/generation/lifecycle
failureはすべて0でした。Windowsのbounded mechanismは完成し、自動reclaim policyは
別gateとして残します。

HZ11は独立したselected lineとして固定します。ownership/adoption/reclaimの
以後の変更はすべてHZ12内で行い、HZ11へ戻しません。
