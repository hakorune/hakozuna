# HZ12 Current Task

## Restart Surface: ColdSpanOwner-L1 GO Candidate

The public HZ12 core remains unchanged. A Windows-only opt-in prototype now
records advisory ownership on allocation, keeps normal free owner-blind, and
groups ownership only when a class cache flushes. Its dedicated owner-by-class
inbox fixed the unsafe reuse of the deferred-free inbox and passed the 100%
cross-owner pipeline.

```text
local random_mixed R5:
  core                         about 153..155M ops/s
  allocation owner map        about 149..153M (-1.6..-2.8%)
  integrated flush routing    about 139..141M (about -7% total)

xowner R5, 4 producer / 4 consumer:
  HZ11 ownerless              12.939M / 6.61 MiB peak
  HZ12 integrated flushroute  26.128M / 11.79 MiB peak
  HZ12 OwnerFast upper bound  34.324M / 15.80 MiB peak
  tcmalloc                    36.318M / 13.81 MiB peak
```

FlushTimeOwnerRouting-L1 remains HOLD. ColdSpanOwner-L1 moves owner assignment
to 64 KiB current-span acquisition and drains only at current-span replacement.
It restored local random_mixed to within -1.4..-2.5% of core and reached
29.064M ops/s with 11.32 MiB peak RSS in the fixed xowner R5. This is 11.1%
faster than the first integrated route and 25.2% lower peak RSS than tcmalloc.

ColdSpanOwner-L2 closes the first lifetime blocker with generation-tagged slots
and Windows FLS teardown. A 128-thread sequential churn smoke recorded 128
attaches, 127 reuses, zero full-table events, 128 detaches, and zero stale
fallbacks. The post-lifetime local R5 remains within -1.1..-1.7% of core; the
xowner R3 reached 29.194M with 12.01 MiB peak RSS versus tcmalloc at 38.089M
and 14.19 MiB.

ColdSpanOwner-L3 also passed the concurrent publish-versus-retire smoke for 20
rounds: 120 attaches, 116 generation-slot reuses, zero full-table events, and
120 matching detaches. Publishing and detach serialize on the target inbox;
stale/inactive targets downgrade to ownerless recycling.

The calibrated stable-duration MT R3 closes promotion as NO-GO. ColdSpanOwner
improves HZ12 core to 59.380M balanced, 32.086M wide_ws, and 74.075M
larger_sizes, but tcmalloc reaches 523.964M, 441.749M, and 245.415M. HZ12 peak
RSS is also 4.5..19.8% higher on these rows.

Decision: keep ColdSpanOwner as opt-in internal HZ12 improvement; do not
promote it or claim a tcmalloc Pareto win. If continuing, allow one diagnostic
pass on balanced returned-sink lock/refill traffic. Do not start another
owner-inbox cap/drain tuning ladder.

That final diagnostic found the missing internal bottleneck: all-class
returned-refill batch32 raises ColdSpanOwner from 54.725M to 166.157M balanced,
33.480M to 70.661M wide_ws, and 74.503M to 128.981M larger_sizes. It still
reaches only 16.0..51.8% of tcmalloc. Local medium/mixed regress by 3.8/4.7%,
so Batch32 is broad-specialist evidence, not a general promotion.

Final decision: close the Windows HZ12 speed-promotion track. Keep the
mechanism/lifetime/reclaim evidence and the opt-in specialist rows. Do not add
a class-mask or batch-size tuning ladder.

## Completed: Windows Bounded Reclaim Lifecycle L5-F

HZ12 opens from HZ11's deterministic Windows cross-owner pipeline evidence.
The first box is diagnostic-only. It must not change HZ11 or introduce an HZ12
production allocator behavior.

```text
evidence:
  xowner pipeline, 4 producers / 4 consumers, sizes 8..1024
  cross-owner free rate = 100%
  sharing factor = 2.0
  HZ11 R3 median = 7.45M ops/s
  tcmalloc R3 median = 27.35M ops/s

HZ12 charter:
  primary: reclaimable spans and low-RSS behavior
  secondary: cross-owner throughput recovery

L0 behavior:
  no inbox publish
  no owner handoff
  no reclaim/decommit
  no hot-path owner read
  flush-time owner routing projection only

L0 required output:
  would_route_to_owner
  would_keep_ownerless
  owner_inbox_pressure projection
  dead_owner / orphan projection
  span_reclaimable projection
  cross-owner pipeline attribution
```

