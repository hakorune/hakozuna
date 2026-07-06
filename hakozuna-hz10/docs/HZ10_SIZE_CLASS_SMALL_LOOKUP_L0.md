# HZ10SizeClassSmallLookup-L0

Status: NO-GO. Prototype reverted.

## Goal

Reduce `hz10_malloc()` instruction count in the preload product lane without
changing any size-class semantics. The target workload is sh6bench: it uses
small requests (1..1000B), so the existing fine-class path pays the bit-scan
and quarter-step arithmetic on most allocations even though the answer is a
tiny fixed mapping.

## Box Boundary

Only `hz10_size_class_for()` changes:

- `size == 0` and `size > HZ10_PAGE_QUANTUM` keep the same invalid result.
- `size <= 1024` uses a 64-entry table indexed by `(size - 1) >> 4`.
- `size > 1024` falls through to the previous bit-twiddle implementation.

The table is compile-time selected for the coarse and fine class layouts. The
class tables themselves, page layout, route validation, marker policy,
adoption, and shim lanes are unchanged.

## Correctness Gate

The existing `smoke-size-class` test is the main gate: it exhaustively checks
every byte size from 1 through 65536 against the slot-size table oracle. This
is stronger than boundary sampling and directly covers the new table.

## Result

The prototype worked at the codegen and correctness level:

- `smoke-size-class` stayed green and exhaustively verified every byte size.
- `objdump --disassemble=hz10_malloc` showed `size <= 1024` using the lookup
  table, with `bsr` and quarter-step arithmetic only on larger sizes.

But the product-lane wall-time gate did not justify keeping it:

```text
hz10-only RUNS=5:
  sh6bench     0.430s / 318,592 KiB
  python_alloc 0.840s / 106,700 KiB
  larson       4.173s / 283,644 KiB current

full RUNS=5:
  sh6bench hz10 0.440s / 318,336 KiB
  python_alloc hz10 0.840s / 106,644 KiB
  larson hz10 4.175s / 283,260 KiB current
```

The preceding `HZ10MallocFastLeafSplit-L0` full guard had sh6bench at 0.420s.
The lookup was neutral in the hz10-only lane and worse than the latest full
reference on the target row, likely because the new table load/layout change
does not reduce the actual front-end cost enough to beat placement effects.

## Archived Gate

Run the usual product-lane macro guard:

- hz10-only RUNS=5 macro matrix first, looking mainly at sh6bench.
- Full all-allocator RUNS=5 macro guard if the hz10-only lane is not worse.
- Keep RSS flat; this is pure codegen/addressing work.

GO requires a repeatable sh6bench improvement or a neutral result with cleaner
objdump evidence. Any broad macro regression is NO-GO and the table path should
be reverted.

Verdict: do not keep the lookup table. Leave `hz10_size_class_for()` on the
existing arithmetic path and move the next speed work to a larger structural
box, such as active-page/class-state addressing or per-owner local-free page
indexing.
