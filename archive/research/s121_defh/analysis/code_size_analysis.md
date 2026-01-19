# S121-D/E/F/H Code Size Analysis

## Summary

Total code removed: **677 lines (20.2% of S121 codebase)**

All four variants (S121-D/E/F/H) resulted in NO-GO decisions due to performance regressions ranging from -25% to -82%.

## Breakdown by File

### hz3_owner_stash_globals.inc
- **S121-D (PAGE_PACKET)**: Lines 284-406 (123 lines)
  - Page packet slot management
  - Batch accumulation logic
  - Flush helpers
  - Stats infrastructure

- **S121-E (CADENCE_COLLECT)**: Lines 412-630 (219 lines)
  - Global chunked page list
  - Budget-limited scanner
  - Chunk allocation
  - Collect logic

- **S121-F (PAGEQ_SHARD)**: Lines 98-123, 133-277 (82 lines)
  - Sub-shard structure definitions
  - Hash-based shard selection
  - Per-shard push/pop/drain helpers

**Subtotal: 424 lines**

### hz3_owner_stash_push.inc
- **S121-D**: Lines 74-76, 128-130, 164-166, 224-226, 470-472, 520-521, 615-617 (66 lines)
  - Packet add calls (instead of direct pageq push)
  - Stats registration

**Subtotal: 66 lines**

### hz3_owner_stash_pop.inc
- **S121-E**: Lines 19-63 (45 lines)
  - Cadence collect pop path
  - Spill drain + global page scan

- **S121-F**: Lines 125-143, 210-221, 225-231, 260-277, 329-334 (72 lines)
  - Sub-shard drain loop
  - Merge-once approach
  - Requeue-to-subshard logic

- **S121-H**: Lines 103-116 (14 lines)
  - Budget-limited drain loop
  - Per-page pop with early exit

**Subtotal: 131 lines**

### hz3_owner_stash_stats.inc
- **S121-F**: Lines 312-341 (30 lines)
  - Pageq push/pop contention stats
  - Remote push contention tracking

**Subtotal: 30 lines**

### hz3_types.h
- **S121-D**: Lines 170-183 (14 lines)
  - Hz3PagePacketSlot type definition

- **S121-E**: Lines 185-208 (24 lines)
  - Hz3PageChunk type definition
  - Hz3OwnerPageListGlobal type definition

**Subtotal: 38 lines**

## Total Lines Removed: 677

## Performance Impact (Reason for NO-GO)

| Variant | Performance vs Baseline | Primary Issue |
|---------|------------------------|---------------|
| S121-D  | -82%                   | Batch management overhead (slot search + packet chain walk) |
| S121-E  | -40%                   | O(n) page scan overhead |
| S121-F  | -25% ~ -35%            | Pop-side fixed cost increase (multi-shard drain) |
| S121-H  | -30%                   | Per-page CAS/pop overhead (no drain-all benefit) |

## Archive Rationale

These implementations were well-designed research explorations but failed to meet performance targets:

1. **S121-D**: Batching overhead exceeded single-push latency benefits
2. **S121-E**: Global scan approach created O(n) bottleneck
3. **S121-F**: Sharding reduced CAS contention but increased pop latency
4. **S121-H**: Budget-limiting prevented requeue explosion but lost drain-all efficiency

Keeping them in-tree would:
- Increase maintenance burden (20.2% of code)
- Reduce code readability
- Risk accidental re-enablement

## Retained Variants (GO)

- **S121-C**: Page-local remote with pageq notification (baseline, GO)
- **S121-G**: Atomic packed state (tagged pointer, GO)

## References

- Design docs: `hakozuna/hz3/docs/PHASE_HZ3_S121_*.md`
- Results: `hakozuna/hz3/docs/PHASE_HZ3_S121_SERIES_RESULTS_NO_GO.md`
- Config archive: `hakozuna/hz3/include/hz3_config_archive.h`