## Next Decision

Shadow evidence decides whether HZ12 L1 adds a bounded mutex-protected owner
inbox with batch splice. Promotion requires a clear, bounded routing signal and
no safety ambiguity. If shadow shows low routing value or persistent imbalance,
close the ownership track rather than adding a per-free mechanism.

L0 result: Windows xowner R3 routed 100% of shadow flush objects as foreign,
with zero unknown owners, zero cap256 fallback, projected inbox peak 256, and
61 foreign-flush spans. L1 is now implemented as a benchmark-wrapper behavior
prototype: consumer-local deferred batches, bounded mutex inboxes, producer
drain, and ownerless fallback. It still does not modify HZ11 behavior.

L1 result: the first Windows R3 reached 21.458M ops/s with 100% cross-owner
frees, `fallback_unknown=0`, and only 0.001-0.003% bounded ownerless overflow.
This is GO as owner-inbox mechanism evidence. It is not a default/profile
promotion. Next: cap/drain policy confirmation, then dead-owner adoption and
verified whole-span state before any reclaim/decommit behavior.

Same-session xowner compare R3: HZ11 ownerless 9.138M, HZ12 owner inbox L1
24.516M, tcmalloc 28.553M. HZ12 L1 is 2.68x HZ11 and about 86% of tcmalloc.
The L1 teardown smoke passed 10/10 with `fallback_unknown=0`. Keep the current
L1 as a mechanism lane.

Cap policy R3, with 5-second xowner runs and a drain interval of 256, keeps
1024 as the balanced L1 control: 24.612M ops/s, 17.04 MiB peak RSS, 16.89 MiB
final RSS, 4,007 ownerless-overflow objects, and an observed inbox maximum of
1,021. Cap 2048 was only 0.3% faster (24.680M) but raised peak RSS to
18.01 MiB; cap 512 was slower and overflowed 281,076 objects. This is a cap
policy result, not a public/default promotion.

## Next Decision

L2-A now adds only retired-owner lifecycle observation: it snapshots a bounded
retired inbox and verifies that normal L1 drain reduces that projection to zero.
It must not detach objects, change routing, reclaim a span, or decommit memory.
L2-B may add an explicit adopter only after this shadow contract is verified.

L2-A initial evidence: the dedicated smoke observed a retired inbox with eight
pending objects and then zero after the existing owner drain. A 2 producer / 2
consumer xowner run marked two owners retired and ended with zero pending
retired inbox objects. This passes the observation contract only; adoption and
whole-span reclaim remain separate implementation gates.

L2-B adds an explicit adopter only in a dedicated smoke: it may detach one
bounded inbox from a marked-retired owner and release that chain through the
existing ownerless HZ12 path. The normal L1 xowner runner retains its current
drain behavior. Whole-span accounting and reclaim remain disabled.

L2-B initial evidence: the retired-inbox smoke rejected adoption for an active
owner, adopted eight pending objects exactly once after retirement, and then
observed zero pending objects. The regular 2 producer / 2 consumer xowner run
does not call adoption and ended with zero pending retired inbox objects. Next:
L2-C verified whole-span accounting, still without reclaim/decommit behavior.

L2-C records wrapper-only per-span alloc/free/live counts. A zero live count is
not enough for reclaim: the span must also have issued its full class capacity
with matching tracked releases and no accounting underflow. Even that result is
only an accounting candidate until current-span, cache, route, and source
lifetime detachment are separately proven.

L2-C initial evidence: the controlled 64-byte full-span smoke reached one
candidate with 1,024 tracked allocs, 1,024 releases, zero live objects, and no
underflow. A normal 2 producer / 2 consumer mixed xowner run ended with zero
tracked live objects but 39 partial spans and zero candidates. Therefore L2-D
must prove detachment; neither an empty inbox nor zero live count permits
reclaim/decommit.

