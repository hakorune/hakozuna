# HZ9 Include

HZ9 currently keeps the inherited `h8.h` API surface while the allocator is
experimental.

Do not rename the public API in L0. Keep comparisons against the copied HZ8
baseline mechanically small, then decide on public `h9_*` naming only after the
architecture is viable.

`h8.h` includes `h8_stats_fields.inc` inside `H8Stats` to keep the public header
under the repository line-count budget while preserving the inherited ABI shape.
