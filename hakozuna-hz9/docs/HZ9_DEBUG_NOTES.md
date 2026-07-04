# HZ9 Debug Notes

Durable debugging record for rare bugs and investigation trails that are too
detailed for `HZ9_CURRENT_STATUS.md`.

## 20260705 Owner Flush All-Free Release

Commit: `4e6a50d1` (`HZ9: release all-free owner pages on owner flush`)

Symptom class:

- Rare owner-exit retention in `HZ9OwnerLocalPagePoolPureLocal-L1`.
- The bug required `h9_owner_page_flush_owner()` to see a page that had no live
  slots left after pending remote frees were collected.

Root cause:

- The old owner-flush path detached every page owned by the exiting owner.
- Detach is correct for partially live pages, because final exact frees still
  need a global route target.
- Detach is not enough for already all-free pages: there is no future exact
  free left to release the mapping, so memory can remain retained.

Fix:

- Collect pending under `h9_owner_page_global_lock`.
- If the page is all-free:
  - unregister it from the global route list,
  - start drain,
  - detach it,
  - queue it for release,
  - drop the lock,
  - destroy/release through `h9_owner_page_destroy_unlocked()`.
- If the page is not all-free, only detach it; it remains globally routeable
  until final exact frees arrive.

Why the shared destroy helper matters:

- Normal thread/page release, local final release, and owner-flush all-free
  release now share the same `page_release` and `page_bytes_release` counter
  updates.

