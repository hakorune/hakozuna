#include "h8_internal.h"
#include "h8_hz9_local_slab_inline_body.h"
#include "h8_hz9_local_slab_route_boundary.h"

#if defined(H9_LOCAL_SLAB_PAGE_ROUTE_BOUNDARY_L0)

#include <string.h>

#define H9_LSP_SEGMENT_BYTES (256u * 1024u)
#define H9_LSP_PAYLOAD_OFFSET 4096u
#define H9_LSP_SEGMENT_MAGIC 0x48394c53u
#define H9_LSP_SEGMENT_DEAD 0x48394445u
#define H9_LSP_MAX_SEGMENTS 1024u
#define H9_LSP_HASH_CAP 2048u

#if defined(__GNUC__) || defined(__clang__)
#define H9_LSP_COLD_NOINLINE __attribute__((noinline, cold))
#else
#define H9_LSP_COLD_NOINLINE
#endif

typedef struct H9LspSegment {
  uint32_t magic;
  uint32_t generation;
  uint16_t class_id;
  uint16_t slot_count;
  uint32_t slot_size;
  uint64_t free_bits;
  uint64_t alloc_bits;
  uint64_t cache_bits;
  uint64_t pending_bits;
  void* raw_base;
  size_t raw_bytes;
} H9LspSegment;

static h8_platform_once_t h9_lsp_once = H8_PLATFORM_ONCE_INIT;
static h8_platform_mutex_t h9_lsp_lock;
static H9LspSegment* h9_lsp_segments[H9_LSP_MAX_SEGMENTS];
static uintptr_t h9_lsp_hash[H9_LSP_HASH_CAP];
static uint32_t h9_lsp_generation = 1u;
static H9LspStats h9_lsp_stats;
static _Thread_local H9LspSegment* h9_lsp_active[H8_MEDIUM_CLASS_COUNT];
static _Thread_local void* h9_lsp_last_ptr;
static _Thread_local H9LspSegment* h9_lsp_last_segment;
static _Thread_local uint32_t h9_lsp_last_slot;
static _Thread_local uint32_t h9_lsp_last_generation;
static _Thread_local uint8_t h9_lsp_last_live;

#define H9_LSP_ENTRY_LEDGER_SIZE 64u

typedef struct H9LspEntryToken {
  void* ptr;
  H9LspSegment* segment;
  uint32_t slot;
  uint32_t generation;
  uint32_t class_id;
  uint32_t slot_size;
  uint8_t live;
} H9LspEntryToken;

static _Thread_local H9LspEntryToken
    h9_lsp_entry_ledger[H9_LSP_ENTRY_LEDGER_SIZE];

static void h9_lsp_init_once(void) {
  h8_platform_mutex_init(&h9_lsp_lock);
}

static void h9_lsp_init(void) {
  h8_platform_once(&h9_lsp_once, h9_lsp_init_once);
}

static uint32_t h9_lsp_entry_token_index(const void* ptr) {
  return (uint32_t)(((uintptr_t)ptr >> 6u) &
                    (uintptr_t)(H9_LSP_ENTRY_LEDGER_SIZE - 1u));
}

static void h9_lsp_entry_token_clear(H9LspEntryToken* token) {
  if (!token) {
    return;
  }
  *token = (H9LspEntryToken){0};
}

static H9LspEntryToken* h9_lsp_entry_token_lookup(void* ptr) {
  if (!ptr) {
    return NULL;
  }
  H9LspEntryToken* token =
      &h9_lsp_entry_ledger[h9_lsp_entry_token_index(ptr)];
  return token->live && token->ptr == ptr ? token : NULL;
}

static void h9_lsp_entry_token_insert(void* ptr, H9LspSegment* segment,
                                      uint32_t slot) {
  if (!ptr || !segment) {
    return;
  }
  H9LspEntryToken* token =
      &h9_lsp_entry_ledger[h9_lsp_entry_token_index(ptr)];
  *token = (H9LspEntryToken){
      .ptr = ptr,
      .segment = segment,
      .slot = slot,
      .generation = segment->generation,
      .class_id = segment->class_id,
      .slot_size = segment->slot_size,
      .live = 1u,
  };
}

