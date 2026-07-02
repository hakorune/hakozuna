# HZ9 Bench

HZ9 benches build from the standalone HZ9 tree.

```text
make -C hakozuna-hz9 bench-release
./hakozuna-hz9/h8_bench_release
```

Historical active-run magazine targets remain available for comparison:

```text
make -C hakozuna-hz9 bench-release-hz9mediumlocalmag
make -C hakozuna-hz9 bench-release-hz9mediumlocalmag-disabled
```

Standalone comparison scripts default to local HZ9 artifacts plus `system`.
External allocator rows are opt-in: set `ALLOCATORS` and the relevant `*_SO`
environment variables explicitly.

Preferred public baseline matrix wrapper:

```bash
RUNS=10 hakozuna-hz9/scripts/run_hz9_baseline_public_matrix.sh
ALLOCATORS=hz9,mimalloc,tcmalloc,system RUNS=10 \
  hakozuna-hz9/scripts/run_hz9_baseline_public_matrix.sh
```

The first command is local-only by default. The second command is the explicit
external-allocator comparison form. The older `run_hz8_*` matrix scripts are
compatibility hooks over artifacts built inside this HZ9 tree. They do not
require a sibling HZ8 checkout.

`bench/lib/bench_common.sh` may print optional hints for root-level Linux or
macOS release helper scripts when external allocator `.so` files are missing.
Those helpers are convenience pointers for cross-tree comparison, not HZ9 build
dependencies. HZ9 standalone checks and local-only bench runs must pass without
them.

Primary rows for the current slab route/slab page lane:

```text
fixed64_local0
medium_local0
main_local0
medium_interleaved_r50
main_interleaved_r90
small guard / remote safety rows
```

Current active-run magazine and TLS object-cache evidence is HOLD. The next HZ9
behavior must not return independent slab pointers until the HZ9 slab route
boundary covers `free`, `route`, `usable_size`, and `realloc`.

HZ9 local slab/page shadow probe:

```bash
RUNS=3 THREADS=8 ITERS=30000 \
  hakozuna-hz9/scripts/run_hz9_slab_shadow_probe.sh
```

HZ9 slab/page RSS and cap debug probe:

```bash
RUNS=3 THREADS=8 ITERS=30000 \
  hakozuna-hz9/scripts/run_hz9_slabpage_rss_probe.sh
```

Set `VARIANTS=baseline,slab4,slab6,slab8` to compare page caps.
