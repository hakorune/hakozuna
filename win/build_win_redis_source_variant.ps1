param(
    [string]$RedisRoot,
    [string]$Config = "Release",
    [string]$Platform = "x64",
    [string]$Toolset = "v143",
    [string]$DriveLetter = "R",
    [string]$MSBuildPath = "",
    [ValidateSet("Jemalloc", "Tcmalloc", "Mimalloc", "Hz3", "Hz4")]
    [string]$Allocator = "Jemalloc",
    [string]$VcpkgRoot = "",
    [string]$Hz3Root = "",
    [string]$Hz4Root = "",
    [string[]]$Hz4ExtraDefines
)

$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot
if (-not $RedisRoot) {
    foreach ($candidate in @(
        (Join-Path $RepoRoot "private\bench-assets\windows\redis\source"),
        (Join-Path $RepoRoot "private\hakmem\hakozuna\bench\windows\redis-src")
    )) {
        if (Test-Path $candidate) {
            $RedisRoot = $candidate
            break
        }
    }
}

if (-not $RedisRoot) {
    throw "redis source tree not found. Prepare private assets under private\\bench-assets\\windows\\redis\\source or keep the legacy private\\hakmem tree."
}
if (-not (Test-Path $RedisRoot)) {
    throw "redis source tree not found: $RedisRoot"
}

$msvsDir = Join-Path $RedisRoot "msvs"
$slnPath = Join-Path $msvsDir "RedisServer.sln"
if (-not (Test-Path $slnPath)) {
    throw "RedisServer.sln not found: $slnPath"
}

$releaseHeader = Join-Path $RedisRoot "src\release.h"
if (-not (Test-Path $releaseHeader)) {
    $stamp = [int][double]::Parse((Get-Date -UFormat %s))
    @(
        '#define REDIS_GIT_SHA1 "00000000"'
        '#define REDIS_GIT_DIRTY "0"'
        ('#define REDIS_BUILD_ID "win-{0}"' -f $stamp)
    ) | Set-Content -Encoding ASCII $releaseHeader
}

function Copy-IfDifferentPath {
    param(
        [string]$Src,
        [string]$Dst
    )

    if (-not (Test-Path $Src)) {
        return
    }
    if (Test-Path $Dst) {
        $srcResolved = (Resolve-Path $Src).Path
        $dstResolved = (Resolve-Path $Dst).Path
        if ($srcResolved -eq $dstResolved) {
            return
        }
    }
    Copy-Item -Force $Src $Dst
}

$casePairs = @(
    @{ Src = "src\Win32_Interop\win32_types.h"; Dst = "src\Win32_Interop\Win32_types.h" },
    @{ Src = "src\Win32_Interop\win32_types_hiredis.h"; Dst = "src\Win32_Interop\Win32_types_hiredis.h" },
    @{ Src = "src\Win32_Interop\win32fixes.h"; Dst = "src\Win32_Interop\Win32Fixes.h" }
)
foreach ($pair in $casePairs) {
    $srcPath = Join-Path $RedisRoot $pair.Src
    $dstPath = Join-Path $RedisRoot $pair.Dst
    Copy-IfDifferentPath -Src $srcPath -Dst $dstPath
}

$vcxprojFiles = Get-ChildItem -Path $RedisRoot -Recurse -Filter *.vcxproj
foreach ($file in $vcxprojFiles) {
    $text = Get-Content -Raw $file.FullName
    $updated = $text `
        -replace "MultiThreadedDebug</RuntimeLibrary>", "MultiThreadedDebugDLL</RuntimeLibrary>" `
        -replace "MultiThreaded</RuntimeLibrary>", "MultiThreadedDLL</RuntimeLibrary>"
    if ($Toolset -ne "") {
        $updated = $updated -replace "<PlatformToolset>v\d+</PlatformToolset>", "<PlatformToolset>$Toolset</PlatformToolset>"
    }
    if ($updated -ne $text) {
        Set-Content -NoNewline -Encoding UTF8 $file.FullName $updated
    }
}

if (-not $VcpkgRoot) {
    $VcpkgRoot = $env:VCPKG_ROOT
}
if (-not $VcpkgRoot) {
    $VcpkgRoot = "C:\vcpkg"
}

if (-not $Hz3Root) {
    $Hz3Root = Join-Path $RepoRoot "hakozuna"
}
if (-not $Hz4Root) {
    $Hz4Root = Join-Path $RepoRoot "hakozuna-mt"
}

