param(
    [string]$OutDirName = "out_win_shadow",
    [int]$InboxCap = 1024,
    [int]$DrainInterval = 256,
    [string]$InboxOutputName = "bench_hz12_xowner_inbox.exe"
)

$ErrorActionPreference = "Stop"
$Hz12Root = Split-Path -Parent $PSScriptRoot
$RepoRoot = Split-Path -Parent $Hz12Root
$OutDir = Join-Path $Hz12Root $OutDirName
$Bench = Join-Path $Hz12Root "bench\bench_hz12_xowner_shadow.c"
$InboxBench = Join-Path $Hz12Root "bench\bench_hz12_xowner_inbox.c"
$TokenRetireLive = Join-Path $Hz12Root "bench\bench_hz12_token_retire_live.c"
$TokenXownerPipeline = Join-Path $Hz12Root "bench\bench_hz12_token_xowner_pipeline.c"
$WideWsReclaimShadow = Join-Path $Hz12Root "bench\bench_hz12_widews_reclaim_shadow.c"
$WideWsOwnerLedgerShadow = Join-Path $Hz12Root "bench\bench_hz12_widews_owner_ledger_shadow.c"
$AdoptionSmoke = Join-Path $Hz12Root "tests\hz12_owner_adoption_shadow_smoke.c"
$RetiredAdoptionSmoke = Join-Path $Hz12Root "tests\hz12_retired_inbox_adoption_smoke.c"
$WholeSpanSmoke = Join-Path $Hz12Root "tests\hz12_whole_span_accounting_smoke.c"
$DepotCycleSmoke = Join-Path $Hz12Root "tests\hz12_span_depot_cycle_smoke.c"
$DepotCapSmoke = Join-Path $Hz12Root "tests\hz12_span_depot_cap_smoke.c"
$OwnerRegistrySmoke = Join-Path $Hz12Root "tests\hz12_owner_registry_smoke.c"
$TokenInboxSmoke = Join-Path $Hz12Root "tests\hz12_token_inbox_smoke.c"
$OwnerEpochSmoke = Join-Path $Hz12Root "tests\hz12_owner_epoch_smoke.c"
$OwnerRetireGateSmoke = Join-Path $Hz12Root "tests\hz12_owner_retire_gate_smoke.c"
$RetiredReclaimShadowSmoke = Join-Path $Hz12Root "tests\hz12_retired_reclaim_shadow_smoke.c"
$OwnerBatchLedgerSmoke = Join-Path $Hz12Root "tests\hz12_owner_batch_ledger_smoke.c"
$OwnerBatchLedgerBoundarySmoke = Join-Path $Hz12Root "tests\hz12_owner_batch_ledger_boundary_smoke.c"
$OwnerBatchLedgerXownerSmoke = Join-Path $Hz12Root "tests\hz12_owner_batch_ledger_xowner_smoke.c"
$Shadow = Join-Path $Hz12Root "src\hz12_shadow.c"
$Inbox = Join-Path $Hz12Root "src\hz12_inbox.c"
$Accounting = Join-Path $Hz12Root "src\hz12_span_accounting.c"
$ReclaimGate = Join-Path $Hz12Root "src\hz12_reclaim_gate.c"
$SpanDetach = Join-Path $Hz12Root "src\hz12_span_detach.c"
$SpanDecommit = Join-Path $Hz12Root "src\hz12_span_decommit.c"
$SpanDepot = Join-Path $Hz12Root "src\hz12_span_depot.c"
$SpanDepotCore = Join-Path $Hz12Root "src\hz12_span_depot_core.c"
$OwnerRegistry = Join-Path $Hz12Root "src\hz12_owner_registry.c"
$TokenInbox = Join-Path $Hz12Root "src\hz12_token_inbox.c"
$OwnerEpoch = Join-Path $Hz12Root "src\hz12_owner_epoch.c"
$OwnerRetireGate = Join-Path $Hz12Root "src\hz12_owner_retire_gate.c"
$SpanOwnerShadow = Join-Path $Hz12Root "src\hz12_span_owner_shadow.c"
$RetiredReclaimShadow = Join-Path $Hz12Root "src\hz12_retired_reclaim_shadow.c"
$RetiredReclaimDetach = Join-Path $Hz12Root "src\hz12_retired_reclaim_detach.c"
$RetiredReclaimDecommit = Join-Path $Hz12Root "src\hz12_retired_reclaim_decommit.c"
$RetiredReclaimDepotCycle = Join-Path $Hz12Root "src\hz12_retired_reclaim_depot_cycle.c"
$ReclaimCarveDiag = Join-Path $Hz12Root "src\hz12_reclaim_carve_diag.c"
$RetiredReclaimRecycle = Join-Path $Hz12Root "src\hz12_retired_reclaim_recycle.c"
$OwnerBatchLedger = Join-Path $Hz12Root "src\hz12_owner_batch_ledger.c"
$OwnerBatchLedgerCompare = Join-Path $Hz12Root "src\hz12_owner_batch_ledger_compare.c"
$OwnerLedgerRetireGate = Join-Path $Hz12Root "src\hz12_owner_ledger_retire_gate.c"
$SnapshotReclaim = Join-Path $Hz12Root "src\hz12_snapshot_reclaim.c"
$SnapshotRecycle = Join-Path $Hz12Root "src\hz12_snapshot_recycle.c"
$ReclaimPolicyShadow = Join-Path $Hz12Root "src\hz12_reclaim_policy_shadow.c"
$ReclaimEntry = Join-Path $Hz12Root "src\hz12_reclaim_entry.c"
$Hz12Sources = @(
    "$Hz12Root\src\hz12_current_span_install.c",
    "$Hz12Root\src\hz12_size_class.c",
    "$Hz12Root\src\hz12_sys_alloc.c",
    "$Hz12Root\src\hz12_thread_cache.c",
    "$Hz12Root\src\hz12_thread_cache_diag.c",
    "$Hz12Root\src\hz12_public_entry.c",
    "$Hz12Root\src\hz12_span.c",
    "$Hz12Root\src\hz12_live_footprint.c"
)