static bool h9_lsp_entry_token_valid(const H9LspEntryToken* token) {
  if (!token || !token->live || !token->segment ||
      token->generation != token->segment->generation ||
      token->slot >= token->segment->slot_count) {
    return false;
  }
  uint64_t bit = UINT64_C(1) << token->slot;
  return (token->segment->alloc_bits & bit) != 0u &&
         (token->segment->free_bits & bit) == 0u &&
         (token->segment->cache_bits & bit) == 0u &&
         (token->segment->pending_bits & bit) == 0u;
}

static size_t h9_lsp_hash_pos(uintptr_t base) {
  return (size_t)((base / H9_LSP_SEGMENT_BYTES) *
                  UINT64_C(11400714819323198485)) &
         (H9_LSP_HASH_CAP - 1u);
}

static void h9_lsp_hash_insert(uintptr_t base, H9LspSegment* segment) {
  size_t pos = h9_lsp_hash_pos(base);
  for (size_t i = 0; i < H9_LSP_HASH_CAP; ++i) {
    size_t at = (pos + i) & (H9_LSP_HASH_CAP - 1u);
    if (h9_lsp_hash[at] == 0u || h9_lsp_hash[at] == (uintptr_t)segment) {
      h9_lsp_hash[at] = (uintptr_t)segment;
      return;
    }
  }
}

static H9LspSegment* h9_lsp_hash_find(uintptr_t base) {
  size_t pos = h9_lsp_hash_pos(base);
  for (size_t i = 0; i < H9_LSP_HASH_CAP; ++i) {
    uintptr_t current = h9_lsp_hash[(pos + i) & (H9_LSP_HASH_CAP - 1u)];
    if (current == 0u) {
      return NULL;
    }
    H9LspSegment* segment = (H9LspSegment*)current;
    if ((uintptr_t)segment == base && segment->magic == H9_LSP_SEGMENT_MAGIC) {
      return segment;
    }
  }
  return NULL;
}

static uint32_t h9_lsp_slot_count_for(uint32_t slot_size) {
  size_t usable = H9_LSP_SEGMENT_BYTES - H9_LSP_PAYLOAD_OFFSET;
  size_t slots = slot_size ? usable / slot_size : 0u;
  if (slots > 64u) {
    slots = 64u;
  }
  return (uint32_t)slots;
}

