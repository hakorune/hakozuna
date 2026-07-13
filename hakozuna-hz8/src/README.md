# HZ8 Source Map

This file is the behavior-neutral ownership map for `src/`. It keeps module
boundaries visible without changing allocator behavior.

## Public Entry And Runtime

- `h8_core.c`: public allocation/free entry and top-level dispatch
- `h8_runtime.c`, `h8_runtime_types.h`: process/thread runtime state
- `h8_platform.c`: OS backing and synchronization boundary
- `h8_route.c`: route classification and fail-closed validity contract

## Small Objects

- `h8_small_local.c`: local small-span allocation/free and Mag16 behavior
- `h8_remote_inbox.c`: remote publication and owner-side collection
- `h8_small_partial_transition_depot.c`: P1 research-only recovery behavior

P1 and its diagnostics stay behind compile-time research flags. They are not
part of the public speed/default build.

## Medium Objects

- `h8_medium*.c`, `h8_medium*.inc`: medium run, directory, residency, and
  GeneralMediumPage implementation
- `h8_medium_page8k_remote.c`: detached Page8K ownership/remote contract
- `h8_medium_domain_shadow.c`: diagnostic domain/shadow evidence only

The medium implementation is intentionally split by responsibility. Do not
merge diagnostic shadow accounting into the speed path.

## Large Objects And Diagnostics

- `h8_direct_large.c` and `h8_direct_large_*.inc`: direct-large profiles and
  opt-in cache evidence
- `h8_stats*.c`, `h8_stats*.inc`: counter/report implementation

LargeDirect cache variants and counter-bearing builds are research or
diagnostic lanes. Build-target existence is not a default-support claim.

## Cleanup Rules

1. Preserve `MISS` / `VALID` / `INVALID` and generation checks.
2. Keep production hot paths free of diagnostic atomics.
3. Keep research behavior behind named compile-time flags.
4. Split a module only when ownership is clear and tests can exercise both
   sides; do not refactor merely to reduce line counts.
5. Update the platform lane registries when adding or retiring a behavior box.