foreach ($path in @($Bench, $InboxBench, $TokenRetireLive, $TokenXownerPipeline, $WideWsReclaimShadow, $WideWsOwnerLedgerShadow, $AdoptionSmoke, $RetiredAdoptionSmoke, $WholeSpanSmoke, $DepotCycleSmoke, $DepotCapSmoke, $OwnerRegistrySmoke, $TokenInboxSmoke, $OwnerEpochSmoke, $OwnerRetireGateSmoke, $RetiredReclaimShadowSmoke, $OwnerBatchLedgerSmoke, $OwnerBatchLedgerBoundarySmoke, $OwnerBatchLedgerXownerSmoke, $Shadow, $Inbox, $Accounting, $ReclaimGate, $SpanDetach, $SpanDecommit, $SpanDepot, $SpanDepotCore, $OwnerRegistry, $TokenInbox, $OwnerEpoch, $OwnerRetireGate, $SpanOwnerShadow, $RetiredReclaimShadow, $RetiredReclaimDetach, $RetiredReclaimDecommit, $RetiredReclaimDepotCycle, $ReclaimCarveDiag, $RetiredReclaimRecycle, $OwnerBatchLedger, $OwnerBatchLedgerCompare, $OwnerLedgerRetireGate, $SnapshotReclaim, $SnapshotRecycle, $ReclaimPolicyShadow, $ReclaimEntry) + $Hz12Sources) {
    if (-not (Test-Path $path)) { throw "Missing HZ12 shadow source: $path" }
}
if ($InboxCap -lt 1) { throw "InboxCap must be positive." }
if ($DrainInterval -lt 1 -or ($DrainInterval -band ($DrainInterval - 1)) -ne 0) {
    throw "DrainInterval must be a power of two."
}
New-Item -ItemType Directory -Force $OutDir | Out-Null

$compilerArgs = @(
    "/nologo", "/O2", "/DNDEBUG", "/std:c11", "/W3", "/MD",
    "/I$(Join-Path $RepoRoot 'win')",
    "/I$(Join-Path $Hz12Root 'src')",
    "/I$(Join-Path $Hz12Root 'include')",
    "/I$(Join-Path $Hz12Root 'src')",
    "/DHZ12_CLASSIFY_SPAN=1",
    "/DHZ12_CACHE_CAP=256",
    $Bench, $Shadow
) + $Hz12Sources + @(
    "psapi.lib", "/link",
    "/out:$(Join-Path $OutDir 'bench_hz12_xowner_shadow.exe')"
)

& clang-cl @compilerArgs
if ($LASTEXITCODE -ne 0) { throw "clang-cl failed: $LASTEXITCODE" }

$inboxArgs = @(
    "/nologo", "/O2", "/DNDEBUG", "/std:c11", "/W3", "/MD",
    "/I$(Join-Path $RepoRoot 'win')",
    "/I$(Join-Path $Hz12Root 'src')",
    "/I$(Join-Path $Hz12Root 'include')",
    "/I$(Join-Path $Hz12Root 'src')",
    "/DHZ12_CLASSIFY_SPAN=1",
    "/DHZ12_CACHE_CAP=256",
    "/DHZ12_INBOX_CAP=$InboxCap",
    "/DHZ12_DRAIN_INTERVAL=$DrainInterval",
    $InboxBench, $Shadow, $Inbox, $Accounting
) + $Hz12Sources + @(
    "psapi.lib", "/link",
    "/out:$(Join-Path $OutDir $InboxOutputName)"
)
& clang-cl @inboxArgs
if ($LASTEXITCODE -ne 0) { throw "clang-cl failed: $LASTEXITCODE" }

$inboxNoAccountingArgs = @(
    "/nologo", "/O2", "/DNDEBUG", "/std:c11", "/W3", "/MD",
    "/I$(Join-Path $RepoRoot 'win')",
    "/I$(Join-Path $Hz12Root 'src')",
    "/I$(Join-Path $Hz12Root 'include')",
    "/DHZ12_CLASSIFY_SPAN=1",
    "/DHZ12_CACHE_CAP=256",
    "/DHZ12_INBOX_CAP=$InboxCap",
    "/DHZ12_DRAIN_INTERVAL=$DrainInterval",
    "/DHZ12_XOWNER_ACCOUNTING=0",
    "/DHZ12_INBOX_ACCOUNTING=0",
    $InboxBench, $Shadow, $Inbox
) + $Hz12Sources + @(
    "psapi.lib", "/link",
    "/out:$(Join-Path $OutDir 'bench_hz12_xowner_inbox_noaccounting.exe')"
)
& clang-cl @inboxNoAccountingArgs
if ($LASTEXITCODE -ne 0) { throw "clang-cl failed: $LASTEXITCODE" }

$inboxSpeedArgs = @(
    "/nologo", "/O2", "/DNDEBUG", "/std:c11", "/W3", "/MD",
    "/I$(Join-Path $RepoRoot 'win')",
    "/I$(Join-Path $Hz12Root 'src')",
    "/I$(Join-Path $Hz12Root 'include')",
    "/DHZ12_CLASSIFY_SPAN=1",
    "/DHZ12_CACHE_CAP=256",
    "/DHZ12_INBOX_CAP=$InboxCap",
    "/DHZ12_DRAIN_INTERVAL=$DrainInterval",
    "/DHZ12_XOWNER_ACCOUNTING=0",
    "/DHZ12_INBOX_ACCOUNTING=0",
    "/DHZ12_SHADOW_DIAG_COUNTERS=0",
    "/DHZ12_INBOX_DIAG_COUNTERS=0",
    $InboxBench, $Shadow, $Inbox
) + $Hz12Sources + @(
    "psapi.lib", "/link",
    "/out:$(Join-Path $OutDir 'bench_hz12_xowner_inbox_speed.exe')"
)
& clang-cl @inboxSpeedArgs
if ($LASTEXITCODE -ne 0) { throw "clang-cl failed: $LASTEXITCODE" }

