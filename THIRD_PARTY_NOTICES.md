# Third-Party Notices

This repository includes third-party code and data under their own licenses.
Unless otherwise noted, the Apache-2.0 license in `LICENSE` applies only to
code written for this project (e.g., `hakozuna/hz3`, `core/`, `src/`, `tests/`).

## Bundled dependencies (examples)

- `deps/gperftools-src/` — BSD 3-Clause
  - See `deps/gperftools-src/COPYING`

- `mimalloc/` — MIT
  - See `mimalloc/LICENSE`

- `mimalloc-bench/` — MIT (bench harness)
  - See `mimalloc-bench/LICENSE`

- `mimalloc-bench/extern/` — mixed licenses (MIT/BSD/Apache/GPL/AGPL)
  - Examples:
    - `mimalloc-bench/extern/rocksdb-8.1.1/COPYING` (GPLv2)
    - `mimalloc-bench/extern/rocksdb-8.1.1/LICENSE.Apache` (Apache-2.0)
    - `mimalloc-bench/extern/redis-6.2.7/COPYING` (BSD 3-Clause)
    - `mimalloc-bench/extern/rocksdb-8.1.1/utilities/transactions/lock/range/range_tree/lib/COPYING.AGPLv3` (AGPLv3)
  - Refer to each subdirectory for the authoritative license text.

If you redistribute this repository, you are responsible for complying with
all applicable third-party licenses.
