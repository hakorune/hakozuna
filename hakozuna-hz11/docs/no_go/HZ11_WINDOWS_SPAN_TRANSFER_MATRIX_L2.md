# HZ11 Windows SpanTransfer Matrix L2

Status: NO-GO for Windows selected row promotion.

## Question

Can the Linux HZ11 transfer/central span middle-end be enabled on the Windows
`hz11-span` bring-up row to improve the legacy allocator matrix `balanced` row?

## Lane

```text
hz11-span:
  HZ11_CLASSIFY_SPAN=1
  VirtualAlloc arena
  per-class returned-object sink

hz11-span-transfer:
  HZ11_CLASSIFY_SPAN=1
  HZ11_TRANSFER_CENTRAL_SPAN=1
  VirtualAlloc arena
  transfer cache + central object stack
```

The Windows port uses the minimal `hz11_port.h` mutex/once shim so the Linux
middle-end can compile without introducing a general pthread dependency.

## Evidence

```text
summary:
  docs/benchmarks/windows/hz11_l2_span_transfer_matrix_probe/20260709_170332_allocator_matrix.md

command:
  powershell -ExecutionPolicy Bypass -File .\win\run_win_allocator_matrix.ps1 `
    -Profiles smoke,balanced `
    -Allocators hz11-span,hz11-span-transfer `
    -OutputDir .\docs\benchmarks\windows\hz11_l2_span_transfer_matrix_probe `
    -ForceBuild -ContinueOnFailure -BenchTimeoutSeconds 120
```

```text
smoke:
  hz11-span           22.479M ops/s
  hz11-span-transfer 24.053M ops/s

balanced:
  hz11-span           13.945M ops/s, peak RSS 38180KB
  hz11-span-transfer 13.670M ops/s, peak RSS 1282312KB
```

## Reading

`hz11-span-transfer` does not solve the Windows matrix pressure row. It is
slightly slower on `balanced` and massively increases peak RSS.

```text
NO-GO:
  do not promote hz11-span-transfer as the Windows selected row
  do not treat Linux transfer/central as directly portable to Windows
  do not use this lane for Windows public claims

KEEP:
  keep the build row as opt-in evidence/control
  keep hz11-span as the Windows selected bring-up row
```

The likely issue is not basic compile portability; the lane builds and runs.
The problem is policy fit: the transfer/central middle-end retains far more
span-backed memory than the simple Windows `hz11-span` returned-object path in
this matrix shape.

## Next

```text
Prefer:
  Windows-specific transfer/RSS design, or smaller attribution first

Do not:
  continue tuning transfer capacity blindly
  call span-transfer a Windows default candidate
```