$ownerFastLoadArgs = @(
    "/nologo", "/O2", "/DNDEBUG", "/std:c11", "/W3", "/MD",
    "/I$(Join-Path $RepoRoot 'win')",
    "/I$(Join-Path $Hz12Root 'src')",
    "/I$(Join-Path $Hz12Root 'include')",
    "/DHZ12_CLASSIFY_SPAN=1",
    "/DHZ12_CACHE_CAP=256",
    "/DHZ12_INBOX_CAP=$InboxCap",
    "/DHZ12_DRAIN_INTERVAL=$DrainInterval",
    "/DHZ12_XOWNER_ACCOUNTING=0",
    "/DHZ12_INBOX_ACCOUNTING=0",
    "/DHZ12_SHADOW_DIAG_COUNTERS=0",
    "/DHZ12_INBOX_DIAG_COUNTERS=0",
    "/DHZ12_SHADOW_OWNER_FAST_LOAD=1",
    $InboxBench, $Shadow, $Inbox
) + $Hz12Sources + @(
    "psapi.lib", "/link",
    "/out:$(Join-Path $OutDir 'bench_hz12_xowner_inbox_ownerfastload.exe')"
)
& clang-cl @ownerFastLoadArgs
if ($LASTEXITCODE -ne 0) { throw "clang-cl failed: $LASTEXITCODE" }

$bareCoreArgs = @(
    "/nologo", "/O2", "/DNDEBUG", "/std:c11", "/W3", "/MD",
    "/I$(Join-Path $RepoRoot 'win')",
    "/I$(Join-Path $Hz12Root 'src')",
    "/I$(Join-Path $Hz12Root 'include')",
    "/DHZ12_CLASSIFY_SPAN=1",
    "/DHZ12_CACHE_CAP=256",
    "/DHZ12_XOWNER_ACCOUNTING=0",
    "/DHZ12_XOWNER_BARE_CORE=1",
    $InboxBench
) + $Hz12Sources + @(
    "psapi.lib", "/link",
    "/out:$(Join-Path $OutDir 'bench_hz12_xowner_bare_core.exe')"
)
& clang-cl @bareCoreArgs
if ($LASTEXITCODE -ne 0) { throw "clang-cl failed: $LASTEXITCODE" }

$adoptionSmokeArgs = @(
    "/nologo", "/O2", "/DNDEBUG", "/std:c11", "/W3", "/MD",
    "/I$(Join-Path $RepoRoot 'win')",
    "/I$(Join-Path $Hz12Root 'src')",
    "/I$(Join-Path $Hz12Root 'include')",
    "/DHZ12_CLASSIFY_SPAN=1",
    "/DHZ12_CACHE_CAP=256",
    $AdoptionSmoke, $Shadow, $Inbox, $Accounting
) + $Hz12Sources + @(
    "psapi.lib", "/link",
    "/out:$(Join-Path $OutDir 'hz12_owner_adoption_shadow_smoke.exe')"
)
& clang-cl @adoptionSmokeArgs
if ($LASTEXITCODE -ne 0) { throw "clang-cl failed: $LASTEXITCODE" }

$retiredAdoptionSmokeArgs = @(
    "/nologo", "/O2", "/DNDEBUG", "/std:c11", "/W3", "/MD",
    "/I$(Join-Path $RepoRoot 'win')",
    "/I$(Join-Path $Hz12Root 'src')",
    "/I$(Join-Path $Hz12Root 'include')",
    "/DHZ12_CLASSIFY_SPAN=1",
    "/DHZ12_CACHE_CAP=256",
    $RetiredAdoptionSmoke, $Shadow, $Inbox, $Accounting
) + $Hz12Sources + @(
    "psapi.lib", "/link",
    "/out:$(Join-Path $OutDir 'hz12_retired_inbox_adoption_smoke.exe')"
)
& clang-cl @retiredAdoptionSmokeArgs
if ($LASTEXITCODE -ne 0) { throw "clang-cl failed: $LASTEXITCODE" }

$wholeSpanSmokeArgs = @(
    "/nologo", "/O2", "/DNDEBUG", "/std:c11", "/W3", "/MD",
    "/I$(Join-Path $RepoRoot 'win')",
    "/I$(Join-Path $Hz12Root 'src')",
    "/I$(Join-Path $Hz12Root 'include')",
    "/DHZ12_CLASSIFY_SPAN=1",
    "/DHZ12_CACHE_CAP=256",
    $WholeSpanSmoke, $Shadow, $Inbox, $Accounting, $ReclaimGate, $SpanDetach,
    $SpanDecommit, $SpanDepot, $SpanDepotCore
) + $Hz12Sources + @(
    "psapi.lib", "/link",
    "/out:$(Join-Path $OutDir 'hz12_whole_span_accounting_smoke.exe')"
)
& clang-cl @wholeSpanSmokeArgs
if ($LASTEXITCODE -ne 0) { throw "clang-cl failed: $LASTEXITCODE" }