static H9LspSegment* h9_lsp_create_segment(uint32_t class_id) {
  if (class_id >= H8_MEDIUM_CLASS_COUNT) {
    return NULL;
  }
  const H8MediumClassSpec* spec = h8_medium_class_spec(class_id);
  if (!spec || spec->slot_size == 0u) {
    return NULL;
  }
  uint32_t slot_count = h9_lsp_slot_count_for(spec->slot_size);
  if (slot_count == 0u) {
    return NULL;
  }

  h8_platform_mutex_lock(&h9_lsp_lock);
  bool has_slot = false;
  for (size_t i = 0; i < H9_LSP_MAX_SEGMENTS; ++i) {
    if (!h9_lsp_segments[i]) {
      has_slot = true;
      break;
    }
  }
  h8_platform_mutex_unlock(&h9_lsp_lock);
  if (!has_slot) {
    h9_lsp_stats.segment_cap_reject++;
    return NULL;
  }

  size_t raw_bytes = (size_t)H9_LSP_SEGMENT_BYTES * 2u;
  void* raw = h8_platform_reserve(raw_bytes);
  if (!raw) {
    return NULL;
  }
  uintptr_t base = ((uintptr_t)raw + H9_LSP_SEGMENT_BYTES - 1u) &
                   ~(uintptr_t)(H9_LSP_SEGMENT_BYTES - 1u);
  if (h8_platform_commit((void*)base, H9_LSP_SEGMENT_BYTES) != 0) {
    h8_platform_release(raw, raw_bytes);
    return NULL;
  }

  h8_platform_mutex_lock(&h9_lsp_lock);
  size_t index = H9_LSP_MAX_SEGMENTS;
  for (size_t i = 0; i < H9_LSP_MAX_SEGMENTS; ++i) {
    if (!h9_lsp_segments[i]) {
      index = i;
      break;
    }
  }
  if (index == H9_LSP_MAX_SEGMENTS) {
    h8_platform_mutex_unlock(&h9_lsp_lock);
    h8_platform_release(raw, raw_bytes);
    h9_lsp_stats.segment_cap_reject++;
    return NULL;
  }

  H9LspSegment* segment = (H9LspSegment*)base;
  uint64_t full_bits = slot_count == 64u
                           ? UINT64_MAX
                           : ((UINT64_C(1) << slot_count) - 1u);
  *segment = (H9LspSegment){
      .magic = H9_LSP_SEGMENT_MAGIC,
      .generation = h9_lsp_generation++,
      .class_id = (uint16_t)class_id,
      .slot_count = (uint16_t)slot_count,
      .slot_size = spec->slot_size,
      .free_bits = full_bits,
      .raw_base = raw,
      .raw_bytes = raw_bytes,
  };
  h9_lsp_segments[index] = segment;
  h9_lsp_hash_insert(base, segment);
  h9_lsp_stats.segment_create++;
  h9_lsp_stats.segment_live++;
  h9_lsp_stats.segment_committed_bytes += H9_LSP_SEGMENT_BYTES;
  h9_lsp_stats.segment_reserved_bytes += raw_bytes;
  if (h9_lsp_stats.segment_committed_bytes >
      h9_lsp_stats.segment_committed_peak_bytes) {
    h9_lsp_stats.segment_committed_peak_bytes =
        h9_lsp_stats.segment_committed_bytes;
  }
  if (h9_lsp_stats.segment_reserved_bytes >
      h9_lsp_stats.segment_reserved_peak_bytes) {
    h9_lsp_stats.segment_reserved_peak_bytes =
        h9_lsp_stats.segment_reserved_bytes;
  }
  h8_platform_mutex_unlock(&h9_lsp_lock);
  return segment;
}

static void* h9_lsp_slot_ptr(const H9LspSegment* segment, uint32_t slot) {
  return (void*)((uintptr_t)segment + H9_LSP_PAYLOAD_OFFSET +
                 (uintptr_t)slot * (uintptr_t)segment->slot_size);
}

void h9_lsp_debug_reset(void) {
  h9_lsp_init();
  h8_platform_mutex_lock(&h9_lsp_lock);
  for (size_t i = 0; i < H9_LSP_MAX_SEGMENTS; ++i) {
    H9LspSegment* segment = h9_lsp_segments[i];
    if (!segment) {
      continue;
    }
    void* raw = segment->raw_base;
    size_t raw_bytes = segment->raw_bytes;
    segment->magic = H9_LSP_SEGMENT_DEAD;
    h9_lsp_segments[i] = NULL;
    h9_lsp_stats.segment_release++;
    if (h9_lsp_stats.segment_live > 0u) {
      h9_lsp_stats.segment_live--;
    }
    if (h9_lsp_stats.segment_committed_bytes >= H9_LSP_SEGMENT_BYTES) {
      h9_lsp_stats.segment_committed_bytes -= H9_LSP_SEGMENT_BYTES;
    } else {
      h9_lsp_stats.segment_committed_bytes = 0u;
    }
    if (h9_lsp_stats.segment_reserved_bytes >= raw_bytes) {
      h9_lsp_stats.segment_reserved_bytes -= raw_bytes;
    } else {
      h9_lsp_stats.segment_reserved_bytes = 0u;
    }
    h8_platform_release(raw, raw_bytes);
  }
  memset(h9_lsp_hash, 0, sizeof(h9_lsp_hash));
  memset(h9_lsp_active, 0, sizeof(h9_lsp_active));
  memset(h9_lsp_entry_ledger, 0, sizeof(h9_lsp_entry_ledger));
  h9_lsp_last_ptr = NULL;
  h9_lsp_last_segment = NULL;
  h9_lsp_last_slot = 0u;
  h9_lsp_last_generation = 0u;
  h9_lsp_last_live = 0u;
  h8_platform_mutex_unlock(&h9_lsp_lock);
  memset(&h9_lsp_stats, 0, sizeof(h9_lsp_stats));
}

