# HZ8 Redis-Like Candidate Gate

- Fresh-process AB/BA
- Runs: 10
- Candidate: medium-transition-inventory
- Work: threads=4, cycles=5000, ops=2000, size=16..256
- Diagnostic counters: disabled

| pattern | default | candidate | median delta | paired delta | default peak | candidate peak | peak delta |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| SET | 64.37M | 64.64M | +0.44% | -1.46% | 16.48 MiB | 16.51 MiB | +0.17% |
| GET | 296.89M | 286.19M | -3.60% | -3.59% | 16.48 MiB | 16.51 MiB | +0.17% |
| LPUSH | 57.74M | 56.72M | -1.78% | -1.38% | 16.48 MiB | 16.51 MiB | +0.17% |
| LPOP | 999.55M | 996.96M | -0.26% | +3.82% | 16.48 MiB | 16.51 MiB | +0.17% |
| RANDOM | 202.72M | 170.25M | -16.01% | -8.37% | 16.48 MiB | 16.51 MiB | +0.17% |