L2-D now composes that accounting candidate with read-only detachment witnesses:
current-span references, current-thread front-cache objects, global returned
sink objects, direct-index route attachment, and explicit thread quiescence.
The first expected result is a closed gate that explains exactly where the
full-span smoke remains retained.

L2-D initial evidence: the controlled full span remained blocked with one
current-span reference, 256 objects in the front cache, 768 objects in the
returned sink, and its direct-index route still attached. Together the two
caches account for all 1,024 slots. `gate_open=0`, and the query changed no
allocator state. Next: L2-E ordered-detachment shadow/behavior, route last.

L2-E is restricted to a dedicated single-thread/quiescent smoke. It removes
front-cache and returned-sink objects, clears the current-span reference, and
invalidates the direct-index route last. A classify miss for an arena pointer
must be fail-closed before this behavior is allowed. Decommit remains disabled.

L2-E initial evidence: the controlled full span detached 256 front-cache and
768 returned-sink objects, cleared one current-span reference, then detached
the route last. The reclaim gate changed from closed to open. A stale arena
usable-size returned zero and stale free stayed fail-closed. All earlier HZ12
smokes remained green. Next: L2-F detached-span decommit observation/behavior;
reuse and multi-thread reclaim remain separate gates.

L2-F is a Windows-only one-span mechanism smoke. It may call `MEM_DECOMMIT`
only after the L2-E gate is open, then verifies the payload changes from
`MEM_COMMIT` to `MEM_RESERVE`. The arena reservation remains intact and the
span is not reusable. No RSS or production-policy claim is allowed yet.

L2-F initial evidence: one 64 KiB detached span changed from `MEM_COMMIT` to
`MEM_RESERVE`; the arena reservation remained intact and stale arena queries
stayed fail-closed. Earlier smokes passed, and a short xowner repeat-10 ended
with matching alloc/free counts, zero tracked live objects, and zero underflow
in every run. Next: L2-G bounded depot and recommit-before-route, opt-in only.

Before L2-G, source layout was tightened: thread-cache diagnostics moved out of
the allocator body, reducing `hz12_thread_cache.c` from 826 to 545 lines and
placing diagnostics in a 285-line sibling. All HZ12 C/header/include files must
remain below 1,000 lines; the layout check script enforces this boundary.

L2-G adds a bounded depot in a new module. The dedicated quiescent smoke must
recommit the exact span address before resetting accounting, restoring the
route, and installing the span as a current bump source. The normal hot path
does not consult the depot.

L2-G initial evidence: one decommitted span entered a fixed 64-entry depot;
duplicate insertion was rejected. Take recommitted the same address, reset the
accounting generation, restored the route, installed the current span, and the
next malloc returned that span base. The reused generation completed at
`alloc=1/free=1/live=0`. The depot remains disconnected from normal lanes.

L2-H adds depot slow-path-only counters and a same-process repeated lifecycle
smoke. It must keep depot count bounded, reset accounting every generation,
and retain zero recommit/route/current-install rollback failures. Cap saturation
is a separate follow-up smoke.

L2-H result: the same span completed eight in-process generations with eight
successful put/take/recommit operations, depot max one, and zero failures or
rollbacks. The cap smoke accepted 64 of 65 decommitted spans and reported
exactly one `put_full`, with current/max fixed at 64. The counter set is now
sufficient for cycle and capacity diagnosis; RSS policy remains unimplemented.

L3-A adds a separate generation-tagged owner registry. The first diagnostic
smoke covers concurrent register, `ACTIVE -> RETIRING -> DEAD`, slot reuse,
and stale-token publish rejection. It remains disconnected from malloc/free and
the normal owner inbox until the lifecycle contract passes.

L3-A result: the 4-thread lifecycle smoke passed repeat-20. Each run registered
four owners, completed ACTIVE/RETIRING/DEAD transitions, reused all four slots
with new generations, accepted 12 valid publish guards, rejected four DEAD and
four stale tokens, and recorded zero invalid transitions. Next: L3-B token-aware
inbox publish in a dedicated lane; normal malloc/free remains unchanged.

L3-B binds an owner inbox to `slot + generation`. ACTIVE and RETIRING batches
may publish; DEAD and stale batches fall back ownerlessly. A dedicated two
producer lifecycle smoke drains before DEAD and rebinds only an empty inbox to
the replacement generation.