H9LspRouteResult h9_lsp_debug_route(void* ptr) {
  h9_lsp_stats.route_attempt++;
  H9LspRouteResult result = {
      .kind = H8_ROUTE_MISS,
      .reason = H9_LSP_ROUTE_REASON_MISS,
  };
  if (!ptr) {
    h9_lsp_stats.route_miss++;
    return result;
  }
  uintptr_t addr = (uintptr_t)ptr;
  uintptr_t base = addr & ~(uintptr_t)(H9_LSP_SEGMENT_BYTES - 1u);
  H9LspSegment* segment = h9_lsp_hash_find(base);
  if (!segment) {
    h9_lsp_stats.route_miss++;
    return result;
  }

  result.segment = segment;
  result.payload_base = (void*)(base + H9_LSP_PAYLOAD_OFFSET);
  result.class_id = segment->class_id;
  result.slot_size = segment->slot_size;
  result.slot_count = segment->slot_count;
  result.kind = H8_ROUTE_INVALID;

  uintptr_t payload = base + H9_LSP_PAYLOAD_OFFSET;
  if (addr < payload) {
    result.reason = H9_LSP_ROUTE_REASON_INTERIOR;
    h9_lsp_stats.route_invalid++;
    return result;
  }
  uintptr_t offset = addr - payload;
  size_t used = (size_t)segment->slot_size * (size_t)segment->slot_count;
  if (offset >= used) {
    result.reason = H9_LSP_ROUTE_REASON_TAIL;
    h9_lsp_stats.route_invalid++;
    return result;
  }
  if ((offset % (uintptr_t)segment->slot_size) != 0u) {
    result.reason = H9_LSP_ROUTE_REASON_INTERIOR;
    h9_lsp_stats.route_invalid++;
    return result;
  }

  uint32_t slot = (uint32_t)(offset / (uintptr_t)segment->slot_size);
  uint64_t bit = UINT64_C(1) << slot;
  result.slot = slot;
  if ((segment->alloc_bits & bit) == 0u || (segment->free_bits & bit) != 0u ||
      (segment->cache_bits & bit) != 0u || (segment->pending_bits & bit) != 0u) {
    result.reason = H9_LSP_ROUTE_REASON_DOUBLE;
    h9_lsp_stats.route_invalid++;
    return result;
  }

  result.kind = H8_ROUTE_VALID;
  result.reason = H9_LSP_ROUTE_REASON_NONE;
  h9_lsp_stats.route_valid++;
  return result;
}