$depotCycleSmokeArgs = @(
    "/nologo", "/O2", "/DNDEBUG", "/std:c11", "/W3", "/MD",
    "/I$(Join-Path $RepoRoot 'win')",
    "/I$(Join-Path $Hz12Root 'src')",
    "/I$(Join-Path $Hz12Root 'include')",
    "/DHZ12_CLASSIFY_SPAN=1",
    "/DHZ12_CACHE_CAP=256",
    $DepotCycleSmoke, $Shadow, $Inbox, $Accounting, $ReclaimGate, $SpanDetach,
    $SpanDecommit, $SpanDepot, $SpanDepotCore
) + $Hz12Sources + @(
    "psapi.lib", "/link",
    "/out:$(Join-Path $OutDir 'hz12_span_depot_cycle_smoke.exe')"
)
& clang-cl @depotCycleSmokeArgs
if ($LASTEXITCODE -ne 0) { throw "clang-cl failed: $LASTEXITCODE" }

$depotCapSmokeArgs = @(
    "/nologo", "/O2", "/DNDEBUG", "/std:c11", "/W3", "/MD",
    "/I$(Join-Path $RepoRoot 'win')",
    "/I$(Join-Path $Hz12Root 'src')",
    "/I$(Join-Path $Hz12Root 'include')",
    "/DHZ12_CLASSIFY_SPAN=1",
    "/DHZ12_CACHE_CAP=256",
    $DepotCapSmoke, $Shadow, $Inbox, $Accounting, $ReclaimGate, $SpanDetach,
    $SpanDecommit, $SpanDepot, $SpanDepotCore
) + $Hz12Sources + @(
    "psapi.lib", "/link",
    "/out:$(Join-Path $OutDir 'hz12_span_depot_cap_smoke.exe')"
)
& clang-cl @depotCapSmokeArgs
if ($LASTEXITCODE -ne 0) { throw "clang-cl failed: $LASTEXITCODE" }

$ownerRegistrySmokeArgs = @(
    "/nologo", "/O2", "/DNDEBUG", "/std:c11", "/W3", "/MD",
    "/I$(Join-Path $Hz12Root 'src')",
    $OwnerRegistrySmoke, $OwnerRegistry,
    "/link", "/out:$(Join-Path $OutDir 'hz12_owner_registry_smoke.exe')"
)
& clang-cl @ownerRegistrySmokeArgs
if ($LASTEXITCODE -ne 0) { throw "clang-cl failed: $LASTEXITCODE" }

$tokenInboxSmokeArgs = @(
    "/nologo", "/O2", "/DNDEBUG", "/std:c11", "/W3", "/MD",
    "/I$(Join-Path $RepoRoot 'win')",
    "/I$(Join-Path $Hz12Root 'src')",
    "/I$(Join-Path $Hz12Root 'include')",
    "/DHZ12_CLASSIFY_SPAN=1",
    "/DHZ12_CACHE_CAP=256",
    $TokenInboxSmoke, $TokenInbox, $OwnerRegistry
) + $Hz12Sources + @(
    "psapi.lib", "/link",
    "/out:$(Join-Path $OutDir 'hz12_token_inbox_smoke.exe')"
)
& clang-cl @tokenInboxSmokeArgs
if ($LASTEXITCODE -ne 0) { throw "clang-cl failed: $LASTEXITCODE" }

$ownerEpochSmokeArgs = @(
    "/nologo", "/O2", "/DNDEBUG", "/std:c11", "/W3", "/MD",
    "/I$(Join-Path $Hz12Root 'src')",
    $OwnerEpochSmoke, $OwnerEpoch, $OwnerRegistry,
    "/link", "/out:$(Join-Path $OutDir 'hz12_owner_epoch_smoke.exe')"
)
& clang-cl @ownerEpochSmokeArgs
if ($LASTEXITCODE -ne 0) { throw "clang-cl failed: $LASTEXITCODE" }

$ownerRetireGateSmokeArgs = @(
    "/nologo", "/O2", "/DNDEBUG", "/std:c11", "/W3", "/MD",
    "/I$(Join-Path $RepoRoot 'win')",
    "/I$(Join-Path $Hz12Root 'src')",
    "/I$(Join-Path $Hz12Root 'include')",
    "/DHZ12_CLASSIFY_SPAN=1",
    "/DHZ12_CACHE_CAP=256",
    $OwnerRetireGateSmoke, $OwnerRetireGate, $OwnerEpoch, $TokenInbox,
    $OwnerRegistry
) + $Hz12Sources + @(
    "psapi.lib", "/link",
    "/out:$(Join-Path $OutDir 'hz12_owner_retire_gate_smoke.exe')"
)
& clang-cl @ownerRetireGateSmokeArgs
if ($LASTEXITCODE -ne 0) { throw "clang-cl failed: $LASTEXITCODE" }

$tokenRetireLiveArgs = @(
    "/nologo", "/O2", "/DNDEBUG", "/std:c11", "/W3", "/MD",
    "/I$(Join-Path $RepoRoot 'win')",
    "/I$(Join-Path $Hz12Root 'src')",
    "/I$(Join-Path $Hz12Root 'include')",
    "/DHZ12_CLASSIFY_SPAN=1",
    "/DHZ12_CACHE_CAP=256",
    $TokenRetireLive, $OwnerRetireGate, $OwnerEpoch, $TokenInbox,
    $OwnerRegistry
) + $Hz12Sources + @(
    "psapi.lib", "/link",
    "/out:$(Join-Path $OutDir 'bench_hz12_token_retire_live.exe')"
)
& clang-cl @tokenRetireLiveArgs
if ($LASTEXITCODE -ne 0) { throw "clang-cl failed: $LASTEXITCODE" }