L3-B result: the token-inbox lifecycle smoke passed repeat-20 with two
concurrent producers. It accepted 24 ACTIVE/RETIRING batches, drained 512
objects before DEAD and 256 replacement-generation objects afterward, and
ownerless-freed all 128 DEAD/stale objects. The inbox remained generation-pure
(`generation_reject=0`, `overflow=0`, `inbox_max=512`). Token publication and
its atomic diagnostic counters remain outside normal `hz12_malloc`/`hz12_free`.
L3-C adds an enrollment-bounded owner epoch: the retirement snapshot records
every producer slot and generation, and an owner may become DEAD only after
each recorded participant acknowledges the epoch. It is still diagnostic-only
and is not a normal allocator barrier.

L3-C result: the two-producer owner-epoch smoke passed repeat-20. Before the
two enrolled producers checkpointed, readiness was false; after both
acknowledged the captured epoch, readiness was true and the owner completed
`RETIRING -> DEAD`. Both participants unregistered cleanly. The start snapshot
retains participant generations, so an unregister cannot silently reduce the
required acknowledgement set. Next: L3-D can compose epoch readiness with the
token-inbox drain gate in one controlled owner retirement smoke.

L3-D composes those two teardown preconditions. It requires both every
start-snapshot producer acknowledgement and an empty generation-bound token
inbox before `RETIRING -> DEAD` is allowed.

L3-D result: the two-producer retirement-gate smoke passed repeat-20. It
blocked once before producer checkpoints, blocked again after both checkpoints
while 256 final-flush objects remained queued, then opened only after the exact
256-object inbox drain. The owner transitioned to DEAD afterward. This remains
a controlled diagnostic teardown gate; normal allocation/free stays untouched.

L4-A runs the L3-B/C/D lifecycle in a paced live diagnostic: two producers
publish token-bound batches while the owner drains, then each producer emits one
final RETIRING batch and checkpoints. The lane is deliberately paced and is not
a throughput benchmark.

L4-A result: repeat-5 passed. The representative one-second run published and
drained 4,288 objects with zero inbox overflow, registry rejection, or
generation rejection. The gate remained closed while the final 64 objects were
queued and opened only after their drain. Next decision: keep this controlled
retirement contract as evidence and design a separate, bounded owner-routing
behavior candidate only if it can preserve the no-per-free-cost rule.

L4-B is a separate 1 producer / 1 consumer, 100% cross-owner token pipeline.
The consumer routes only at 256-object flush boundaries through the token inbox;
the producer drains every allocation in this overflow-free lifecycle control.

L4-B result: repeat-5 completed with zero overflow, registry rejection, and
generation rejection. Median throughput was about 10.10M ops/s, versus about
11.66M for the existing owner-id L1 control at the same 1P/1C one-second setup.
This is expected control cost from the deliberately eager drain policy, not a
production comparison. Keep L4-B as safe token-routing evidence; do not promote
it. Any future drain-interval policy probe must remain separate from the L4-B
lifecycle contract and preserve ownerless fallback.

L4-C now sweeps token-inbox drain intervals of 1..256 allocations in the same
1P/1C diagnostic pipeline. It records throughput, inbox peak, bounded
ownerless fallback, rejection counters, and retirement completion. L4-B's
interval-one control remains the safety baseline.

L4-C result: no interval is a promotion candidate. The initial R3 sweep showed
more fallback as intervals widened; interval 8 looked clean initially but had
2/10 fallback runs, and interval 16 had 3/10 fallback runs in focused R10.
Both retained zero registry/generation rejection and completed the retirement
gate, so ownerless fallback is safe. However, Windows scheduling can fill the
1,024-object inbox even under eager drain. Freeze drain policy as diagnostic
evidence: do not default it, do not use it for a throughput claim, and do not
raise the cap without a separate RSS/Pareto study.

L5-A now opens as a read-only retired-owner reclaim shadow. A separate
generation-aware side table narrows span discovery; L3-D proves owner
quiescence; the existing L2 reclaim gate remains the sole span safety
authority. `detach_ready_bytes` and fully gated `reclaimable_bytes` are reported
separately, and incomplete foreign-cache scope forces both to zero.

