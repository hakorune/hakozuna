# Current Task

This root file is the short project orientation ledger. Long chronological
HZ6/HZ5 experiment history was archived so the active task stays readable.

## Read First

```text
HZ6 stable lane docs:
  hakozuna-hz6/docs/HZ6_SELECTED_FAMILY_SUMMARY.md
  hakozuna-hz6/docs/HZ6_LANE_GUIDE.md
  hakozuna-hz6/docs/HZ6_UBUNTU_PRELOAD_LANES.md
  hakozuna-hz6/docs/HZ6_REPO_HYGIENE.md
  hakozuna-hz6/docs/HZ6_SOURCE_MODULARIZATION.md

Legacy/source cleanup lane:
  docs/HZ3_SOURCE_MODULARIZATION_LANE.md

Active HZ6 source cleanup lane:
  hakozuna-hz6/docs/HZ6_SOURCE_MODULARIZATION.md

Full archived root history:
  docs/archive/current_task_2026-06-hz6_linux_history.md

Detailed archived HZ6 pre-compaction ledger:
  hakozuna-hz6/docs/archive/current_task_2026-06-16_pre_compaction.md
```

## Current Focus

```text
HZ6 remains the active Windows/Linux implementation and benchmarking family.
HZ5 Linux remains profile-stabilized; new HZ5 work should not blur the HZ6
contract.

Source cleanup status:
  ./linux/audit_large_source_files.sh --top 20
  default source/script audit is clean after the HZ3/HZ6/Windows benchmark
  helper splits.

Next allocator work:
  continue from the HZ6 selected lane docs, not from this archived root ledger.
  Keep new benchmark rows mirrored into the HZ6 stable docs when promoted.
  Latest HZ6 Ubuntu production selected addition:
    HZ6_PRELOAD_PHASE_COUNT_COMPILED_OUT_L1=1
    Stats/diagnostic runners preserve phase counters for attribution.
    HZ6_DIRECT_LOCAL_REUSE_RAW_POP_L1=1
    Production direct-local reuse bypasses the generic frontcache pop wrapper;
    diagnostics keep wrapper counters.
    HZ6_MIDPAGE_DIRECT_LOCAL_REUSE_TRUSTED_CLASS_L1=1
    Ubuntu preload selected now uses the trusted MidPage class local-reuse
    success path after focused/fixed and shared-matrix guards passed.
    malloc_trim(size_t pad) is interposed by the HZ6 preload DSO as an
    explicit quiescent RSS release API; it scavenges HZ6 local-free payload
    before forwarding to libc malloc_trim.
  Latest HZ6 Ubuntu balance matrix:
    hakozuna-hz6/private/raw-results/linux/hz6_ubuntu_selected_balance_20260616_011316
    4096..16384 hz6 45.315M / 94.25 MiB
    4096..16384 hz3 51.230M / 73.38 MiB
    4096..16384 tcmalloc 34.618M / 100.25 MiB
  Latest HZ6 Ubuntu fixed-size matrix:
    hakozuna-hz6/private/raw-results/linux/hz6_ubuntu_size_slices_20260616_011443
    fixed_4k  hz6 31.542M / 91.88 MiB
    fixed_8k  hz6 43.506M / 93.25 MiB
    fixed_16k hz6 45.586M / 93.25 MiB
    read: fixed_16k now edges HZ3 speed in this repeat-3 read; fixed_8k is
      strong but below HZ3; fixed_4k remains a Toy/tcmalloc/HZ3 gap.
  Latest HZ6 Ubuntu quiescent RSS read:
    hakozuna-hz6/private/raw-results/linux/hz6_midpage_payload_trim_ab_20260616_012801
    hakozuna-hz6/private/raw-results/linux/hz6_fixed_rss_residency_refresh_20260616_021200
    malloc_trim keeps peak RSS flat but lowers current RSS:
      16..4096    79.88 MiB -> 27.27 MiB
      1024..4096  91.00 MiB -> 27.18 MiB
      4096..16384 94.25 MiB -> 28.52 MiB
      fixed_16k   93.38 MiB -> 28.38 MiB
    read: RSS progress is currently strongest as explicit quiescent recovery;
      peak RSS still mostly reflects touched MidPage source payload.
      fixed-size residency refresh keeps fixed_16k at 520 MiB logical 32K
      all-local-free/frontcache-retained payload with ref mismatch 0, so
      cold-retire/free-path release remains closed for default.
    cold_retire_max16 retires payload on the target row but does not reduce
      current/peak RSS enough and remains control/no-go.
  Latest HZ6 Ubuntu follow-up control:
    HZ6_PRELOAD_REALLOC_BOUNDARY_SLACK_4K_L1
    HZ6_PRELOAD_REALLOC_BOUNDARY_SLACK_8K_L1
    Split realloc-boundary profile DSOs are available for fixed_4k/fixed_8k
    workloads. They are not selected/default because focused and target guards
    remain mixed, but they directly attack the current fixed-boundary realloc
    copy pressure.
  Superseded HZ6 Ubuntu profile-position refresh:
    hakozuna-hz6/private/raw-results/linux/hz6_profile_position_focused_20260616_014250
    hakozuna-hz6/private/raw-results/linux/hz6_profile_position_fixed_20260616_014402
    small-boundary-fast was the strongest general profile DSO in that read:
      16..256      selected 58.182M -> 78.220M
      1024..4096   selected 34.045M -> 39.610M
      4096..16384  selected 45.518M -> 46.164M
      fixed_4k     selected 32.005M -> 48.188M
      fixed_16k    selected 45.614M -> 45.923M
    realloc-boundary-8k is the exact fixed_8k profile:
      fixed_8k selected 43.184M -> 45.960M
    read: this is retained as historical profile-position evidence; the newer
      small-boundary-trusted repeat below supersedes the broad recommendation.
  Latest HZ6 Ubuntu hygiene:
    hakozuna-hz6/linux/hz6_preload_aliases.sh now centralizes profile alias
    autobuild hooks for selected-balance and fixed-size matrix runners.
    run_hz6_midpage_payload_trim_ab.sh accepts small_boundary_fast as the
    A/B alias for small_boundary_raw_push_trusted_owner.
    validation raws:
      hakozuna-hz6/private/raw-results/linux/hz6_small_boundary_fast_alias_smoke_20260616_014954
      hakozuna-hz6/private/raw-results/linux/hz6_matrix_alias_helper_source_smoke_20260616_015014
      hakozuna-hz6/private/raw-results/linux/hz6_size_alias_helper_source_smoke_20260616_015021
  Latest HZ6 Ubuntu profile decomposition:
    hakozuna-hz6/private/raw-results/linux/hz6_midpage_payload_trim_ab_20260616_014717
    read: raw-push alone is weak; the profile win comes from the
      trusted-owner/raw-push combination. Keep it profile-only.
  Latest HZ6 Ubuntu profile DSO:
    hakozuna-hz6/linux/build_hz6_preload_small_boundary_trusted_target.sh
    aliases:
      hz6-small-boundary-trusted-target / hz6_small_boundary_trusted_target
    validation raws:
      hakozuna-hz6/private/raw-results/linux/hz6_small_boundary_trusted_alias_smoke_20260616_015323
      hakozuna-hz6/private/raw-results/linux/hz6_small_boundary_trusted_focused_20260616_015341
      hakozuna-hz6/private/raw-results/linux/hz6_small_boundary_trusted_position_20260616_015331
      hakozuna-hz6/private/raw-results/linux/hz6_trusted_profile_cross_20260616_020157
      hakozuna-hz6/private/raw-results/linux/hz6_trusted_profile_fixed_20260616_020211
    read: trusted supersedes fast as the preferred broad small/fixed HZ6
      profile. Latest repeat keeps trusted positive versus selected on focused
      and fixed-mid rows; selected default remains unchanged.
  Latest HZ6 Ubuntu aligned profile refresh:
    hakozuna-hz6/private/raw-results/linux/hz6_aligned_profile_refresh_20260616_020716
    hakozuna-hz6/private/raw-results/linux/hz6_aligned_profile_guard_20260616_020941
    hakozuna-hz6/private/raw-results/linux/hz6_aligned_profile_fixed_guard_20260616_020953
    read: aligned_free_skip recovers dedicated aligned allocation rows from
      about 14K ops/s to 9.4M..10.4M ops/s, but focused/fixed guards are mixed
      to negative. Keep hz6-aligned-target profile-only.
```

## Recent Cleanup Commits

```text
ce50edb Split Windows PowerShell benchmark helpers
f4ff246 Split Windows benchmark HZ6 stats helpers
e8b9d70 Split HZ3 scale part8 config helpers
61cdf18 Split HZ3 arena helpers
986634a Split HZ3 tcache slowpath helpers
a2b15cb Split HZ3 tcache helpers
```

## Rules

```text
Do not add long chronological benchmark logs to this root file.
Use current_task.md for orientation only.
Use family-specific docs for selected/default/no-go lane decisions.
Use docs/archive/current_task_2026-06-hz6_linux_history.md for old root notes.
Use hakozuna-hz6/docs/archive/current_task_2026-06-16_pre_compaction.md for
the detailed pre-compaction HZ6 ledger.
```