H9LspRouteResult h9_lsp_debug_route_direct_owned(void* ptr) {
  h9_lsp_stats.route_attempt++;
  H9LspRouteResult result = {
      .kind = H8_ROUTE_INVALID,
      .reason = H9_LSP_ROUTE_REASON_INTERIOR,
  };
  uintptr_t addr = (uintptr_t)ptr;
  uintptr_t base = addr & ~(uintptr_t)(H9_LSP_SEGMENT_BYTES - 1u);
  H9LspSegment* segment = (H9LspSegment*)base;
  if (!ptr || segment->magic != H9_LSP_SEGMENT_MAGIC) {
    result.kind = H8_ROUTE_MISS;
    result.reason = H9_LSP_ROUTE_REASON_MISS;
    h9_lsp_stats.route_miss++;
    return result;
  }
  result.segment = segment;
  result.payload_base = (void*)(base + H9_LSP_PAYLOAD_OFFSET);
  result.class_id = segment->class_id;
  result.slot_size = segment->slot_size;
  result.slot_count = segment->slot_count;
  uintptr_t payload = base + H9_LSP_PAYLOAD_OFFSET;
  uintptr_t offset = addr - payload;
  if (addr < payload ||
      offset >= (size_t)segment->slot_size * (size_t)segment->slot_count ||
      (offset % (uintptr_t)segment->slot_size) != 0u) {
    h9_lsp_stats.route_invalid++;
    return result;
  }
  uint32_t slot = (uint32_t)(offset / (uintptr_t)segment->slot_size);
  uint64_t bit = UINT64_C(1) << slot;
  result.slot = slot;
  if ((segment->alloc_bits & bit) == 0u || (segment->free_bits & bit) != 0u ||
      (segment->cache_bits & bit) != 0u || (segment->pending_bits & bit) != 0u) {
    result.reason = H9_LSP_ROUTE_REASON_DOUBLE;
    h9_lsp_stats.route_invalid++;
    return result;
  }
  result.kind = H8_ROUTE_VALID;
  result.reason = H9_LSP_ROUTE_REASON_NONE;
  h9_lsp_stats.route_valid++;
  return result;
}

void* h9_lsp_debug_alloc(uint32_t class_id) {
  h9_lsp_init();
  h9_lsp_stats.malloc_call++;
  if (class_id >= H8_MEDIUM_CLASS_COUNT) {
    return NULL;
  }
  H9LspSegment* segment = h9_lsp_active[class_id];
  bool created = false;
  if (!segment || segment->free_bits == 0u) {
    segment = h9_lsp_create_segment(class_id);
    h9_lsp_active[class_id] = segment;
    created = segment != NULL;
  } else {
    h9_lsp_stats.malloc_tls_page_hit++;
  }
  if (!segment || segment->free_bits == 0u) {
    return NULL;
  }
  uint32_t slot = (uint32_t)__builtin_ctzll(segment->free_bits);
  uint64_t bit = UINT64_C(1) << slot;
  segment->free_bits &= ~bit;
  segment->alloc_bits |= bit;
  if (created) {
    h9_lsp_stats.malloc_page_create++;
  }
  return h9_lsp_slot_ptr(segment, slot);
}

bool h9_lsp_debug_alloc_slot(uint32_t class_id, void** ptr_out,
                             uint32_t* slot_out) {
  h9_lsp_init();
  h9_lsp_stats.malloc_call++;
  if (ptr_out) {
    *ptr_out = NULL;
  }
  if (slot_out) {
    *slot_out = UINT32_MAX;
  }
  if (class_id >= H8_MEDIUM_CLASS_COUNT) {
    return false;
  }
  H9LspSegment* segment = h9_lsp_active[class_id];
  bool created = false;
  if (!segment || segment->free_bits == 0u) {
    segment = h9_lsp_create_segment(class_id);
    h9_lsp_active[class_id] = segment;
    created = segment != NULL;
  } else {
    h9_lsp_stats.malloc_tls_page_hit++;
  }
  if (!segment || segment->free_bits == 0u) {
    return false;
  }
  uint32_t slot = (uint32_t)__builtin_ctzll(segment->free_bits);
  uint64_t bit = UINT64_C(1) << slot;
  segment->free_bits &= ~bit;
  segment->alloc_bits |= bit;
  if (created) {
    h9_lsp_stats.malloc_page_create++;
  }
  if (ptr_out) {
    *ptr_out = h9_lsp_slot_ptr(segment, slot);
  }
  if (slot_out) {
    *slot_out = slot;
  }
  return true;
}