L5-A result: the dedicated smoke passed repeat-20. With incomplete foreign
cache scope it reported zero bytes. With complete scope but before detach it
identified one accounting candidate blocked by current-span, front-cache,
returned-sink, and route witnesses. After the already-proven L2 ordered detach,
the same owner span reported exactly 65,536 reclaimable bytes. Reusing the owner
slot with a new generation produced one stale span and zero attributed bytes.
L5-A remains read-only; the test invokes L2 detach only to validate the positive
observation boundary.

L5-A wide_ws-like attribution result: repeat-5 found 72.19..79.25 MiB of
physical full-span accounting candidates (median 72.88 MiB) against median
peak RSS 82.00 MiB. This explains roughly 89% of the observed RSS scale. The
runner uses the matrix thread/iteration/working-set/size shape but omits its
periodic realloc, so it is structural evidence rather than a benchmark score.
`reclaimable_bytes` remains correctly zero because foreign-thread cache scope
has not yet been proven complete. Next: L5-B class-flush checkpoint shadow.

L5-B result: each of eight enrolled workers explicitly flushed its class cache,
cleared diagnostic current-span references, and acknowledged the owner epoch
before exit. Repeat-5 removed all foreign-scope/current/front blockers and found
80.19..81.25 MiB of physical full-span candidates (median 80.75 MiB) against
89.70 MiB median peak RSS. All candidate spans remain blocked by the returned
sink and route; reclaimable bytes remain zero. Next: L5-C bounded quiescent
returned-sink detach, route last, with no decommit yet.

L5-C result: the wide_ws-like diagnostic completed a fixed 64-span budget in
repeat-3. Every run attempted and detached exactly 64 spans, failed zero times,
removed exactly 4.00 MiB from returned-sink ownership, detached each route last,
and observed exactly 4.00 MiB as fully gated reclaimable bytes afterward.
Candidate pools remained much larger (67.94..73.44 MiB in this repeat), proving
the budget rather than candidate scarcity limited behavior. No span was
decommitted or inserted into the depot. Next: L5-D bounded decommit of only the
L5-C success set, still without automatic production policy.

L5-D result: the 64-span wide_ws-like lane passed repeat-3. Every run
decommitted 64/64 gate-open spans, failed zero times, and released exactly
4.00 MiB. Working-set RSS fell by 3.99 MiB in every run (72.48->68.49,
83.46->79.47, and 82.30->78.31 MiB). This directly validates the HZ12 low-RSS
charter at a fixed bounded budget. No span entered the depot and no automatic
reclaim policy ran. Next: L5-E bounded depot insertion/recommit cycle for only
the L5-D success set.

L5-E now connects only the bounded L5-D set to the existing 64-span depot. It
must prove exact-address return, generation ownership, recommit-before-route,
accounting reset, current-span installation, and an empty depot after the
cycle. The explicit checkpoint between same-class takes is diagnostic-only.
No automatic reclaim, depot admission, or normal malloc/free policy is added.

L5-E result: the fixed wide_ws-like lane passed repeat-3. Each run detached,
decommitted, inserted, took, and recommitted exactly 64/64 spans (4.00 MiB).
All returned addresses belonged to the inserted set, owner generation mismatch
was zero, all 64 explicit current checkpoints completed, and the depot ended
empty. The pre-recommit L5-D RSS observation still fell by 3.99-4.00 MiB.
This is GO as bounded lifecycle evidence only.

## Next Decision

L5-F may exercise a bounded subset of the recommitted spans through real slot
allocation and payload touch, then require a second ordered detach/decommit
cycle. It must remain diagnostic-only and must not introduce automatic reclaim
or depot admission. If a recommitted span cannot complete that second cycle
without route/accounting mismatch, stop before policy work.

L5-F result: repeat-3 completed the full 64-span second cycle. Every run put,
took, recommitted, exercised, detached again, decommitted again, and returned
64/64 spans to the bounded depot. Each selected span touched every class slot
(9,984..10,048 slots per run), redecommitted exactly 4.00 MiB, and ended with
address mismatch 0, owner-generation mismatch 0, failures 0, and depot count
64. The Windows bounded reclaim lifecycle is complete as mechanism evidence.

