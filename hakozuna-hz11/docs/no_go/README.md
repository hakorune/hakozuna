# HZ11 NO-GO Docs

This directory holds detailed HZ11 documents for boxes that should not be
promoted or retried without new evidence.

Keep `../HZ11_NO_GO_LEDGER.md` as the short decision index. Put the longer
design, measurement, and interpretation records here.

```text
HZ11_SPAN_BACKED_CLASSIFY_L1.md:
  span-backed direct-index classify A/B; measured as NO-GO for the tcmalloc
  fixed-local gate and showed classification alone was not the main lever

HZ11_CACHE_SHAPE_L1.md:
  pointer-top cache A/B; measured as NO-GO because instruction count increased

HZ11_CACHE_LAYOUT_L1.md:
  SOA class-cache layout sibling; small instruction win but missed the gate

HZ11_SIZE_TABLE_STATIC_INIT_L1.md:
  NO-GO for removing the size-class lazy-init guard; loader-time malloc makes
  the guard load-bearing

HZ11_STATIC_CONST_SIZE_TABLE_L1.md:
  NO-GO for replacing the runtime-filled size table with a const .rodata table;
  correct but slower on the speed-ceiling lane

HZ11_MACRO_SPEED_LANE_GATE_L1.md:
  macro gate for the promoted transfer lane; NO-GO for macro speed-lane
  promotion because abort/RSS rows failed

HZ11_MACRO_FAILURE_ATTRIBUTION_L1.md:
  diagnostic cap A/B for macro failures; cap-only explains abort rows but is
  not a memory policy

HZ11_CENTRAL_FREELIST_SPAN_RETURN_L1.md:
  span-aware central free list and reusable span stack; fixes abort rows but
  remains NO-GO for macro promotion

HZ11_SPAN_SOURCE_ATTRIBUTION_L1.md:
  source/current-span attribution for larson RSS and sh6bench span-return wall
  regression; narrows the next work to current-span pooling and metadata-lock
  batching
```
