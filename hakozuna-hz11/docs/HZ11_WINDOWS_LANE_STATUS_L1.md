# HZ11 Windows Lane Status L1

Status: orientation / cleanup record.

## Selected Row

```text
hz11-span-cache256:
  selected Windows HZ11 bring-up row
  general Windows random_mixed row
  not a DLL replacement
  not Linux fine128 parity
```

Reason:

```text
random_mixed small/medium/mixed stays strong
RSS remains small
matrix pressure rows are understood as profile-specific pressure signals
```

## Candidate / Specialist Rows

```text
hz11-span-cache512:
  pressure evidence only
  cap increase alone does not solve wide_ws

hz11-span-cache512-classbatch16:
  candidate-watch / matrix helper
  improves balanced, wide_ws, and larger_sizes
  small random_mixed medium/mixed cost remains

hz11-span-cache512-classbatch:
  broad matrix specialist
  too aggressive for random_mixed general use
```

## Diagnostic Rows

```text
hz11-span-diag
hz11-span-cache256-diag
hz11-span-cache512-diag
hz11-span-cache512-classdiag
hz11-span-cache512-classbatch-diag
```

These rows enable counters and/or class attribution. Do not use them for speed
ranking.

## No-Go / Evidence Rows

```text
hz11-span-transfer:
  Windows transfer/central span port is not a selected row.
  RSS is too high for promotion.

hz11-span-cache512-classbatch16-4-7:
  preserves random_mixed but loses larger_sizes.
  balanced/wide specialist only.

hz11-span-cache512-classbatch16-pressure:
hz11-span-cache512-classbatch16-pressure1:
  pressure-gated classbatch protects random_mixed but loses the matrix win.
  no-go as implemented.
```

Runner cleanup:

```text
kept in normal Windows runners:
  hz11-span-cache512
  hz11-span-cache512-classdiag
  hz11-span-cache512-classbatch
  hz11-span-cache512-classbatch16
  hz11-span-cache512-classbatch16-4-7

not kept in normal Windows runners:
  hz11-span-cache512-classbatch16-4-6
  hz11-span-cache512-classbatch16-pressure
  hz11-span-cache512-classbatch16-pressure1
```

The removed runner rows remain documented evidence. They should only be
restored if a new design question explicitly needs them.

## Current Recommendation

```text
General Windows HZ11:
  use hz11-span-cache256

Matrix pressure experiments:
  use hz11-span-cache512-classbatch16 as candidate-watch

Do not:
  promote classbatch32, pressure-gated classbatch, or transfer as selected rows
```

## Next Work

If Windows HZ11 work continues, the next useful step is not another simple
classbatch threshold. Prefer one of:

```text
1. app-like / real workload confirmation for hz11-span-cache256
2. profile-scoped classbatch16 lane if the matrix row matters
3. a new pre-refill workload signal, not post-miss pressure gating
```
