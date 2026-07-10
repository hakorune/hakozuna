# HZ12 Windows ReturnedRefillBatch32 Probe (2026-07-10)

Status: GO as bottleneck evidence; HOLD as a broad specialist; NO-GO for the
general/default ColdSpanOwner profile.

## Stable MT R3

The probe adds the existing returned-sink batch refill behavior to
ColdSpanOwner for every size class, with a batch size of 32.

| Profile | ColdSpanOwner | Batch32 | Gain | tcmalloc | Batch peak RSS | tc peak RSS |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| balanced | 54.725M | 166.157M | 3.04x | 507.371M | **46.36 MiB** | 47.76 MiB |
| wide_ws | 33.480M | 70.661M | 2.11x | 441.301M | 90.69 MiB | **75.90 MiB** |
| larger_sizes | 74.503M | 128.981M | 1.73x | 249.126M | 96.50 MiB | **80.93 MiB** |

The result confirms that one-object refill from the global returned sink is a
major HZ12 Windows bottleneck. Even after removing much of that lock traffic,
Batch32 reaches only 32.7%, 16.0%, and 51.8% of tcmalloc throughput.

## Local Gate R5

| Profile | HZ12 core | Batch32 | Delta |
| --- | ---: | ---: | ---: |
| small | 155.191M | 152.882M | -1.5% |
| medium | 153.859M | 147.997M | -3.8% |
| mixed | 153.986M | 146.815M | -4.7% |

Medium and mixed exceed the 3% local regression gate.

## Decision

- Preserve Batch32 as broad bottleneck evidence and an opt-in specialist.
- Do not promote it to the general/default ColdSpanOwner profile.
- Do not start a batch-size or class-mask ladder in HZ12; the remaining gap to
  tcmalloc is too large for this policy knob to close.
- The Windows HZ12 speed-promotion track is closed. The completed ownership,
  generation, retirement, and reclaim mechanisms remain valid research output.

Evidence: `bench_results/windows_stable_mt_refillbatch_probe_r3.md`.
