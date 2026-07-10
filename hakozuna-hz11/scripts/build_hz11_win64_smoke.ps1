$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$outTokenExe = Join-Path $root "hz11_thread_cache_smoke_win.exe"
$outSpanExe = Join-Path $root "hz11_thread_cache_smoke_span_win.exe"
$outFine128TransferExe = Join-Path $root "hz11_thread_cache_smoke_fine128_transfer_win.exe"

$clang = (Get-Command clang-cl -ErrorAction Stop).Source

$tokenSources = @(
  (Join-Path $root "src\hz11_size_class.c"),
  (Join-Path $root "src\hz11_sys_alloc.c"),
  (Join-Path $root "src\hz11_thread_cache.c"),
  (Join-Path $root "src\hz11_public_entry.c"),
  (Join-Path $root "tests\hz11_thread_cache_smoke.c")
)

$spanSources = @(
  (Join-Path $root "src\hz11_size_class.c"),
  (Join-Path $root "src\hz11_sys_alloc.c"),
  (Join-Path $root "src\hz11_thread_cache.c"),
  (Join-Path $root "src\hz11_public_entry.c"),
  (Join-Path $root "src\hz11_span.c"),
  (Join-Path $root "src\hz11_live_footprint.c"),
  (Join-Path $root "tests\hz11_thread_cache_smoke.c")
)
$transferSources = $spanSources[0..5] + @(
  (Join-Path $root "src\hz11_transfer_cache.c"),
  (Join-Path $root "src\hz11_alloc_diag.c"),
  $spanSources[6]
)

function Invoke-Clang {
  param([Parameter(Mandatory = $true)][string[]]$Args)
  & $clang @Args
  if ($LASTEXITCODE -ne 0) {
    throw "clang-cl failed with exit code $LASTEXITCODE"
  }
}

$tokenArgs = @(
  "/nologo", "/W4", "/WX", "/O2", "/D_CRT_SECURE_NO_WARNINGS",
  "/I", (Join-Path $root "include"),
  "/I", (Join-Path $root "src"),
  "/Fe$outTokenExe"
)
Invoke-Clang ($tokenArgs + $tokenSources)

Write-Host "built $outTokenExe"

& $outTokenExe
if ($LASTEXITCODE -ne 0) {
  throw "hz11_thread_cache_smoke_win failed with exit code $LASTEXITCODE"
}

$spanArgs = @(
  "/nologo", "/W4", "/WX", "/O2", "/D_CRT_SECURE_NO_WARNINGS",
  "/DHZ11_CLASSIFY_SPAN=1",
  "/I", (Join-Path $root "include"),
  "/I", (Join-Path $root "src"),
  "/Fe$outSpanExe"
)
Invoke-Clang ($spanArgs + $spanSources)

Write-Host "built $outSpanExe"

& $outSpanExe
if ($LASTEXITCODE -ne 0) {
  throw "hz11_thread_cache_smoke_span_win failed with exit code $LASTEXITCODE"
}

$fine128TransferArgs = @(
  "/nologo", "/W4", "/WX", "/O2", "/D_CRT_SECURE_NO_WARNINGS",
  "/DHZ11_CLASSIFY_SPAN=1",
  "/DHZ11_TLS_FASTPATH=1",
  "/DHZ11_CACHE_BYTE_ACCOUNTING=0",
  "/DHZ11_CACHE_SOA=1",
  "/DHZ11_TRANSFER_STATS=0",
  "/DHZ11_TRANSFER_CENTRAL_SPAN=1",
  "/DHZ11_CURRENT_SPAN_THREAD_EXIT=1",
  "/DHZ11_CENTRAL_CAP=65536",
  "/DHZ11_TRANSFER_BATCH=32",
  "/DHZ11_FINE_SIZE_CLASSES=1",
  "/DHZ11_FINE_LINEAR_MAX=128u",
  "/I", (Join-Path $root "include"),
  "/I", (Join-Path $root "src"),
  "/Fe$outFine128TransferExe"
)
Invoke-Clang ($fine128TransferArgs + $transferSources)

Write-Host "built $outFine128TransferExe"

& $outFine128TransferExe
if ($LASTEXITCODE -ne 0) {
  throw "hz11_thread_cache_smoke_fine128_transfer_win failed with exit code $LASTEXITCODE"
}