$vcpkgInclude = Join-Path $VcpkgRoot "installed\x64-windows\include"
$vcpkgLib = Join-Path $VcpkgRoot "installed\x64-windows\lib"
$vcpkgBin = Join-Path $VcpkgRoot "installed\x64-windows\bin"
$hz3Include = Join-Path $Hz3Root "include"
$hz3WinInclude = Join-Path $Hz3Root "win\include"
$hz3Lib = Join-Path $Hz3Root "out_win\hz3_win.lib"
$hz4CoreInclude = Join-Path $Hz4Root "core"
$hz4Include = Join-Path $Hz4Root "include"
$hz4WinInclude = Join-Path $Hz4Root "win"
$hz4Lib = Join-Path $Hz4Root "out_win_bench\hz4_win.lib"
$hz4WinApiSource = Join-Path $Hz4Root "win\hz4_win_api.c"
$hz4WinApiObject = Join-Path $Hz4Root "out_win_bench\hz4_win_api_redis.obj"

function Split-SemicolonList {
    param([string]$Value)

    if ([string]::IsNullOrWhiteSpace($Value)) {
        return @()
    }
    return $Value.Split(";") | ForEach-Object { $_.Trim() } | Where-Object { $_ -ne "" }
}

function Join-SemicolonList {
    param([string[]]$Items)

    return (($Items | Where-Object { $_ -and $_.Trim() -ne "" }) | Select-Object -Unique) -join ";"
}

function Get-DefineName {
    param([string]$Define)

    if ([string]::IsNullOrWhiteSpace($Define)) {
        return ""
    }
    $eq = $Define.IndexOf("=")
    if ($eq -lt 0) {
        return $Define.Trim()
    }
    return $Define.Substring(0, $eq).Trim()
}

function Merge-Defines {
    param(
        [string[]]$BaseDefines,
        [string[]]$OverrideDefines
    )

    $overrideMap = @{}
    foreach ($define in @($OverrideDefines)) {
        if ([string]::IsNullOrWhiteSpace($define)) {
            continue
        }
        $overrideMap[(Get-DefineName -Define $define)] = $define.Trim()
    }

    $merged = New-Object System.Collections.Generic.List[string]
    foreach ($define in @($BaseDefines)) {
        if ([string]::IsNullOrWhiteSpace($define)) {
            continue
        }
        $name = Get-DefineName -Define $define
        if ($overrideMap.ContainsKey($name)) {
            continue
        }
        $merged.Add($define.Trim())
    }

    foreach ($define in $overrideMap.Values) {
        $merged.Add($define)
    }

    return $merged.ToArray()
}

function Ensure-XmlChild {
    param(
        [xml]$Document,
        [System.Xml.XmlNode]$Parent,
        [string]$Name,
        [string]$NamespaceUri,
        [System.Xml.XmlNamespaceManager]$NamespaceManager
    )

    $node = $Parent.SelectSingleNode(("msb:{0}" -f $Name), $NamespaceManager)
    if (-not $node) {
        $node = $Document.CreateElement($Name, $NamespaceUri)
        [void]$Parent.AppendChild($node)
    }
    return $node
}