$tokenXownerPipelineArgs = @(
    "/nologo", "/O2", "/DNDEBUG", "/std:c11", "/W3", "/MD",
    "/I$(Join-Path $RepoRoot 'win')",
    "/I$(Join-Path $Hz12Root 'src')",
    "/I$(Join-Path $Hz12Root 'include')",
    "/DHZ12_CLASSIFY_SPAN=1",
    "/DHZ12_CACHE_CAP=256",
    $TokenXownerPipeline, $Shadow, $Accounting, $OwnerRetireGate, $OwnerEpoch,
    $TokenInbox, $OwnerRegistry
) + $Hz12Sources + @(
    "psapi.lib", "/link",
    "/out:$(Join-Path $OutDir 'bench_hz12_token_xowner_pipeline.exe')"
)
& clang-cl @tokenXownerPipelineArgs
if ($LASTEXITCODE -ne 0) { throw "clang-cl failed: $LASTEXITCODE" }

$retiredReclaimShadowSmokeArgs = @(
    "/nologo", "/O2", "/DNDEBUG", "/std:c11", "/W3", "/MD",
    "/I$(Join-Path $RepoRoot 'win')",
    "/I$(Join-Path $Hz12Root 'src')",
    "/I$(Join-Path $Hz12Root 'include')",
    "/DHZ12_CLASSIFY_SPAN=1",
    "/DHZ12_CACHE_CAP=256",
    $RetiredReclaimShadowSmoke, $RetiredReclaimShadow, $SpanOwnerShadow,
    $OwnerRetireGate, $OwnerEpoch, $TokenInbox, $OwnerRegistry, $Shadow,
    $Inbox, $Accounting, $ReclaimGate, $SpanDetach
) + $Hz12Sources + @(
    "psapi.lib", "/link",
    "/out:$(Join-Path $OutDir 'hz12_retired_reclaim_shadow_smoke.exe')"
)
& clang-cl @retiredReclaimShadowSmokeArgs
if ($LASTEXITCODE -ne 0) { throw "clang-cl failed: $LASTEXITCODE" }

$ownerBatchLedgerSmokeArgs = @(
    "/nologo", "/O2", "/DNDEBUG", "/std:c11", "/W3", "/MD",
    "/I$(Join-Path $RepoRoot 'win')",
    "/I$(Join-Path $Hz12Root 'src')",
    "/I$(Join-Path $Hz12Root 'include')",
    "/DHZ12_CLASSIFY_SPAN=1",
    "/DHZ12_CACHE_CAP=256",
    $OwnerBatchLedgerSmoke, $OwnerBatchLedger, $OwnerBatchLedgerCompare,
    $SpanOwnerShadow, $Accounting
) + $Hz12Sources + @(
    "psapi.lib", "/link",
    "/out:$(Join-Path $OutDir 'hz12_owner_batch_ledger_smoke.exe')"
)
& clang-cl @ownerBatchLedgerSmokeArgs
if ($LASTEXITCODE -ne 0) { throw "clang-cl failed: $LASTEXITCODE" }

$ownerBatchLedgerBoundarySmokeArgs = @(
    "/nologo", "/O2", "/DNDEBUG", "/std:c11", "/W3", "/MD",
    "/I$(Join-Path $RepoRoot 'win')",
    "/I$(Join-Path $Hz12Root 'src')",
    "/I$(Join-Path $Hz12Root 'include')",
    "/DHZ12_CLASSIFY_SPAN=1",
    "/DHZ12_CACHE_CAP=256",
    "/DHZ12_FLUSH_OWNER_ROUTE=1",
    "/DHZ12_FLUSH_OWNER_COLD_SPAN=1",
    "/DHZ12_OWNER_BATCH_LEDGER_DIAG=1",
    $OwnerBatchLedgerBoundarySmoke, $OwnerBatchLedger,
    $OwnerBatchLedgerCompare, $SpanOwnerShadow,
    $Accounting, $Shadow,
    (Join-Path $Hz12Root "src\hz12_flush_owner_route.c")
) + $Hz12Sources + @(
    "psapi.lib", "/link",
    "/out:$(Join-Path $OutDir 'hz12_owner_batch_ledger_boundary_smoke.exe')"
)
& clang-cl @ownerBatchLedgerBoundarySmokeArgs
if ($LASTEXITCODE -ne 0) { throw "clang-cl failed: $LASTEXITCODE" }

$ownerBatchLedgerXownerSmokeArgs = @(
    "/nologo", "/O2", "/DNDEBUG", "/std:c11", "/W3", "/MD",
    "/I$(Join-Path $RepoRoot 'win')",
    "/I$(Join-Path $Hz12Root 'src')",
    "/I$(Join-Path $Hz12Root 'include')",
    "/DHZ12_CLASSIFY_SPAN=1",
    "/DHZ12_CACHE_CAP=256",
    "/DHZ12_FLUSH_OWNER_ROUTE=1",
    "/DHZ12_FLUSH_OWNER_COLD_SPAN=1",
    "/DHZ12_OWNER_BATCH_LEDGER_DIAG=1",
    "/DHZ12_FLUSH_OWNER_INBOX_CAP=2048",
    $OwnerBatchLedgerXownerSmoke, $OwnerBatchLedger,
    $OwnerBatchLedgerCompare, $SpanOwnerShadow,
    $Accounting, $Shadow, $OwnerLedgerRetireGate, $OwnerRetireGate,
    $OwnerEpoch, $TokenInbox, $OwnerRegistry,
    (Join-Path $Hz12Root "src\hz12_flush_owner_route.c")
) + $Hz12Sources + @(
    "psapi.lib", "/link",
    "/out:$(Join-Path $OutDir 'hz12_owner_batch_ledger_xowner_smoke.exe')"
)
& clang-cl @ownerBatchLedgerXownerSmokeArgs
if ($LASTEXITCODE -ne 0) { throw "clang-cl failed: $LASTEXITCODE" }

