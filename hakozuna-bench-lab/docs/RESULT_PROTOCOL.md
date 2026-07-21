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
  "laneKind": "speed",
  "artifactSha256": "...",
  "workloadId": "mt-remote90",
  "command": ["bench.exe", "16", "50000"],
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
kind (`speed`, `diagnostic`, or `research`), and build provenance. Verified
speed charts accept only `speed` lanes. A hash mismatch blocks execution until
the user deliberately refreshes the manifest.

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
