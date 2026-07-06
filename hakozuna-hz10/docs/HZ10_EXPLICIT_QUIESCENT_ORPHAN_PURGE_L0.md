# HZ10 Explicit Quiescent Orphan Purge L0

Status: implemented + measured; GO as explicit API, not automatic.

## Purpose

`HZ10OrphanRegistryDrainPotential-L0` showed that the remote-heavy post-RSS
residual is freed-but-undrained orphan capacity. At a caller-provided
quiescent boundary, draining those orphan pages turns them fully idle.

This box adds the explicit boundary API:

```c
void hz10_public_entry_purge_orphan_registry_quiescent(
    Hz10OrphanRegistryPurgeStats* stats_out);
```

## Contract

The caller must already be globally quiescent for orphan pages:

```text
- worker/producer threads that could free into orphan pages are joined/stopped
- caller-owned handoff queues are drained
- no free is between remote claim and publish
- no live allocation path is adopting from the orphan registry concurrently
```

The function is never run from pthread destructors and never from the
malloc/free hot path.

## Mechanism

Under the orphan-registry lock:

```text
for each orphan ACTIVE page:
  skip pages whose old owner is not EXITED
  temporarily set page owner to the purge sentinel owner
  hz10_page_drain_remote(page)
  if free_count == slot_count:
    unlink from registry and enqueue for destroy
  else:
    restore the old owner token and keep the page registered
```

After releasing the registry lock, the destroy list is returned through
`hz10_pooled_page_destroy()`. That keeps registry mutation bounded and avoids
doing pool/release work under the registry lock.

## Safety Notes

`hz10_page_drain_remote()` remains owner-only. The purge path does not perform
ownerless drain; it uses the same temporary-owner proof as the drain-potential
probe and partial adoption transfer step.

Destroy only happens after:

```text
old owner state == EXITED
temporary-owner drain completed
free_count == slot_count
page has been unlinked from the orphan registry
```

Non-idle pages are restored to the old owner token and left in the registry.

## Gate

Smoke coverage:

```text
smoke-public-entry-orphan-adoption
```

The smoke creates an orphan page, frees every slot remotely after owner exit,
calls the quiescent purge, and verifies that reclaimed pages and merged slots
are reported and the old pointer no longer routes to the same page.

## Measurement

Formal HZ10 lane:

```text
THREADS=16 ITERS=50000 RUNS=3
allocator: HZ10 libhz10.so via LD_PRELOAD
script: scripts/run_hz10_hz8_public_purge_matrix.sh
```

Median:

```text
row                         baseline RSS  purge RSS   delta
small_interleaved_remote90   41.81MB       5.85MB    -35.96MB
main_interleaved_r90         99.09MB       8.22MB    -90.87MB
medium_interleaved_r50       70.12MB       7.38MB    -62.75MB
main_local0                  35.26MB       3.99MB    -31.27MB
medium_local0                 6.42MB       3.18MB     -3.24MB
```

Work throughput is measured before the purge point; small ops/s deltas in the
A/B table are run-order noise, not hot-path cost.

Record:

```text
bench_results/20260707T_hz10_hz8_public_purge_matrix_l0/
```

Verdict: GO. The explicit API is the correct mechanism for post-workload RSS
recovery. It should remain manual; atexit purge is still not a claim for the
HZ8 post-RSS gate because that sample happens before process exit.