$wideWsOwnerLedgerShadowArgs = @(
    "/nologo", "/O2", "/DNDEBUG", "/std:c11", "/W3", "/MD",
    "/I$(Join-Path $RepoRoot 'win')",
    "/I$(Join-Path $Hz12Root 'src')",
    "/I$(Join-Path $Hz12Root 'include')",
    "/DHZ12_CLASSIFY_SPAN=1",
    "/DHZ12_CACHE_CAP=256",
    "/DHZ12_FLUSH_OWNER_ROUTE=1",
    "/DHZ12_FLUSH_OWNER_COLD_SPAN=1",
    "/DHZ12_OWNER_BATCH_LEDGER_DIAG=1",
    "/DHZ12_FLUSH_OWNER_INBOX_CAP=2048",
    $WideWsOwnerLedgerShadow, $OwnerBatchLedger,
    $OwnerBatchLedgerCompare, $SpanOwnerShadow,
    $Accounting, $Shadow, $OwnerLedgerRetireGate, $OwnerRetireGate,
    $OwnerEpoch, $TokenInbox, $OwnerRegistry,
    (Join-Path $Hz12Root "src\hz12_flush_owner_route.c")
) + $Hz12Sources + @(
    "psapi.lib", "/link",
    "/out:$(Join-Path $OutDir 'bench_hz12_widews_owner_ledger_shadow.exe')"
)
& clang-cl @wideWsOwnerLedgerShadowArgs
if ($LASTEXITCODE -ne 0) { throw "clang-cl failed: $LASTEXITCODE" }

$wideWsReclaimPolicyShadowArgs = @(
    "/nologo", "/O2", "/DNDEBUG", "/std:c11", "/W3", "/MD",
    "/I$(Join-Path $RepoRoot 'win')",
    "/I$(Join-Path $Hz12Root 'src')",
    "/I$(Join-Path $Hz12Root 'include')",
    "/DHZ12_CLASSIFY_SPAN=1",
    "/DHZ12_CACHE_CAP=256",
    "/DHZ12_FLUSH_OWNER_ROUTE=1",
    "/DHZ12_FLUSH_OWNER_COLD_SPAN=1",
    "/DHZ12_OWNER_BATCH_LEDGER_DIAG=1",
    "/DHZ12_RECLAIM_POLICY_SHADOW=1",
    "/DHZ12_FLUSH_OWNER_INBOX_CAP=2048",
    $WideWsOwnerLedgerShadow, $OwnerBatchLedger,
    $OwnerBatchLedgerCompare, $SpanOwnerShadow,
    $Accounting, $Shadow, $OwnerLedgerRetireGate, $OwnerRetireGate,
    $OwnerEpoch, $TokenInbox, $OwnerRegistry, $ReclaimPolicyShadow,
    $SpanDepotCore,
    (Join-Path $Hz12Root "src\hz12_flush_owner_route.c")
) + $Hz12Sources + @(
    "psapi.lib", "/link",
    "/out:$(Join-Path $OutDir 'bench_hz12_widews_reclaim_policy_shadow.exe')"
)
& clang-cl @wideWsReclaimPolicyShadowArgs
if ($LASTEXITCODE -ne 0) { throw "clang-cl failed: $LASTEXITCODE" }

$wideWsReclaimPolicyBelowArgs = @(
    "/nologo", "/O2", "/DNDEBUG", "/std:c11", "/W3", "/MD",
    "/I$(Join-Path $RepoRoot 'win')",
    "/I$(Join-Path $Hz12Root 'src')",
    "/I$(Join-Path $Hz12Root 'include')",
    "/DHZ12_CLASSIFY_SPAN=1",
    "/DHZ12_CACHE_CAP=256",
    "/DHZ12_FLUSH_OWNER_ROUTE=1",
    "/DHZ12_FLUSH_OWNER_COLD_SPAN=1",
    "/DHZ12_OWNER_BATCH_LEDGER_DIAG=1",
    "/DHZ12_RECLAIM_POLICY_SHADOW=1",
    "/DH12_LEDGER_WIDE_SPANS=63",
    "/DHZ12_FLUSH_OWNER_INBOX_CAP=2048",
    $WideWsOwnerLedgerShadow, $OwnerBatchLedger,
    $OwnerBatchLedgerCompare, $SpanOwnerShadow,
    $Accounting, $Shadow, $OwnerLedgerRetireGate, $OwnerRetireGate,
    $OwnerEpoch, $TokenInbox, $OwnerRegistry, $ReclaimPolicyShadow,
    $SpanDepotCore,
    (Join-Path $Hz12Root "src\hz12_flush_owner_route.c")
) + $Hz12Sources + @(
    "psapi.lib", "/link",
    "/out:$(Join-Path $OutDir 'bench_hz12_widews_reclaim_policy_below.exe')"
)
& clang-cl @wideWsReclaimPolicyBelowArgs
if ($LASTEXITCODE -ne 0) { throw "clang-cl failed: $LASTEXITCODE" }

$wideWsSnapshotReclaimArgs = @(
    "/nologo", "/O2", "/DNDEBUG", "/std:c11", "/W3", "/MD",
    "/I$(Join-Path $RepoRoot 'win')",
    "/I$(Join-Path $Hz12Root 'src')",
    "/I$(Join-Path $Hz12Root 'include')",
    "/DHZ12_CLASSIFY_SPAN=1",
    "/DHZ12_CACHE_CAP=256",
    "/DHZ12_FLUSH_OWNER_ROUTE=1",
    "/DHZ12_FLUSH_OWNER_COLD_SPAN=1",
    "/DHZ12_OWNER_BATCH_LEDGER_DIAG=1",
    "/DHZ12_SNAPSHOT_RECLAIM_BEHAVIOR=1",
    "/DHZ12_FLUSH_OWNER_INBOX_CAP=2048",
    $WideWsOwnerLedgerShadow, $OwnerBatchLedger,
    $OwnerBatchLedgerCompare, $SpanOwnerShadow,
    $Accounting, $Shadow, $OwnerLedgerRetireGate, $OwnerRetireGate,
    $OwnerEpoch, $TokenInbox, $OwnerRegistry, $SnapshotReclaim, $SpanDepotCore,
    $ReclaimEntry,
    (Join-Path $Hz12Root "src\hz12_flush_owner_route.c")
) + $Hz12Sources + @(
    "psapi.lib", "/link",
    "/out:$(Join-Path $OutDir 'bench_hz12_widews_snapshot_reclaim.exe')"
)
& clang-cl @wideWsSnapshotReclaimArgs
if ($LASTEXITCODE -ne 0) { throw "clang-cl failed: $LASTEXITCODE" }