## Frozen Boundary

The L5 chain is now frozen. It proves that retired-owner spans can be identified,
quiesced, detached, decommitted, recommitted at the same address, used through
normal free, and reclaimed again. It does not select candidates automatically.
Any production pacing/scanning/depot-admission policy must begin as a separate
diagnostic lane with RSS and throughput acceptance gates.

## Measurement Snapshot

The original grouped R5 is retained only as invalid measurement-order evidence.
The provenance-safe runner now rotates row order, fixes affinity and priority,
and records executable hashes and build flags. Its A/A tests place median row
noise near 4%, although individual Windows runs can still move by 14..24%.

The corrected 100% cross-owner R5 first measured the diagnostic L1 at 17.604M
ops/s. Compiling out atomic whole-span accounting raised the median from
18.420M to 25.351M (+37.6%). Compiling out only the remaining shadow/inbox
diagnostic counters then raised 25.620M to 28.477M (+11.2%). Both changes kept
the owner mapping, bounded mutex inbox, drain behavior, ownerless fallback,
and retirement path intact.

The final round-robin speed comparison measured HZ11 ownerless at 13.046M,
HZ12 owner-inbox speed at 28.896M, and tcmalloc at 36.833M ops/s. HZ12 is 2.21x
HZ11 and reaches 78.4% of tcmalloc in this exact pipeline. Owner-inbox behavior
is therefore GO as a speed mechanism; it is not yet a public/default allocator
lane or a complete HZ11 performance superset.

The bare-core ceiling control then measured 13.434M versus 29.274M for the
owner-inbox speed row. The inbox path is 2.18x faster because it replaces the
ownerless per-object returned-list critical-section traffic with bounded batch
publication. Do not remove owner routing or the inbox. The next narrow A/B is
to replace the steady-state owner-map CAS with a relaxed-load same-owner hit,
while preserving first-writer CAS and all routing/fallback behavior.

OwnerFastLoad-L1 is GO as a Windows xowner candidate. The narrow R5 measured
36.052M versus 29.292M for the counter-free speed control (+23.1%). A direct
same-session R5 then measured 35.542M versus tcmalloc at 37.597M, placing HZ12
at 94.5% of tcmalloc in this row. Repeat-10 short safety runs all completed
with zero pending retired owners/objects and at most one producer-stop cleanup
object. The first-writer CAS remains; only an already-matching owner token uses
the relaxed-load return.

The fair RSS R5 now includes all three rows. HZ11 measured 12.992M / 7.79 MiB
peak, HZ12 OwnerFastLoad measured 36.427M / 15.54 MiB, and tcmalloc measured
36.439M / 14.81 MiB. HZ12 is throughput-parity in this exact pipeline but uses
4.9% more peak RSS and 8.2% more final RSS than tcmalloc. Do not claim a low-RSS
win from this compact row.

Broad controls are now connected to the existing Windows random_mixed and
mixed_ws runners. HZ12 core is strong on local random_mixed: 154.122M small,
150.096M medium, and 152.453M mixed, with 4.27..5.57 MiB peak RSS. The short MT
matrix remains weak (13.902M balanced, 14.632M wide_ws, 27.734M larger_sizes),
which is consistent with ownerless returned-list contention.

Owner attribution decomposition fixes the next design boundary. Allocation-
only owner mapping costs 1.6..2.4% on local random_mixed and passes the 3% gate.
Adding a free-time owner lookup costs 9.7..10.6% and is NO-GO. The next behavior
must leave normal free owner-blind and group by owner only when a class cache
flushes.

Review synthesis: the measured L1 row mixes allocator behavior with shadow,
whole-span accounting, and inbox diagnostics, so an HZ12 speed lane has not yet
been measured. Runner output must include a build-flag manifest in addition to
hashes, and the corrected runner must establish its own A/A band. Production
reclaim authority must not reuse per-op atomic shadow accounting: use an
owner-local batch ledger updated at carve, local flush, owner drain, and retire
checkpoint. Keep the atomic shadow only as a diagnostic judge and require exact
shadow/ledger agreement before any automatic reclaim policy can be promoted.

## Next Decision

