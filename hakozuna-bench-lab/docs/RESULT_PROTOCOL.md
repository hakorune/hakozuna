# Result Protocol

The GUI consumes normalized result bundles rather than scraping Markdown.
Schema version `1` is the Windows MVP contract.

## Bundle Layout

```text
result-id/
  manifest.json
  samples.jsonl
  summary.json
  stdout/
    0001.txt
  stderr/
    0001.txt
  report.md
```

## Required Manifest Fields

```json
{
  "schemaVersion": 1,
  "mode": "verified",
  "allocatorId": "hz8-default",
  "allocatorLabel": "HZ8",
  "allocatorVersion": "hz8-balanced-2026.07",
  "allocatorBuildCommit": "8a73a4ab...",
  "providerPath": "out_win_suite/bench_mixed_ws_hz8.exe",
  "laneKind": "speed",
  "artifactSha256": "...",
  "workloadId": "mt-remote90",
  "runnerVersion": "2026.07",
  "runnerSha256": "...",
  "command": ["bench.exe", "16", "50000"],
  "environment": {
    "H8_PROFILE": "balanced",
    "H8_API_TOKEN": "<redacted>"
  },
  "platform": "windows-x64",
  "runOrder": "ABBA",
  "requestedRuns": 10,
  "createdUtc": "2026-07-21T00:00:00Z"
}
```

## Required Sample Fields

```text
sampleIndex
startedUtc
durationSeconds
operations
opsPerSecond
peakWorkingSetBytes
postWorkingSetBytes
privateBytes
exitCode
timedOut
allocationFailures
stdoutPath
stderrPath
```

Missing memory fields are represented as `null`, never zero. Failed samples
remain in `samples.jsonl` and are excluded from a median only with an explicit
failure count in `summary.json`.

## Identity and Lane Safety

Allocator manifests require an immutable ID, expected executable hash, lane
kind (`speed`, `diagnostic`, or `research`), version, provider path, and build
provenance. Workload manifests independently record runner version and hash.
Verified speed charts accept only `speed` lanes with complete allocator and
runner identities. A hash mismatch blocks execution until the user deliberately
refreshes the manifest.

Allocator and workload environment overrides are merged into the child process
and recorded with the result. The report captures only allocator/benchmark
families such as `H8_*`, `MIMALLOC_*`, `TCMALLOC_*`, `MALLOC_*`, `BENCH_*`,
`LD_PRELOAD`, and `DYLD_INSERT_LIBRARIES`. Names containing token, password,
secret, authentication, cookie, credential, or private-key markers are stored
as `<redacted>`.

## Existing Output Adapters

The first parser recognizes the repository's stable markers:

```text
[BENCH_ARGS]
[EFFECTIVE_REMOTE]
[ALLOC_FAILURES]
[PROCESS_MEMORY]
threads=... ops=... time=... ops/s=...
```

Allocator-specific diagnostic markers may be stored as raw evidence but do not
enter the common speed schema.
