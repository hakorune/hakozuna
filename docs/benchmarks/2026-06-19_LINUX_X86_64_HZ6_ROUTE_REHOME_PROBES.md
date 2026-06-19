# Linux x86_64 HZ6 Route Rehome Probe Observe, 2026-06-19

## Box

`RouteRehomeProbeObserve-L1`

This is a behavior-neutral diagnostic box. It splits committed route rehome
work into local route-table probe components:

- `route_rehome_origin_lookup_probe_total/max`
- `route_rehome_destination_register_probe_total/max`
- `route_rehome_origin_unregister_probe_total/max`

The selected runtime flags are unchanged.

## Integrity Smoke

Command:

```bash
./hakozuna-hz6/linux/run_hz6_preload_integrity_smoke.sh
```

Result: passed.

Key counters:

```text
route_rehome_success=2465
route_rehome_directory_transfer_probe_total=2497
route_rehome_directory_transfer_probe_max=3
route_rehome_origin_lookup_probe_total=2522
route_rehome_origin_lookup_probe_max=3
route_rehome_destination_register_probe_total=2504
route_rehome_destination_register_probe_max=2
route_rehome_origin_unregister_probe_total=2522
route_rehome_origin_unregister_probe_max=3
transfer_reserve_full=34734
remote_free_returned_uncommitted=34734
transfer_reserve_full_after_state_mutation=0
```

Interpretation: committed rehome local route-table probes are already short,
roughly one probe per committed rehome phase in this smoke. The remaining
remote90 gap is more likely from transfer backpressure and total route movement
than from long exact-table probe chains inside rehome.

## Quick Perf Guard

Command:

```bash
RUNS=3 ./hakozuna-hz6/linux/run_hz6_preload_remote_median.sh
```

Result:

```text
remote50  13,177,127.52 ops/s
remote90   7,498,798.29 ops/s
```

This is a diagnostic/tooling result, not a new selected performance claim.

## Decision

`GO(tooling)`.

Keep the counters for phase selection. Do not prioritize direct proof-token
shared transfer or route-table probe shortening as the next primary performance
box. Focus next on reducing committed route movement, making backpressure
semantics explicit, or changing the transfer saturation policy.