1. Open `FlushTimeOwnerRouting-L1` as a benchmark/profile sibling: advisory
   alloc map, owner-blind normal free, owner grouping only on cache flush.
2. Keep the bounded inbox: it is 2.18x the ownerless bare-core control.
3. Keep atomic accounting and all diagnostic counters out of the speed lane.
4. Start production reclaim authority only after flush-time routing passes
   local, MT, RSS, and teardown gates, using an owner-local batch-ledger
   sibling, checked against the frozen atomic shadow; do not reconnect per-op
   atomics to normal malloc/free.

## Active: ReclaimPolicy-L6A Owner-Local Batch Ledger Shadow

L6-A is shadow-only. It adds a physical-slot ledger whose only legal mutation
points are source acquire/carve, owner drain observation, local batch return,
and a quiescent retirement checkpoint. Foreign producers never update an
owner's ledger. Owner drain does not clear outstanding slots because the
objects remain in the owner cache until class flush.

The initial dedicated smoke must show exact candidate agreement with the
existing atomic accounting shadow after a full class-flush checkpoint. Normal
speed lanes do not link the ledger module. Automatic detach, decommit, depot
insertion, and reclaim policy remain closed.

Initial L6-A result: the dedicated 64-byte-class smoke passed repeat-20. One
full 64 KiB span produced 1,024 acquired slots; observing a 512-object owner
drain did not reduce the outstanding count; the quiescent class flush returned
all 1,024 slots; and the ledger/atomic candidate comparison reported one match
and zero mismatches. Existing whole-span, retired-reclaim, and owner-retire-gate
smokes also pass. The next gate is real-boundary wiring in a diagnostic sibling:
source acquire, flush-owner batch split, actual flush-owner inbox drain, and
retirement snapshot. The current retire gate still observes the token inbox,
not the allocator's flush-owner inbox, so automatic reclaim remains closed.

L6-B real-boundary result: the diagnostic-only build now records fresh/current
span bump acquisition and same-owner flush return directly in allocator cold
paths. Its 1,024-slot smoke observes the real owner token `0:1`, reaches zero
outstanding slots after the real reclaim checkpoint, and matches the atomic
candidate with zero mismatches. Foreign publish performs no ledger mutation;
the owner-drain hook observes transit without clearing outstanding state.
L6-B2 must exercise that cross-thread publish/drain path before the actual
flush-owner inbox can join the retirement gate.

L6-B2 result: the 100% cross-thread producer/consumer smoke passed repeat-20.
The consumer published 1,024 foreign slots without mutating the owner ledger;
the producer observed all 1,024 at the real flush-owner drain and reached zero
outstanding only after its local class-flush checkpoint. Atomic and ledger
candidates matched exactly with zero rejected transitions. Next: L6-C adds a
read-only pending query for the real flush-owner inbox and composes it with the
existing epoch/token-inbox retirement gate. No reclaim behavior before that
combined gate is proven.

L6-C result: the combined read-only retirement gate passed repeat-20. Before
owner drain it observed 1,024 real flush-owner inbox objects and stayed closed.
After producer epoch acknowledgement, real inbox drain, and owner-local cache
flush, pending fell to zero; the owner-scoped ledger/atomic comparison reported
one match and zero mismatches; and the combined gate opened. This removes the
previous token-inbox-only observation gap without adding malloc/free work.
Next: use this authority in a bounded wide_ws shadow scan only. Detach,
decommit, depot insertion, and automatic reclaim remain closed.

L6-D result: the bounded wide working-set owner-ledger shadow passed repeat-10.
One owner exposed 64 full spans (65,536 64-byte slots); the consumer returned
one span at a time and the owner drained after every span, keeping the real
inbox bounded. All 64 ledger candidates matched all 64 atomic candidates with
zero mismatches, pending ended at zero, the combined retirement gate opened,
and the measured candidate set was exactly 4 MiB. No detach or decommit ran.
Next: L6-E may hand only this fixed success set to the existing bounded ordered
detach/decommit path, still as a diagnostic sibling with fail-closed set
comparison before mutation.

