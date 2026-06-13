function Get-Hz5WinBenchIncludeFlags {
    param([Parameter(Mandatory = $true)][string]$Hz5Root)

    @(
        "/I$Hz5Root\include",
        "/I$Hz5Root\contract",
        "/I$Hz5Root\core",
        "/I$Hz5Root\fallback",
        "/I$Hz5Root\lowpage",
        "/I$Hz5Root\policy",
        "/I$Hz5Root\route",
        "/I$Hz5Root\smallfront",
        "/I$Hz5Root\wrapper"
    )
}

function Get-Hz5WinBenchLibSources {
    param([Parameter(Mandatory = $true)][string]$Hz5Root)

    @(
        "$Hz5Root\contract\hz5_contract.c",
        "$Hz5Root\core\hz5_segment.c",
        "$Hz5Root\core\hz5_run.c",
        "$Hz5Root\core\hz5_owner.c",
        "$Hz5Root\core\hz5_remote.c",
        "$Hz5Root\core\hz5_tcache.c",
        "$Hz5Root\core\hz5_stats.c",
        "$Hz5Root\route\hz5_route.c",
        "$Hz5Root\wrapper\hz5_wrapper.c",
        "$Hz5Root\policy\hz5_policy.c",
        "$Hz5Root\lowpage\hz5_lowpage64.c",
        "$Hz5Root\lowpage\hz5_lowpage64_os.c",
        "$Hz5Root\lowpage\hz5_lowpage64_p43_segment.c"
    )
}

function Get-Hz5WinBenchFlags {
    @(
        "/DHZ_BENCH_USE_HZ5_POLICY",
        "/DBENCHLAB_HZ5_NO_HZ3_FALLBACK=1",
        "/DBENCHLAB_HZ5_WIN_LOCAL2P=1",
        "/DBENCHLAB_HZ5_WIN_LOCAL2P_OBJECT_NODE=1",
        "/DBENCHLAB_HZ5_WIN_LOCAL2P_REUSE_STATE_ONLY=1",
        "/DBENCHLAB_HZ5_WIN_LOCAL2P_TLS_FAST_RETURN=1"
    )
}

