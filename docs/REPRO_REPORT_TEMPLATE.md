# Reproduction Report Template

Use this template when reporting:

- benchmark results
- performance regressions
- preload or integration failures
- Ubuntu/Linux or Windows-native compatibility observations

GitHub Issues now provide structured forms for `bug`, `perf`, and `compatibility` reports.
If you are posting a manual report, discussion note, or external reproduction, copy the template below.

## Template

```text
Summary:
- short description of the result or problem

Allocator / profile:
- hz3 / hz4 / other

Platform:
- Ubuntu/Linux / Windows native / WSL / other

Environment:
- CPU:
- OS:
- Compiler:
- glibc or MSVC:

Commit or release:
- commit hash or tag

Workload / lane:
- main_r0 / cross128_r90 / Redis balanced / memcached ext-client / Larson T16 / other

Command:
- exact build and run command

RUNS:
- example: 10

Observed result:
- median ops/s, RSS, latency, crash signature, or other key metric

Expected or baseline:
- previous result, expected behavior, or comparison allocator

Notes:
- preload order
- build flags
- working set
- object sizes
- logs or artifact links
```

## Minimum Useful Report

If you want the shortest version that still helps:

```text
allocator:
platform:
commit:
workload:
command:
result:
baseline:
```

## Tips

- Prefer median-based results when variance is non-trivial.
- Include the exact lane or workload name, not just "benchmark."
- For preload issues, include allocator order and whether another allocator was active.
- For Windows-native reports, include the script name under `win/` when possible.