L6-E result: the fixed L6-D success set passed ordered detach/decommit repeat-5.
All 64 candidate spans detached and decommitted with zero failures, releasing
4 MiB of payload. Working-set RSS fell by 4,186,112 bytes (~3.99 MiB) in every
run. The separate L6-D shadow executable still performs zero detach/decommit.
This remains HZ12 diagnostic behavior, not HZ8 integration or automatic
production policy. Next: L6-F bounded depot/recommit, real slot exercise, and a
second owner-ledger reclaim cycle for the same fixed set.

L6-F result: the bounded owner-ledger lifecycle passed repeat-5. All 64
decommitted spans entered the depot, recommitted at valid addresses, exercised
all 65,536 physical slots, reached ledger/atomic agreement again, completed 64
second detaches and 64 second decommits, and ended with 64 bounded depot
entries. Address, owner, and accounting mismatches remained zero; the ~3.99 MiB
RSS reduction remained visible. Ledger query is now route-independent after
detach/decommit, matching the side-table contract. The fixed lifecycle is
complete. Next work is a separately gated bounded production policy, not more
lifecycle mechanics and not direct HZ8 integration.

## ReclaimPolicy P0 Performance Gate: NO-GO

P0 separated the ledger core from the atomic comparison module, linked only
the core into a production-shape ColdSpanOwner sibling, and kept automatic
reclaim off. Build flags and binary hashes are saved in the P0 manifest.

The two-second wide_ws R5 did not show a throughput regression (35.753M versus
34.442M baseline), although peak RSS rose from 88.29 to 91.20 MiB before any
reclaim policy. Local random_mixed is decisive: small fell 16.3%, medium 15.6%,
and mixed 17.8%. P0 therefore fails the -5% hard gate and is frozen as a
control. L6-A..L6-F remain valid lifecycle evidence.

Do not implement automatic reclaim yet. Next: diagnostic attribution of the
second flush scan, per-slot bitmap transitions, acquire-side lookup, and
return-side update. The production candidate should reuse owner information
already computed by flush routing and use compact per-span batch counts; the
bitmap remains a diagnostic judge.

P0-A attribution result: small random_mixed R5 measured baseline 99.489M,
inert 98.957M (-0.5%), acquire-only 97.370M (-2.1%), return-only 84.132M
(-15.4%), and full 82.804M (-16.8%). Return-side duplicate classification and
per-slot transition are the blocker. Next: P0-B trusted-owned batch return may
reuse the existing all-owner proof and skip repeated owner filtering. If that
still misses -3%, replace the production bitmap with compact per-span batch
counts; keep the bitmap only as diagnostic authority.

P0-B trusted-owned return result: removing the duplicate owner filter and
span-owner lookup improved the full ledger from 82.730M to 90.065M, but the
98.055M baseline still leads by 8.1%. P0-B is NO-GO for production promotion.
The remaining per-slot location/bitmap transition is too expensive. Freeze the
bitmap implementation as the safety judge. Next: compact owner-local per-span
batch counts with O(1) contiguous carve and route-reused return attribution.

P0-C compact-count result: replacing the bitmap with per-span owner/class,
carved-highwater, and outstanding counts improved small random_mixed to
87.415M, versus 70.540M for the bitmap control, but the 99.781M baseline still
leads by 12.4%. Exact returned-sink reacquire/return maintenance remains too
frequent. P0-C is NO-GO. Stop continuous-ledger tuning. Next: reconstruct the
candidate set only at a retirement/pressure checkpoint after real inbox drain
and enrolled cache flush; validate that snapshot against bitmap/atomic shadows
in diagnostic lanes without adding normal malloc/free/refill accounting.

P0-D checkpoint snapshot result: the returned sink now supports a cold
lock-held per-span snapshot using a temporary bitmap to reject unaligned and
duplicate slots. In the 64-span L6-D diagnostic, repeat-10 reconstructed all 64
atomic/bitmap candidates with zero snapshot mismatches and 4 MiB of candidate
payload. The snapshot adds no malloc/free/refill/cache-flush accounting. P0-D
is the first authority placement compatible with the speed gate by
construction. Next: a separate retire-only behavior sibling may consume at
most 64 snapshot-proven spans / 4 MiB; continuous bitmap/count controls remain
NO-GO and must not enter the speed lane.
