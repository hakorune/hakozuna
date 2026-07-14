# HZ8 Redis-Like Candidate Gate

- Fresh-process AB/BA
- Runs: 10
- Candidate: medium-transition-inventory
- Work: threads=4, cycles=5000, ops=2000, size=16..256
- Diagnostic counters: disabled

| pattern | default | candidate | median delta | paired delta | default peak | candidate peak | peak delta |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| SET | 56.57M | 57.15M | +1.03% | -0.64% | 16.45 MiB | 16.45 MiB | +0.02% |
| GET | 272.23M | 293.55M | +7.83% | +0.21% | 16.45 MiB | 16.45 MiB | +0.02% |
| LPUSH | 56.59M | 57.41M | +1.45% | -0.39% | 16.45 MiB | 16.45 MiB | +0.02% |
| LPOP | 978.62M | 1,002.72M | +2.46% | +3.92% | 16.45 MiB | 16.45 MiB | +0.02% |
| RANDOM | 170.08M | 199.63M | +17.37% | +9.16% | 16.45 MiB | 16.45 MiB | +0.02% |