$wideWsSnapshotReclaimNoCapArgs = @(
    "/nologo", "/O2", "/DNDEBUG", "/std:c11", "/W3", "/MD",
    "/I$(Join-Path $RepoRoot 'win')",
    "/I$(Join-Path $Hz12Root 'src')",
    "/I$(Join-Path $Hz12Root 'include')",
    "/DHZ12_CLASSIFY_SPAN=1",
    "/DHZ12_CACHE_CAP=256",
    "/DHZ12_FLUSH_OWNER_ROUTE=1",
    "/DHZ12_FLUSH_OWNER_COLD_SPAN=1",
    "/DHZ12_OWNER_BATCH_LEDGER_DIAG=1",
    "/DHZ12_SNAPSHOT_RECLAIM_BEHAVIOR=1",
    "/DHZ12_SNAPSHOT_RECLAIM_NO_CAP=1",
    "/DHZ12_FLUSH_OWNER_INBOX_CAP=2048",
    $WideWsOwnerLedgerShadow, $OwnerBatchLedger,
    $OwnerBatchLedgerCompare, $SpanOwnerShadow,
    $Accounting, $Shadow, $OwnerLedgerRetireGate, $OwnerRetireGate,
    $OwnerEpoch, $TokenInbox, $OwnerRegistry, $SnapshotReclaim, $SpanDepotCore,
    $ReclaimEntry,
    (Join-Path $Hz12Root "src\hz12_flush_owner_route.c")
) + $Hz12Sources + @(
    "psapi.lib", "/link",
    "/out:$(Join-Path $OutDir 'bench_hz12_widews_snapshot_reclaim_nocap.exe')"
)
& clang-cl @wideWsSnapshotReclaimNoCapArgs
if ($LASTEXITCODE -ne 0) { throw "clang-cl failed: $LASTEXITCODE" }

$wideWsReclaimPolicyBehaviorArgs = @(
    "/nologo", "/O2", "/DNDEBUG", "/std:c11", "/W3", "/MD",
    "/I$(Join-Path $RepoRoot 'win')",
    "/I$(Join-Path $Hz12Root 'src')",
    "/I$(Join-Path $Hz12Root 'include')",
    "/DHZ12_CLASSIFY_SPAN=1",
    "/DHZ12_CACHE_CAP=256",
    "/DHZ12_FLUSH_OWNER_ROUTE=1",
    "/DHZ12_FLUSH_OWNER_COLD_SPAN=1",
    "/DHZ12_OWNER_BATCH_LEDGER_DIAG=1",
    "/DHZ12_RECLAIM_POLICY_SHADOW=1",
    "/DHZ12_SNAPSHOT_RECLAIM_BEHAVIOR=1",
    "/DHZ12_RECLAIM_POLICY_BEHAVIOR=1",
    "/DHZ12_FLUSH_OWNER_INBOX_CAP=2048",
    $WideWsOwnerLedgerShadow, $OwnerBatchLedger,
    $OwnerBatchLedgerCompare, $SpanOwnerShadow,
    $Accounting, $Shadow, $OwnerLedgerRetireGate, $OwnerRetireGate,
    $OwnerEpoch, $TokenInbox, $OwnerRegistry, $ReclaimPolicyShadow,
    $SnapshotReclaim, $SpanDepotCore, $ReclaimEntry,
    (Join-Path $Hz12Root "src\hz12_flush_owner_route.c")
) + $Hz12Sources + @(
    "psapi.lib", "/link",
    "/out:$(Join-Path $OutDir 'bench_hz12_widews_reclaim_policy_behavior.exe')"
)
& clang-cl @wideWsReclaimPolicyBehaviorArgs
if ($LASTEXITCODE -ne 0) { throw "clang-cl failed: $LASTEXITCODE" }

$wideWsSnapshotRecycleArgs = @(
    "/nologo", "/O2", "/DNDEBUG", "/std:c11", "/W3", "/MD",
    "/I$(Join-Path $RepoRoot 'win')",
    "/I$(Join-Path $Hz12Root 'src')",
    "/I$(Join-Path $Hz12Root 'include')",
    "/DHZ12_CLASSIFY_SPAN=1",
    "/DHZ12_CACHE_CAP=256",
    "/DHZ12_FLUSH_OWNER_ROUTE=1",
    "/DHZ12_FLUSH_OWNER_COLD_SPAN=1",
    "/DHZ12_OWNER_BATCH_LEDGER_DIAG=1",
    "/DHZ12_SNAPSHOT_RECLAIM_BEHAVIOR=1",
    "/DHZ12_SNAPSHOT_RECYCLE_BEHAVIOR=1",
    "/DHZ12_FLUSH_OWNER_INBOX_CAP=2048",
    $WideWsOwnerLedgerShadow, $OwnerBatchLedger,
    $OwnerBatchLedgerCompare, $SpanOwnerShadow,
    $Accounting, $Shadow, $OwnerLedgerRetireGate, $OwnerRetireGate,
    $OwnerEpoch, $TokenInbox, $OwnerRegistry, $SnapshotReclaim,
    $SnapshotRecycle, $SpanDepotCore, $ReclaimEntry,
    (Join-Path $Hz12Root "src\hz12_flush_owner_route.c")
) + $Hz12Sources + @(
    "psapi.lib", "/link",
    "/out:$(Join-Path $OutDir 'bench_hz12_widews_snapshot_recycle.exe')"
)
& clang-cl @wideWsSnapshotRecycleArgs
if ($LASTEXITCODE -ne 0) { throw "clang-cl failed: $LASTEXITCODE" }

