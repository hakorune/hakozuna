# HZ9 Local Slab Public Entry L0

## Purpose

```text
move HZ9 from micro substrate proof toward public allocator shape
without opening preload, remote protocol, or RSS policy yet
```

## Scope

```text
entry:
  h9_lsp_debug_public_malloc(size)
  h9_lsp_debug_public_free(ptr, owned_out)

fast local path:
  size -> medium class
  TLS active routeleafcompact entry
  same-thread exact last-token free skips route

fallback:
  direct-owned route remains canonical authority for miss / invalid / non-LIFO

non-goals:
  no preload integration
  no multithread remote protocol
  no RSS/cache cap policy
  no HZ8 default behavior change
```

## Measurement Rules

```text
honesty:
  asm audit must keep slot_select = 1 for gate candidates

stability:
  local throughput gate should run through ASLR OFF harness when available
  record explicit ASLR status

baseline:
  routeleafcompact remains current local honest baseline on this host
```

## Gates

```text
publicentry local:
  target is not 800M phantom ceiling
  compare against routeleafcompact honest baseline

correctness:
  exact free accepted
  double free rejected as owned invalid
  interior pointer rejected as owned invalid
  miss pointer not owned
  non-LIFO fallback route_valid / ptr_fallback clean

decision:
  if publicentry is far below routeleafcompact, fix entry shape before preload
```

## L0 Result

```text
ASLR OFF, CLASS_ID=5, ITERS=30000000, TOUCH=1:
  routeleafcompact:
    median 331.698M

  publicentry:
    median 103.979M

  publicentrynonlifo:
    median 80.545M
    route_valid = 30000000
    ptr_fallback = 30000000
    state_mismatch = 0

smoke:
  public exact route VALID
  public interior INVALID
  public after-free INVALID
  public double free rejected as owned
  public miss free not owned

asm audit:
  public_malloc:
    honest-candidate
    slot_select = 1

  public_free:
    route-authority-candidate
    route_call = 1

read:
  public-shaped authority is correct but expensive.
  Synchronizing entry-local bits back to routeable segment state drops LIFO
  throughput by about 3x versus routeleafcompact.

decision:
  PublicEntry-L0 is correct evidence, not performance-ready.
  Do not open preload until public entry shape is improved.
```
