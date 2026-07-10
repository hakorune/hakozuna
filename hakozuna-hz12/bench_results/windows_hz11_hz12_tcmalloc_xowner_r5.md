# HZ12 Windows Cross-Owner Compare L1

- Runs: 5, runtime: 5s
- Producers/consumers: 4/4, ring: 4096
- Size: 8..1024, cross-owner free rate: 100%, sharing factor: 2.0

| Allocator | Median ops/s | Runs |
| --- | ---: | ---: |
| hz11-ownerless | 5.828M | 5 |
| hz12-owner-inbox-l1 | 10.634M | 5 |
| tcmalloc | 27.440M | 5 |
