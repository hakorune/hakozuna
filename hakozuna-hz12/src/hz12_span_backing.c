#include "hz12_span_backing.h"

#include "hz12_span.h"

#include <stdint.h>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <errno.h>
#include <sys/mman.h>
#endif

static int h12_span_backing_address_valid(void* span_base) {
  return span_base && hz12_arena_contains(span_base) &&
      ((((uintptr_t)span_base - (uintptr_t)hz12_arena_base) &
        (HZ12_SPAN_BYTES - 1u)) == 0u);
}

int h12_span_backing_discard(void* span_base,
                             uint32_t* before_state,
                             uint32_t* after_state) {
  if (!h12_span_backing_address_valid(span_base)) return 0;
#if defined(_WIN32)
  {
    MEMORY_BASIC_INFORMATION before;
    MEMORY_BASIC_INFORMATION after;
    if (VirtualQuery(span_base, &before, sizeof(before)) != sizeof(before) ||
        before.State != MEM_COMMIT) return 0;
    if (before_state) *before_state = before.State;
    if (!VirtualFree(span_base, HZ12_SPAN_BYTES, MEM_DECOMMIT)) return 0;
    if (VirtualQuery(span_base, &after, sizeof(after)) != sizeof(after) ||
        after.State != MEM_RESERVE) return 0;
    if (after_state) *after_state = after.State;
  }
#else
  int rc;
  if (before_state) *before_state = HZ12_BACKING_STATE_RESIDENT;
  do {
    rc = madvise(span_base, HZ12_SPAN_BYTES, MADV_DONTNEED);
  } while (rc != 0 && errno == EINTR);
  if (rc != 0) return 0;
  if (after_state) *after_state = HZ12_BACKING_STATE_DISCARDED;
#endif
  return 1;
}

int h12_span_backing_recommit(void* span_base,
                              uint32_t* before_state,
                              uint32_t* after_state) {
  if (!h12_span_backing_address_valid(span_base)) return 0;
#if defined(_WIN32)
  {
    MEMORY_BASIC_INFORMATION before;
    MEMORY_BASIC_INFORMATION after;
    if (VirtualQuery(span_base, &before, sizeof(before)) != sizeof(before) ||
        before.State != MEM_RESERVE) return 0;
    if (before_state) *before_state = before.State;
    if (VirtualAlloc(span_base, HZ12_SPAN_BYTES, MEM_COMMIT,
                     PAGE_READWRITE) != span_base) return 0;
    if (VirtualQuery(span_base, &after, sizeof(after)) != sizeof(after) ||
        after.State != MEM_COMMIT) return 0;
    if (after_state) *after_state = after.State;
  }
#else
  if (before_state) *before_state = HZ12_BACKING_STATE_DISCARDED;
  /* MADV_DONTNEED preserves the mapping; the next write faults in zero pages. */
  *(volatile unsigned char*)span_base = 0u;
  if (after_state) *after_state = HZ12_BACKING_STATE_RESIDENT;
#endif
  return 1;
}

int h12_span_backing_validate_discarded(void* span_base) {
  if (!h12_span_backing_address_valid(span_base)) return 0;
#if defined(_WIN32)
  {
    MEMORY_BASIC_INFORMATION memory;
    return VirtualQuery(span_base, &memory, sizeof(memory)) == sizeof(memory) &&
        memory.State == MEM_RESERVE;
  }
#else
  /* Linux keeps MADV_DONTNEED ranges mapped; depot ownership is the authority. */
  return 1;
#endif
}