void* h9_lsp_debug_ptrtoken_alloc(uint32_t class_id) {
  h9_lsp_init();
  h9_lsp_stats.malloc_call++;
  if (class_id >= H8_MEDIUM_CLASS_COUNT) {
    return NULL;
  }
  H9LspSegment* segment = h9_lsp_active[class_id];
  bool created = false;
  if (!segment || segment->free_bits == 0u) {
    segment = h9_lsp_create_segment(class_id);
    h9_lsp_active[class_id] = segment;
    created = segment != NULL;
  } else {
    h9_lsp_stats.malloc_tls_page_hit++;
  }
  if (!segment || segment->free_bits == 0u) {
    return NULL;
  }
  uint32_t slot = (uint32_t)__builtin_ctzll(segment->free_bits);
  uint64_t bit = UINT64_C(1) << slot;
  segment->free_bits &= ~bit;
  segment->alloc_bits |= bit;
  if (created) {
    h9_lsp_stats.malloc_page_create++;
  }
  void* ptr = h9_lsp_slot_ptr(segment, slot);
  h9_lsp_entry_token_insert(ptr, segment, slot);
  return ptr;
}

void* h9_lsp_debug_lasttoken_alloc(uint32_t class_id) {
  h9_lsp_init();
  h9_lsp_stats.malloc_call++;
  if (class_id >= H8_MEDIUM_CLASS_COUNT) {
    return NULL;
  }
  H9LspSegment* segment = h9_lsp_active[class_id];
  bool created = false;
  if (!segment || segment->free_bits == 0u) {
    segment = h9_lsp_create_segment(class_id);
    h9_lsp_active[class_id] = segment;
    created = segment != NULL;
  } else {
    h9_lsp_stats.malloc_tls_page_hit++;
  }
  if (!segment || segment->free_bits == 0u) {
    return NULL;
  }
  uint32_t slot = (uint32_t)__builtin_ctzll(segment->free_bits);
  uint64_t bit = UINT64_C(1) << slot;
  segment->free_bits &= ~bit;
  segment->alloc_bits |= bit;
  if (created) {
    h9_lsp_stats.malloc_page_create++;
  }
  void* ptr = h9_lsp_slot_ptr(segment, slot);
  h9_lsp_last_ptr = ptr;
  h9_lsp_last_segment = segment;
  h9_lsp_last_slot = slot;
  h9_lsp_last_generation = segment->generation;
  h9_lsp_last_live = 1u;
  return ptr;
}

bool h9_lsp_debug_free(void* ptr, bool* owned_out) {
  H9LspRouteResult route = h9_lsp_debug_route(ptr);
  if (owned_out) {
    *owned_out = route.kind != H8_ROUTE_MISS;
  }
  if (route.kind == H8_ROUTE_MISS) {
    h9_lsp_stats.free_miss++;
    return false;
  }
  if (route.kind != H8_ROUTE_VALID) {
    h9_lsp_stats.free_invalid_owned++;
    return false;
  }
  H9LspSegment* segment = (H9LspSegment*)route.segment;
  uint64_t bit = UINT64_C(1) << route.slot;
  segment->alloc_bits &= ~bit;
  segment->free_bits |= bit;
  h9_lsp_stats.free_same_owner_local++;
  return true;
}

bool h9_lsp_debug_ptrtoken_free(void* ptr, bool* owned_out) {
  H9LspEntryToken* token = h9_lsp_entry_token_lookup(ptr);
  if (token) {
    if (h9_lsp_entry_token_valid(token)) {
      H9LspSegment* segment = token->segment;
      uint64_t bit = UINT64_C(1) << token->slot;
      segment->alloc_bits &= ~bit;
      segment->free_bits |= bit;
      h9_lsp_entry_token_clear(token);
      if (owned_out) {
        *owned_out = true;
      }
      h9_lsp_stats.ptrtoken_free_fast++;
      h9_lsp_stats.free_same_owner_local++;
      return true;
    }
    h9_lsp_stats.ptrtoken_state_mismatch++;
    h9_lsp_entry_token_clear(token);
  }
  h9_lsp_stats.ptrtoken_free_fallback++;
  return h9_lsp_debug_free(ptr, owned_out);
}

