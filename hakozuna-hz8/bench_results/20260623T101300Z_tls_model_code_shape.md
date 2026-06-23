# TlsModelCodeShape-L1

Date: 2026-06-23

## Decision

Conditional GO from design review:

```text
supported:
  Linux x86_64 ELF
  executable link
  startup DT_NEEDED
  exec-time LD_PRELOAD

not supported in v0:
  arbitrary late dlopen
  dlclose / reload guarantee
```

## Change

`h8_tls_ctx` now uses variable-scoped TLS model selection:

```text
__attribute__((tls_model("initial-exec"), visibility("hidden")))
```

Only the `h8_tls_ctx` declaration/definition is affected.  The build does not
use global `-ftls-model=initial-exec`.

## Code Shape

Raw log:

```text
bench_results/20260623T101300Z_tls_model_code_shape.log
```

Results:

```text
src/h8_small_local.c assembly:
  h8_tls_ctx@gottpoff(%rip)
  no __tls_get_addr

h8_bench_release malloc/free leaf:
  %fs:(%rax) TLS load
  no __tls_get_addr

libhakozuna_hz8_preload.so malloc/free leaf:
  %fs:(%rax) TLS load
  no __tls_get_addr

readelf:
  R_X86_64_TPOFF64
  FLAGS: STATIC_TLS
  h8_tls_ctx is LOCAL TLS symbol
```

## Verification

```text
make bench-release preload smoke safety-stress preload-smoke
./h8_smoke
./h8_safety_stress
LD_PRELOAD=$PWD/libhakozuna_hz8_preload.so /bin/true
```

All passed.

Focused release checks:

```text
guard/local0 RUNS=5:
  median ~= 333.3M ops/s
  p25 ~= 308.6M ops/s
  steady median ~= 391.5M ops/s

small_interleaved_remote90 RUNS=5:
  median ~= 54.9M ops/s
  p25 ~= 54.8M ops/s
  steady median ~= 57.2M ops/s
  zero gates clean
```

Raw logs:

```text
bench_results/20260623T101300Z_tls_model_local_r5.log
bench_results/20260623T101300Z_tls_model_interleaved_r5.log
```

## Decision

`TlsModelCodeShape-L1` is implemented for the startup-loaded v0 contract.  Do
not claim arbitrary late `dlopen()` support for this build shape.
