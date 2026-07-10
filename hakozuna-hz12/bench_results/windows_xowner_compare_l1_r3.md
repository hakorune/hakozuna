# HZ12 Windows Cross-Owner Compare L1

- Runs: 3, runtime: 5s
- Producers/consumers: 4/4, ring: 4096
- Size: 8..1024, cross-owner free rate: 100%, sharing factor: 2.0

| Allocator | Median ops/s | Runs |
| --- | ---: | ---: |
| hz11-ownerless | 9.138M | 3 |
| hz12-owner-inbox-l1 | 24.516M | 3 |
| tcmalloc | 28.553M | 3 |
