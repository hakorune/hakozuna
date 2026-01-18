---
title: "I just said 'Box!' and somehow beat mimalloc"
published: false
description: How a simple design philosophy led to a memory allocator that outperforms mimalloc and tcmalloc
tags: memory, allocator, performance, cpp
cover_image: https://raw.githubusercontent.com/hakorune/hakozuna/main/docs/images/architecture_overview.png
---

# I just said "Box!" and somehow beat mimalloc

## TL;DR

- Built a memory allocator using **Box Theory** - a super simple design philosophy
- It beats mimalloc and tcmalloc by up to **+28%** in multi-threaded workloads
- The philosophy? Just **"Box!"**

## What's Box Theory?

People ask me this a lot, so here's a Q&A:

**Q: What's Box Theory?**
A: It's a box.

**Q: Why boundary concentration?**
A: Because it's a box! When you create boundaries, responsibilities separate, debugging becomes clear, and development speeds up.

**Q: Why reversibility?**
A: Because it's a box! Just swap the box if needed.

**Q: How do you decide NO-GO?**
A: Benchmarks! Numbers don't lie.

...That's it. Simple, right?

## Let me explain a bit more seriously

Box Theory has 4 principles:

### 1. Boundary Concentration

Minimize boundaries between the hot path (where speed matters) and control layer (where decisions happen).

```
Inside the box = Fast processing. Don't touch.
Box boundary = Control & decisions. Only change here.
```

When you concentrate boundaries to one place, it's clear what to change.

### 2. Reversibility

All optimizations can be toggled with compile-time flags.

```bash
# Normal build
make all_ldpreload_scale

# Want to try a new feature? Just add a flag
make all_ldpreload_scale HZ3_LDPRELOAD_DEFS_EXTRA='-DHZ3_NEW_FEATURE=1'
```

If it doesn't work, just revert. Swap the box.

### 3. Observability

Constant logging is heavy, so we dump stats only once at exit (SSOT method).

This ensures reproducibility - you can track "that build from that time."

### 4. Fail-Fast

Catch bugs at box boundaries. Don't let them inside.

Find something wrong? Crash immediately. Makes debugging easier.

## hakozuna (hz3) Architecture

So here's the memory allocator I built with this philosophy:

![Architecture](https://raw.githubusercontent.com/hakorune/hakozuna/main/docs/images/architecture_overview.png)

It has 3 layers (boxes):

| Layer | What it does |
|-------|--------------|
| **Hot Path** | Fastest alloc/free. Completes in TLS |
| **Cache Layer** | Buffering. Owner Stash, RemoteStash Ring |
| **Central Layer** | Shared between threads |

Each layer is a "box." You only pay costs when crossing boundaries.

## Benchmark Results

So what happened?

| Benchmark | Condition | vs mimalloc |
|-----------|-----------|-------------|
| **Larson** | T=8-16 | **+15%** 🎉 |
| **memcached** | T=4 | **+10%** 🎉 |
| **MT remote** | T=8 R=90% | **+28%** 🎉 |
| random_mixed | - | About the same |

It's especially strong in "remote-free" situations (allocate in thread A, free in thread B).

### Why did it win?

Honestly, I don't fully understand everything, but...

- **Owner Stash**: Buffers remote frees to avoid mutex contention
- **RemoteStash Ring**: Reduced TLS size by 92%
- **Full Drain Exchange**: Bulk collection with atomic exchange

Since everything was separated into boxes, each part could be optimized independently.

## I failed a lot too (NO-GO cases)

The great thing about Box Theory is you can always revert.

Actually, 20+ optimizations were rejected (NO-GO):

| What I tried | Result | Lesson |
|--------------|--------|--------|
| SegMath optimization | NO-GO | "Obviously faster" was disproven by benchmarks |
| Page-Local Remote | NO-GO | Only synthetic benchmarks improved, others regressed |
| PGO | NO-GO | Different conditions made it slower |

**If benchmarks say no, revert.** That's the rule.

## About the hakorune project

hakozuna (hz3) is part of the [hakorune](https://github.com/hakorune) project.

hakorune is a programming language built with Box Theory - it runs on both VM and LLVM after MIR conversion.

The compiler is... still a work in progress 😅

## Summary

```
Q: How did you beat mimalloc?
A: I just separated things into boxes and it happened

Q: How do you decide the design?
A: Benchmarks!

Q: What if it doesn't work?
A: Just swap the box
```

Turns out a simple philosophy can produce real results.

## Links

- **GitHub**: https://github.com/hakorune/hakozuna
- **Paper (English)**: https://github.com/hakorune/hakozuna/blob/main/docs/paper/main_en.pdf
- **Paper (Japanese)**: https://github.com/hakorune/hakozuna/blob/main/docs/paper/main_ja.pdf

---

*By the way, I built this project with Claude (AI). Design philosophy by me, implementation by AI. Maybe that's also a kind of "box"...?*