static H9_LSP_COLD_NOINLINE bool h9_lsp_debug_lasttoken_free_fallback(
    void* ptr, bool* owned_out) {
  h9_lsp_stats.ptrtoken_free_fallback++;
  return h9_lsp_debug_free(ptr, owned_out);
}

static bool h9_lsp_lasttoken_allocated_hit(void* ptr, H9LspSegment** seg_out,
                                           uint32_t* slot_out) {
  H9LspSegment* segment = h9_lsp_last_segment;
  if (!h9_lsp_last_live || h9_lsp_last_ptr != ptr || !segment ||
      h9_lsp_last_generation != segment->generation ||
      h9_lsp_last_slot >= segment->slot_count) {
    return false;
  }
  uint64_t bit = UINT64_C(1) << h9_lsp_last_slot;
  if ((segment->alloc_bits & bit) == 0u || (segment->free_bits & bit) != 0u ||
      (segment->cache_bits & bit) != 0u ||
      (segment->pending_bits & bit) != 0u) {
    h9_lsp_last_live = 0u;
    h9_lsp_stats.ptrtoken_state_mismatch++;
    return false;
  }
  if (seg_out) {
    *seg_out = segment;
  }
  if (slot_out) {
    *slot_out = h9_lsp_last_slot;
  }
  return true;
}

bool h9_lsp_debug_lasttoken_free(void* ptr, bool* owned_out) {
  H9LspSegment* segment = NULL;
  uint32_t slot = 0u;
  if (h9_lsp_lasttoken_allocated_hit(ptr, &segment, &slot)) {
    uint64_t bit = UINT64_C(1) << slot;
    segment->alloc_bits &= ~bit;
    segment->free_bits |= bit;
    h9_lsp_last_live = 0u;
    if (owned_out) {
      *owned_out = true;
    }
    h9_lsp_stats.ptrtoken_free_fast++;
    h9_lsp_stats.free_same_owner_local++;
    return true;
  }
  return h9_lsp_debug_lasttoken_free_fallback(ptr, owned_out);
}

static H9_LSP_COLD_NOINLINE bool h9_lsp_debug_lasttoken_usable_fallback(
    void* ptr, size_t* usable_out, bool* owned_out) {
  h9_lsp_stats.ptrtoken_usable_fallback++;
  return h9_lsp_debug_usable_size(ptr, usable_out, owned_out);
}

bool h9_lsp_debug_lasttoken_usable_size(void* ptr, size_t* usable_out,
                                        bool* owned_out) {
  H9LspSegment* segment = NULL;
  if (h9_lsp_lasttoken_allocated_hit(ptr, &segment, NULL)) {
    if (owned_out) {
      *owned_out = true;
    }
    if (usable_out) {
      *usable_out = segment->slot_size;
    }
    h9_lsp_stats.ptrtoken_usable_fast++;
    return true;
  }
  return h9_lsp_debug_lasttoken_usable_fallback(ptr, usable_out, owned_out);
}

static H9_LSP_COLD_NOINLINE void* h9_lsp_debug_lasttoken_realloc_fallback(
    void* ptr, size_t size, bool* owned_out) {
  h9_lsp_stats.ptrtoken_realloc_fallback++;
  return h9_lsp_debug_realloc_in_place(ptr, size, owned_out);
}

void* h9_lsp_debug_lasttoken_realloc_in_place(void* ptr, size_t size,
                                              bool* owned_out) {
  H9LspSegment* segment = NULL;
  if (h9_lsp_lasttoken_allocated_hit(ptr, &segment, NULL) &&
      size <= segment->slot_size) {
    if (owned_out) {
      *owned_out = true;
    }
    h9_lsp_stats.ptrtoken_realloc_fast++;
    return ptr;
  }
  return h9_lsp_debug_lasttoken_realloc_fallback(ptr, size, owned_out);
}

