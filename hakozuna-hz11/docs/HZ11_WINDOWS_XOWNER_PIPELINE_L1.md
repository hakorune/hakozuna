# HZ11 Windows Cross-Owner Pipeline Diagnostic

This is a diagnostic-only workload for deciding whether HZ11's ownerless
recycling model is sufficient for cross-thread free workloads. It does not
change HZ11 production code or the selected Windows row.

`win/bench_xowner_pipeline.c` creates equal producer and consumer pairs. A
producer allocates an object and places it on an SPSC ring; a different
consumer removes and frees it. Therefore the lane guarantees:

```text
cross-owner free rate = 100%
sharing factor = 2.0
```

The ring and thread coordination are shared by all allocator variants. The
lane is intentionally separate from Larson because it makes the ownership
relationship deterministic instead of inferring it from a noisy workload.

## Initial Windows R3

Configuration: 4 producers, 4 consumers, ring 4096, sizes 8..1024, 5-second
runs, three repetitions on the same Windows host.

| Allocator | Median ops/s | Reading |
| --- | ---: | --- |
| HZ11 | 7.45M | ownerless baseline |
| tcmalloc | 27.35M | reference |

HZ11 is about 72.7% below tcmalloc in this deterministic cross-owner lane,
well beyond the 15% opening threshold. This is the first workload that directly
establishes a large ownership-sensitive gap rather than relying on Larson's
unstable worker/main signal.

## Decision

```text
HZ11:
  keep ownerless default unchanged
  do not add owner metadata to the hot path

HZ12:
  ownership research is justified
  start with flush-time owner routing / batch handoff
  keep free-time safety independent from ownership locality
```

The next generation should begin with a diagnostic/profile lane, not a default
HZ11 change. The first HZ12 design should use cold-path routing and bounded
mutex-protected batch splice; lock-free remote queues and per-free owner reads
remain out of scope until the first profile has evidence.
