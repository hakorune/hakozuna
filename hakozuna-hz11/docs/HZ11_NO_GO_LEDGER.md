# HZ11 NO-GO Ledger

Status: initialized with inherited decisions from HZ8/HZ10 speed work.

Do not retry these ideas in the HZ11 default hot path without new evidence.

| Box / Idea | Decision | Reason |
|---|---|---|
| Preserve HZ10 owner-return remote free in HZ11 speed mode | NO-GO for default hot path | HZ11 exists to test current-thread/current-CPU cache reuse. Owner-return is a HZ8/HZ10 correctness/RSS design, not the tcmalloc-speed shape. |
| Route every local free through pagemap metadata | NO-GO for default hot path | HZ10 attribution showed route and ownership validation are fixed costs. HZ11 must use token/cache hits and route only on fallback. |
| Put fail-closed diagnostics on every speed-mode op | NO-GO for default hot path | Checked diagnostics are valuable, but speed mode must not pay marker/state/generation/reason-counter costs by default. |
| Optimize HZ10 into HZ11 in-place | NO-GO | HZ10 is the route/ownership/reclaim research candidate. HZ11 needs a different semantic split and should live in a separate folder. |
| HZ11ThreadCacheFastPath-L0: system-backed token-table front-end as the speed path | NO-GO for the tcmalloc-tier gate (measured) | Implemented + measured. Front-end works (cache hit 100%, token hit 100%, beats glibc/mimalloc/hz10), but RUNS=3 median fixed16/64/256/4096 land at 0.71-0.82 of tcmalloc (gate 0.95). The gap is the free-side token hash: tcmalloc classifies a free by a direct-indexed page table (~1-2 cycles), hz11 must hash because system-malloc objects have arbitrary addresses (no contiguous region to index). The hash is structural to the system-backed L0. Next box: span/pageheap backing -> direct-indexed classification -> close the gap. |
| HZ11SpanBackedClassify-L1: arena/span direct-index classify | NO-GO for the tcmalloc-tier gate (measured) | Implemented + measured against the L0 token lane. The span lane is correct after fixing arena range checks to use integer address arithmetic and adding an overflow returned-list sink, but RUNS=5 fixed64 reaches only 0.739 of tcmalloc (gate 0.95) and improves over the token lane by only ~2-3%. The L0 token-hash diagnosis was too strong: classification is not the main lever. Next box should attribute the whole HZ11 front-end instruction/call/TLS/size-class cost against tcmalloc on the same fixed rows. See docs/HZ11_SPAN_BACKED_CLASSIFY_L1.md. |