bool h9_lsp_debug_free_direct_owned(void* ptr) {
  H9LspRouteResult route = h9_lsp_debug_route_direct_owned(ptr);
  if (route.kind != H8_ROUTE_VALID) {
    return false;
  }
  H9LspSegment* segment = (H9LspSegment*)route.segment;
  uint64_t bit = UINT64_C(1) << route.slot;
  segment->alloc_bits &= ~bit;
  segment->free_bits |= bit;
  h9_lsp_stats.free_same_owner_local++;
  return true;
}

bool h9_lsp_debug_free_known_slot(uint32_t class_id, uint32_t slot) {
  if (class_id >= H8_MEDIUM_CLASS_COUNT) {
    return false;
  }
  H9LspSegment* segment = h9_lsp_active[class_id];
  if (!segment || slot >= segment->slot_count) {
    return false;
  }
  uint64_t bit = UINT64_C(1) << slot;
  if ((segment->alloc_bits & bit) == 0u || (segment->free_bits & bit) != 0u ||
      (segment->cache_bits & bit) != 0u || (segment->pending_bits & bit) != 0u) {
    return false;
  }
  segment->alloc_bits &= ~bit;
  segment->free_bits |= bit;
  h9_lsp_stats.free_same_owner_local++;
  return true;
}

bool h9_lsp_debug_usable_size(void* ptr, size_t* usable_out,
                              bool* owned_out) {
  H9LspRouteResult route = h9_lsp_debug_route(ptr);
  if (owned_out) {
    *owned_out = route.kind != H8_ROUTE_MISS;
  }
  if (route.kind != H8_ROUTE_VALID) {
    return false;
  }
  if (usable_out) {
    *usable_out = route.slot_size;
  }
  h9_lsp_stats.usable_route_valid++;
  return true;
}

bool h9_lsp_debug_ptrtoken_usable_size(void* ptr, size_t* usable_out,
                                       bool* owned_out) {
  H9LspEntryToken* token = h9_lsp_entry_token_lookup(ptr);
  if (token && h9_lsp_entry_token_valid(token)) {
    if (owned_out) {
      *owned_out = true;
    }
    if (usable_out) {
      *usable_out = token->slot_size;
    }
    h9_lsp_stats.ptrtoken_usable_fast++;
    return true;
  }
  if (token) {
    h9_lsp_stats.ptrtoken_state_mismatch++;
    h9_lsp_entry_token_clear(token);
  }
  h9_lsp_stats.ptrtoken_usable_fallback++;
  return h9_lsp_debug_usable_size(ptr, usable_out, owned_out);
}

void* h9_lsp_debug_realloc_in_place(void* ptr, size_t size, bool* owned_out) {
  H9LspRouteResult route = h9_lsp_debug_route(ptr);
  if (owned_out) {
    *owned_out = route.kind != H8_ROUTE_MISS;
  }
  if (route.kind != H8_ROUTE_VALID || size > route.slot_size) {
    return NULL;
  }
  h9_lsp_stats.realloc_route_valid++;
  return ptr;
}

void* h9_lsp_debug_ptrtoken_realloc_in_place(void* ptr, size_t size,
                                             bool* owned_out) {
  H9LspEntryToken* token = h9_lsp_entry_token_lookup(ptr);
  if (token && h9_lsp_entry_token_valid(token) && size <= token->slot_size) {
    if (owned_out) {
      *owned_out = true;
    }
    h9_lsp_stats.ptrtoken_realloc_fast++;
    return ptr;
  }
  if (token && !h9_lsp_entry_token_valid(token)) {
    h9_lsp_stats.ptrtoken_state_mismatch++;
    h9_lsp_entry_token_clear(token);
  }
  h9_lsp_stats.ptrtoken_realloc_fallback++;
  return h9_lsp_debug_realloc_in_place(ptr, size, owned_out);
}

#include "h8_hz9_local_slab_route_leaf_bench.inc"

H9LspStats h9_lsp_debug_stats(void) {
  return h9_lsp_stats;
}

#endif
