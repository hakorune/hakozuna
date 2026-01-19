# S18-2 PTAG 16-bit (NO-GO)

Purpose
- Reduce PTAG dst/bin tagmap footprint (4MB -> 2MB) to improve mixed/medium without hurting small.

Approach
- 16-bit tagmap for dst/bin (PageTagMap).
- Tried two bit layouts:
  - 12+4: bin+1 in bits 0-11, dst+1 in bits 12-15.
  - 8+8: bin+1 in low byte, dst+1 in high byte (simpler decode).

Build/Run (historical)
- build: make -C hakozuna/hz3 clean all_ldpreload \
  HZ3_LDPRELOAD_DEFS='-DHZ3_ENABLE=1 -DHZ3_SHIM_FORWARD_ONLY=0 -DHZ3_SMALL_V2_ENABLE=1 \
  -DHZ3_SEG_SELF_DESC_ENABLE=1 -DHZ3_SMALL_V2_PTAG_ENABLE=1 -DHZ3_PTAG_V1_ENABLE=1 \
  -DHZ3_PTAG_DSTBIN_ENABLE=1 -DHZ3_PTAG_DSTBIN_16=1'
- run:  RUNS=10 ./hakozuna/hz3/scripts/run_bench_hz3_ssot.sh

Results (RUNS=10 median)
- Baseline 32-bit: /tmp/hz3_ssot_6cf087fc6_20260101_170646
  - small 97.88M, medium 95.94M, mixed 92.51M

- 16-bit 12+4: /tmp/hz3_ssot_6cf087fc6_20260101_172250
  - small 90.69M (-7.4%), medium 96.83M (+0.9%), mixed 94.92M (+2.6%)

- 16-bit 8+8: /tmp/hz3_ssot_6cf087fc6_20260101_173147
  - small 95.38M (-2.6%), medium 94.06M (-2.0%), mixed 94.29M (+1.9%)

Decision
- NO-GO: small/medium regressions outweigh mixed gain.

Recovery / Repro
- If revisiting, start from S17 (32-bit) and reintroduce 16-bit under a new gate.
- Use the SSOT script and compare against /tmp/hz3_ssot_* logs above.
