# HZ10 Current Task

HZ10 is a standalone next-substrate research line. Keep this file short.

## Active Direction

```text
status:
  scaffold only
  no allocator behavior implemented yet

design:
  thread-local intrusive freelist pages
  O(1) pagemap route
  remote stack + owner drain
  bounded RSS page pool

first box:
  HZ10PageMapRoute-L0

first GO:
  >=2.0x HZ8 local or 250M+ local0
  remote >=1.2x HZ8
  post RSS <=2x HZ8
```

## Rules

```text
Keep active source files under 800 lines.
Do not copy HZ9 history wholesale.
Do not treat tcmalloc 70% local as the first gate.
Do not weaken fail-closed route without writing the contract first.
```

## Read First

```text
README.md
docs/HZ10_LOCAL_PAGE_SUBSTRATE_TARGET.md
```

