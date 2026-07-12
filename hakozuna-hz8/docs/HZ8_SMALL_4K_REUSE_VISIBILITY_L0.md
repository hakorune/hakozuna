# HZ8 Small 4K Reuse Visibility L0

## Question

Fixed 4KiB remains about three times slower than tcmalloc, while replacing the
small path with the R3 page substrate regressed 4.92%. Determine whether HZ8
commits new spans while usable same-owner spans exist outside the active hint
and Mag16.

## Observation

The diagnostic hook runs only immediately before a source span commit. It scans
the current owner's selected class list under the existing lock and reports:

```text
checkpoint count
owner-list spans scanned
usable spans
usable spans hidden from active + Mag16
hidden bytes and maximum hidden bytes
full and state-blocked spans
```

The lane also enables the existing speed attribution counters. Production and
normal speed lanes preprocess the hook to a no-op.

## Gate

```text
hidden usable >= 1 per commit at a material rate:
  open one O(1) visibility/index design box

hidden usable approximately zero:
  visibility is not the blocker; close this path
```

Do not enable the normal owner-list scan as behavior. Its skip rule is an
intentional hot-path decision, and this L0 only tests whether an O(1) index has
enough potential value to justify a separate design.

## Windows Result

Fixed 4KiB, T=4, working set 4096:

```text
alloc attempts / failures: 305,088 / 0
span commits: 17,548
Mag empty pops, class 8: 17,544
normal owner-list scan steps: 0

shadow owner-list spans scanned: 38,482,764
usable spans: 36,166,500
hidden usable spans: 36,166,500
average hidden per commit: about 2,061
maximum hidden at one commit: 4,230 / 264.38MiB
state blockers: 0
```

The signal is decisive: source commits occur while thousands of same-owner
usable spans exist outside the active hint and Mag16. The current skip avoids
an O(n) scan but converts visibility loss into source growth.

## Decision

```text
visibility hypothesis: CONFIRMED
owner-list scan as behavior: FORBIDDEN
next box: SmallAvailableIndex-L1, O(1), class 8 first
```

The index must be advisory and validated on pop. Push only on a full-to-usable
transition, prevent duplicate membership, and clear membership before reuse or
lifecycle handoff. Production counters and global atomics remain forbidden.
