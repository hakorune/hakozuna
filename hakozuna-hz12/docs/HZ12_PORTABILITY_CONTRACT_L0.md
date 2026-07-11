# HZ12 Portability Contract L0

HZ12 shares allocator semantics, not backing implementations.

```text
Shared:
  advisory owner token and generation
  owner alive / dead / adopted state
  ownerless fallback on inbox overflow
  flush-time batch routing contract
  reclaim eligibility contract
  route validity independent of ownership

Windows-specific:
  VirtualAlloc / VirtualFree backing
  Windows TLS and thread-exit behavior
  CRITICAL_SECTION or mutex inbox implementation
  native xowner pipeline and RSS collection

Linux-specific:
  mmap / madvise backing
  pthread TLS and thread-exit behavior
  Linux preload and real-app harnesses
```

Linux HZ12 uses `MADV_DONTNEED` for detached-span discard. The arena mapping
stays reserved, and recycle faults the same virtual span back in before route
and owner metadata are restored. `src/hz12_span_backing.*` is the sole OS
backing boundary for discard, recommit, and rollback.

The Windows L0 result gates Linux behavior work. Linux may receive the same
shadow contract after the Windows routing signal is proven, but it does not
receive a copy of Windows backing code.