$wideWsSnapshotRecycleRollbackArgs = @(
    "/nologo", "/O2", "/DNDEBUG", "/std:c11", "/W3", "/MD",
    "/I$(Join-Path $RepoRoot 'win')",
    "/I$(Join-Path $Hz12Root 'src')",
    "/I$(Join-Path $Hz12Root 'include')",
    "/DHZ12_CLASSIFY_SPAN=1",
    "/DHZ12_CACHE_CAP=256",
    "/DHZ12_FLUSH_OWNER_ROUTE=1",
    "/DHZ12_FLUSH_OWNER_COLD_SPAN=1",
    "/DHZ12_OWNER_BATCH_LEDGER_DIAG=1",
    "/DHZ12_SNAPSHOT_RECLAIM_BEHAVIOR=1",
    "/DHZ12_SNAPSHOT_RECYCLE_BEHAVIOR=1",
    "/DHZ12_SNAPSHOT_RECYCLE_ROLLBACK=1",
    "/DHZ12_FLUSH_OWNER_INBOX_CAP=2048",
    $WideWsOwnerLedgerShadow, $OwnerBatchLedger,
    $OwnerBatchLedgerCompare, $SpanOwnerShadow,
    $Accounting, $Shadow, $OwnerLedgerRetireGate, $OwnerRetireGate,
    $OwnerEpoch, $TokenInbox, $OwnerRegistry, $SnapshotReclaim,
    $SnapshotRecycle, $SpanDepotCore, $ReclaimEntry,
    (Join-Path $Hz12Root "src\hz12_flush_owner_route.c")
) + $Hz12Sources + @(
    "psapi.lib", "/link",
    "/out:$(Join-Path $OutDir 'bench_hz12_widews_snapshot_recycle_rollback.exe')"
)
& clang-cl @wideWsSnapshotRecycleRollbackArgs
if ($LASTEXITCODE -ne 0) { throw "clang-cl failed: $LASTEXITCODE" }

$wideWsOwnerLedgerReclaimArgs = @(
    "/nologo", "/O2", "/DNDEBUG", "/std:c11", "/W3", "/MD",
    "/I$(Join-Path $RepoRoot 'win')",
    "/I$(Join-Path $Hz12Root 'src')",
    "/I$(Join-Path $Hz12Root 'include')",
    "/DHZ12_CLASSIFY_SPAN=1",
    "/DHZ12_CACHE_CAP=256",
    "/DHZ12_FLUSH_OWNER_ROUTE=1",
    "/DHZ12_FLUSH_OWNER_COLD_SPAN=1",
    "/DHZ12_OWNER_BATCH_LEDGER_DIAG=1",
    "/DHZ12_OWNER_LEDGER_RECLAIM_BEHAVIOR=1",
    "/DHZ12_OWNER_BATCH_LEDGER_RECYCLE_DIAG=1",
    "/DHZ12_FLUSH_OWNER_INBOX_CAP=2048",
    $WideWsOwnerLedgerShadow, $OwnerBatchLedger,
    $OwnerBatchLedgerCompare, $SpanOwnerShadow,
    $Accounting, $Shadow, $OwnerLedgerRetireGate, $OwnerRetireGate,
    $OwnerEpoch, $TokenInbox, $OwnerRegistry, $RetiredReclaimDetach,
    $RetiredReclaimDecommit, $SpanDetach, $SpanDecommit, $ReclaimGate,
    $RetiredReclaimRecycle, $SpanDepot, $SpanDepotCore, $ReclaimCarveDiag,
    (Join-Path $Hz12Root "src\hz12_flush_owner_route.c")
) + $Hz12Sources + @(
    "psapi.lib", "/link",
    "/out:$(Join-Path $OutDir 'bench_hz12_widews_owner_ledger_reclaim.exe')"
)
& clang-cl @wideWsOwnerLedgerReclaimArgs
if ($LASTEXITCODE -ne 0) { throw "clang-cl failed: $LASTEXITCODE" }

$wideWsReclaimShadowArgs = @(
    "/nologo", "/O2", "/DNDEBUG", "/std:c11", "/W3", "/MD",
    "/I$(Join-Path $RepoRoot 'win')",
    "/I$(Join-Path $Hz12Root 'src')",
    "/I$(Join-Path $Hz12Root 'include')",
    "/DHZ12_CLASSIFY_SPAN=1",
    "/DHZ12_CACHE_CAP=256",
    $WideWsReclaimShadow, $RetiredReclaimShadow, $RetiredReclaimDetach,
    $RetiredReclaimDecommit, $SpanOwnerShadow, $SpanDetach, $SpanDecommit,
    $RetiredReclaimDepotCycle, $SpanDepot, $SpanDepotCore,
    $ReclaimCarveDiag, $RetiredReclaimRecycle,
    $OwnerRetireGate, $OwnerEpoch, $TokenInbox, $OwnerRegistry, $Accounting,
    $ReclaimGate
) + $Hz12Sources + @(
    "psapi.lib", "/link",
    "/out:$(Join-Path $OutDir 'bench_hz12_widews_reclaim_shadow.exe')"
)
& clang-cl @wideWsReclaimShadowArgs
if ($LASTEXITCODE -ne 0) { throw "clang-cl failed: $LASTEXITCODE" }
Write-Host "Built HZ12 L0 and L1 artifacts in: $OutDir"
