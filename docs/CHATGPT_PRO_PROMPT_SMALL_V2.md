# ChatGPT Pro Prompt: hz3 Small v2 Design

## Context

- Project: hz3 allocator (hakozuna/hz3).
- Small v2 (16-2048B) + medium v1 (4KB-32KB) mixed is fastest.
- v2-only regressions: small/mixed drop; v1 small path is still faster.
- Hot path constraints: no pthread/getenv; compile-time gates only.
- PageTagMap (PTAG) already exists: arena range check + 1 tag load in hot path.
- v2 small uses a page header (magic/owner/sc); v1 small uses segmap->sc_tag.
- Safety: never misroute v2 ptr to v1 free (false negatives OK, false positives NG).

## Goal

Make v2 small at least as fast as v1 small without bloating hot paths.

## Questions

1) Propose 3 design options to make v2 small match or beat v1 small.
   - For each: expected effect, complexity, and risks.
2) Can PageTagMap fully replace the page header for v2 small?
   - If yes, outline how to keep correctness with only PTAG + range check.
3) Is there a design that keeps v1+v2 mixed (best today) but removes the fixed costs?
   - Focus on hot path costs and shared-structure overhead.

## Output format

- Short bullet list per option (effect, complexity, risk).
- One recommended option with rationale.