function Sync-ProjectCompileInclude {
    param(
        [xml]$Document,
        [System.Xml.XmlNamespaceManager]$NamespaceManager,
        [string]$IncludePath,
        [bool]$Enabled
    )

    $existing = $Document.SelectNodes("//msb:ClCompile[@Include=`"$IncludePath`"]", $NamespaceManager)
    if ($Enabled) {
        if ($existing.Count -eq 0) {
            $itemGroup = $Document.CreateElement("ItemGroup", $Document.Project.NamespaceURI)
            $node = $Document.CreateElement("ClCompile", $Document.Project.NamespaceURI)
            [void]$node.SetAttribute("Include", $IncludePath)
            [void]$itemGroup.AppendChild($node)
            [void]$Document.Project.AppendChild($itemGroup)
        }
        return
    }

    foreach ($node in @($existing)) {
        $parent = $node.ParentNode
        [void]$parent.RemoveChild($node)
        if (-not $parent.HasChildNodes) {
            [void]$parent.ParentNode.RemoveChild($parent)
        }
    }
}

function Build-Hz4HelperObject {
    param([string[]]$ExtraDefines)

    $clang = Get-Command "clang-cl" -ErrorAction SilentlyContinue
    if (-not $clang) {
        throw "clang-cl not found in PATH; required for hz4 Redis helper object"
    }

    $outDir = Split-Path -Parent $hz4WinApiObject
    if (-not (Test-Path $outDir)) {
        New-Item -ItemType Directory -Force -Path $outDir | Out-Null
    }

    $hz4BaseDefines = @(
        "HZ4_TLS_DIRECT=0",
        "HZ4_PAGE_META_SEPARATE=0",
        "HZ4_RSSRETURN=0",
        "HZ4_MID_PAGE_SUPPLY_RESV_BOX=0",
        "HZ4_FREE_ROUTE_SEGMENT_REGISTRY_BOX=1",
        "HZ4_WIN_ONE_SHOT_OWNERSHIP_BOX=1"
    )
    $mergedDefines = Merge-Defines -BaseDefines $hz4BaseDefines -OverrideDefines $ExtraDefines

    $args = @(
        "/c",
        "/nologo",
        "/O2",
        "/MD",
        "/TC",
        ("/I" + $hz4CoreInclude),
        ("/I" + $hz4Include),
        ("/I" + $hz4WinInclude),
        ("/Fo" + $hz4WinApiObject),
        $hz4WinApiSource
    )
    $args = @($args[0..7]) + ($mergedDefines | ForEach-Object { "/D$_" }) + @($args[8], $args[9])
    & $clang.Source @args
    if ($LASTEXITCODE -ne 0) {
        throw "clang-cl failed to build hz4 helper object: $hz4WinApiSource"
    }
}

function Update-RedisAllocator {
    param([string]$VcxprojPath)

    $alloc = $Allocator.ToLowerInvariant()
    $allocDefine = switch ($alloc) {
        "tcmalloc" { "USE_TCMALLOC" }
        "mimalloc" { "USE_MIMALLOC" }
        "hz3" { "USE_HZ3" }
        "hz4" { "USE_HZ4" }
        default { "USE_JEMALLOC" }
    }
    $allocIncludes = @()
    $allocDependencies = @()
    $allocExtraDefines = @()
    $managedDefineNames = @()

    if ($alloc -eq "tcmalloc") {
        $tcLib = Join-Path $vcpkgLib "tcmalloc_minimal.lib"
        if (-not (Test-Path $tcLib)) {
            throw "tcmalloc_minimal.lib not found. Run: vcpkg install gperftools:x64-windows"
        }
        $allocIncludes = @($vcpkgInclude)
        $allocDependencies = @($tcLib)
    }
    if ($alloc -eq "mimalloc") {
        $miLib = Join-Path $vcpkgLib "mimalloc.dll.lib"
        if (-not (Test-Path $miLib)) {
            throw "mimalloc.dll.lib not found. Run: vcpkg install mimalloc:x64-windows"
        }
        $allocIncludes = @($vcpkgInclude)
        $allocDependencies = @($miLib)
    }
    if ($alloc -eq "hz3") {
        if (-not (Test-Path $hz3Lib)) {
            & (Join-Path $Hz3Root "win\build_win_min.ps1") | Out-Null
        }
        if (-not (Test-Path $hz3Lib)) {
            throw "hz3 library not found after build: $hz3Lib"
        }
        $allocIncludes = @($hz3Include, $hz3WinInclude)
        $allocDependencies = @($hz3Lib)
    }
    if ($alloc -eq "hz4") {
        if ($Hz4ExtraDefines -or -not (Test-Path $hz4Lib)) {
            $hz4BuildParams = @{}
            if ($VcpkgRoot) {
                $hz4BuildParams["VcpkgRoot"] = $VcpkgRoot
            }
            if ($Hz4ExtraDefines) {
                $hz4BuildParams["ExtraDefines"] = $Hz4ExtraDefines
            }
            & (Join-Path $Hz4Root "win\build_win_bench_compare.ps1") @hz4BuildParams | Out-Null
        }
        if (-not (Test-Path $hz4Lib)) {
            throw "hz4 library not found after build: $hz4Lib"
        }
        if (-not (Test-Path $hz4WinApiSource)) {
            throw "hz4 helper source not found: $hz4WinApiSource"
        }
        Build-Hz4HelperObject -ExtraDefines $Hz4ExtraDefines
        $allocIncludes = @($hz4CoreInclude, $hz4Include, $hz4WinInclude)
        $allocDependencies = @($hz4WinApiObject, $hz4Lib)
        $hz4BaseDefines = @(
            "HZ4_TLS_DIRECT=0",
            "HZ4_PAGE_META_SEPARATE=0",
            "HZ4_RSSRETURN=0",
            "HZ4_MID_PAGE_SUPPLY_RESV_BOX=0",
            "HZ4_FREE_ROUTE_SEGMENT_REGISTRY_BOX=1",
            "HZ4_WIN_ONE_SHOT_OWNERSHIP_BOX=1"
        )
        $allocExtraDefines = Merge-Defines -BaseDefines $hz4BaseDefines -OverrideDefines $Hz4ExtraDefines
        $managedDefineNames = $hz4BaseDefines | ForEach-Object { Get-DefineName -Define $_ }
    }

    $text = Get-Content -Raw $VcxprojPath
    try {
        [xml]$xml = $text
    } catch {
        $repaired = $text -replace '<AdditionalLibraryDirectories>[^<]*%\(AdditionalDependencies\)</AdditionalDependencies>', '<AdditionalLibraryDirectories>$(OutDir);$(OutDir)lib</AdditionalLibraryDirectories><AdditionalDependencies>psapi.lib;kernel32.lib;user32.lib;gdi32.lib;winspool.lib;advapi32.lib;shell32.lib;%(AdditionalDependencies)</AdditionalDependencies>'
        [xml]$xml = $repaired
    }
    $nsUri = $xml.Project.NamespaceURI
    $nsMgr = New-Object System.Xml.XmlNamespaceManager($xml.NameTable)
    $nsMgr.AddNamespace("msb", $nsUri)

    $baseLibDirs = @('$(OutDir)', '$(OutDir)lib')
    $baseDependencies = @(
        "psapi.lib",
        "kernel32.lib",
        "user32.lib",
        "gdi32.lib",
        "winspool.lib",
        "advapi32.lib",
        "shell32.lib",
        "%(AdditionalDependencies)"
    )

    $itemGroups = $xml.SelectNodes("//msb:ItemDefinitionGroup", $nsMgr)
    foreach ($group in $itemGroups) {
        $clNode = Ensure-XmlChild -Document $xml -Parent $group -Name "ClCompile" -NamespaceUri $nsUri -NamespaceManager $nsMgr
        $defsNode = Ensure-XmlChild -Document $xml -Parent $clNode -Name "PreprocessorDefinitions" -NamespaceUri $nsUri -NamespaceManager $nsMgr
        $defs = Split-SemicolonList -Value $defsNode.InnerText
        $defs = @($allocDefine) + @($allocExtraDefines) + ($defs | Where-Object {
            $name = Get-DefineName -Define $_
            ($_ -notmatch '^USE_(JEMALLOC|TCMALLOC|MIMALLOC|HZ3|HZ4)$') -and
            ($name -notin $managedDefineNames)
        })
        $defsNode.InnerText = Join-SemicolonList -Items $defs

        $includeNode = Ensure-XmlChild -Document $xml -Parent $clNode -Name "AdditionalIncludeDirectories" -NamespaceUri $nsUri -NamespaceManager $nsMgr
        $includes = Split-SemicolonList -Value $includeNode.InnerText
        $includes = $includes | Where-Object {
            ($_ -notin @(
                $vcpkgInclude,
                $hz3Include,
                $hz3WinInclude,
                $hz4CoreInclude,
                $hz4Include,
                $hz4WinInclude
            ))
        }
        $includes = @($allocIncludes) + $includes
        $includeNode.InnerText = Join-SemicolonList -Items $includes

        $langNode = Ensure-XmlChild -Document $xml -Parent $clNode -Name "LanguageStandard_C" -NamespaceUri $nsUri -NamespaceManager $nsMgr
        if ($alloc -eq "hz4") {
            $langNode.InnerText = "stdc17"
        } elseif ($langNode.InnerText -eq "stdc17") {
            $langNode.InnerText = ""
        }

        $linkNode = Ensure-XmlChild -Document $xml -Parent $group -Name "Link" -NamespaceUri $nsUri -NamespaceManager $nsMgr
        $libDirsNode = Ensure-XmlChild -Document $xml -Parent $linkNode -Name "AdditionalLibraryDirectories" -NamespaceUri $nsUri -NamespaceManager $nsMgr
        $libDirs = Split-SemicolonList -Value $libDirsNode.InnerText
        $libDirs = $libDirs | Where-Object {
            ($_ -notmatch '\.lib$') -and
            ($_ -notin $baseDependencies)
        }
        $libDirsNode.InnerText = Join-SemicolonList -Items (@($baseLibDirs) + $libDirs)

        $depsNode = Ensure-XmlChild -Document $xml -Parent $linkNode -Name "AdditionalDependencies" -NamespaceUri $nsUri -NamespaceManager $nsMgr
        $deps = Split-SemicolonList -Value $depsNode.InnerText
        $deps = $deps | Where-Object {
            ($_ -notmatch 'tcmalloc_minimal\.lib$') -and
            ($_ -notmatch 'mimalloc\.dll\.lib$') -and
            ($_ -notmatch 'hz3_win\.lib$') -and
            ($_ -notmatch 'hz4_win\.lib$') -and
            ($_ -notmatch 'hz4_win_api_redis\.obj$') -and
            ($_ -notin $baseDependencies)
        }
        $depsNode.InnerText = Join-SemicolonList -Items (@($allocDependencies) + @($baseDependencies) + $deps)
    }

    Sync-ProjectCompileInclude -Document $xml -NamespaceManager $nsMgr -IncludePath $hz4WinApiSource -Enabled $false
    $xml.Save($VcxprojPath)
}

$redisServerProj = Join-Path $msvsDir "RedisServer.vcxproj"
if (-not (Test-Path $redisServerProj)) {
    throw "RedisServer.vcxproj not found: $redisServerProj"
}
Update-RedisAllocator -VcxprojPath $redisServerProj

if ($MSBuildPath -eq "") {
    $cmd = Get-Command msbuild -ErrorAction SilentlyContinue
    if ($cmd) {
        $MSBuildPath = $cmd.Path
    } else {
        $fallback = "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"
        if (Test-Path $fallback) {
            $MSBuildPath = $fallback
        } else {
            throw "MSBuild.exe not found. Pass -MSBuildPath."
        }
    }
}

$driveName = $DriveLetter.TrimEnd(":")
$driveRoot = "$driveName`:\"
$needUnmap = $false
$existing = Get-PSDrive -Name $driveName -ErrorAction SilentlyContinue
if ($existing -and ($existing.Root -ne $RedisRoot)) {
    cmd /c ("subst {0}: /D" -f $driveName) | Out-Null
    $existing = $null
}
if (-not $existing) {
    cmd /c ("subst {0}: {1}" -f $driveName, $RedisRoot) | Out-Null
    $needUnmap = $true
}

try {
    $slnDrive = Join-Path $driveRoot "msvs\RedisServer.sln"
    if (-not (Test-Path $slnDrive)) {
        throw "Drive mapping failed: $slnDrive not found"
    }
    & $MSBuildPath $slnDrive /m:1 /nodeReuse:false /p:Configuration=$Config /p:Platform=$Platform
    if ($LASTEXITCODE -ne 0) {
        throw "MSBuild failed with exit code $LASTEXITCODE"
    }

    $outDir = Join-Path $msvsDir "$Platform\$Config"
    if (-not (Test-Path $outDir)) {
        throw "Build output not found: $outDir"
    }

    $alloc = $Allocator.ToLowerInvariant()
    if ($alloc -eq "tcmalloc") {
        $tcDll = Join-Path $vcpkgBin "tcmalloc_minimal.dll"
        if (Test-Path $tcDll) {
            Copy-Item -Force $tcDll $outDir | Out-Null
        }
    } elseif ($alloc -eq "mimalloc") {
        $miDll = Join-Path $vcpkgBin "mimalloc.dll"
        $miRedirect = Join-Path $vcpkgBin "mimalloc-redirect.dll"
        if (Test-Path $miDll) {
            Copy-Item -Force $miDll $outDir | Out-Null
        }
        if (Test-Path $miRedirect) {
            Copy-Item -Force $miRedirect $outDir | Out-Null
        }
    }

    Write-Host ("Built Redis variant: allocator={0} output={1}" -f $Allocator, $outDir)
} finally {
    if ($needUnmap) {
        cmd /c ("subst {0}: /D" -f $driveName) | Out-Null
    }
}
