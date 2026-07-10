# HZ12 Windows Owner Inbox Cap Sweep

- Runs: 3, runtime: 5s, drain interval: 256
- Producers/consumers: 4/4, ring: 4096, size: 8..1024

| Inbox cap | Median ops/s | Peak RSS MiB | Final RSS MiB | Overflow objects | Inbox max |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 1024 | 24.612M | 17.04 | 16.89 | 4,007 | 1,021 |
| 2048 | 24.680M | 18.01 | 17.86 | 256 | 1,829 |
| 512 | 24.217M | 18.32 | 18.18 | 281,076 | 512 |
