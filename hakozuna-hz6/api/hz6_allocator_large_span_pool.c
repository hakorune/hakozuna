#include "hz6_allocator.h"

void hz6_allocator_large_span_pool_init(Hz6Allocator* allocator) {
  if (!allocator) {
    return;
  }

  allocator->large_span_pool.bytes_current = 0;
  for (size_t i = 0; i < HZ6_FRONT_CACHE_CLASS_COUNT; ++i) {
    allocator->large_span_pool.bins[i].count = 0;
    allocator->large_span_pool.bins[i].bytes_current = 0;
  }
}

size_t hz6_allocator_large_span_pool_capacity(uint16_t class_id) {
  (void)class_id;
  return HZ6_TRANSFER_CACHE_CAPACITY;
}

size_t hz6_allocator_large_span_pool_bytes_capacity(uint16_t class_id) {
  (void)class_id;
  return HZ6_LARGE_SPAN_CENTRAL_CLASS_BYTES_CAP;
}

size_t hz6_allocator_large_span_pool_global_bytes_capacity(void) {
  return HZ6_LARGE_SPAN_CENTRAL_GLOBAL_BYTES_CAP;
}

size_t hz6_allocator_large_span_pool_count(const Hz6Allocator* allocator,
                                           uint16_t class_id) {
  if (!allocator || class_id >= HZ6_FRONT_CACHE_CLASS_COUNT) {
    return 0;
  }
  return allocator->large_span_pool.bins[class_id].count;
}

size_t hz6_allocator_large_span_pool_bytes(const Hz6Allocator* allocator,
                                           uint16_t class_id) {
  if (!allocator || class_id >= HZ6_FRONT_CACHE_CLASS_COUNT) {
    return 0;
  }
  return allocator->large_span_pool.bins[class_id].bytes_current;
}

size_t hz6_allocator_large_span_pool_global_bytes(
    const Hz6Allocator* allocator) {
  return allocator ? allocator->large_span_pool.bytes_current : 0;
}

int hz6_allocator_large_span_pool_push(Hz6Allocator* allocator,
                                       Hz6ObjectDescriptor* descriptor) {
  if (!allocator || !descriptor ||
      descriptor->class_id >= HZ6_FRONT_CACHE_CLASS_COUNT ||
      descriptor->state != HZ6_STATE_CENTRAL_FREE ||
      descriptor->bytes == 0) {
    return 0;
  }

  Hz6LargeSpanPoolBin* bin =
      &allocator->large_span_pool.bins[descriptor->class_id];
  if (bin->count >= HZ6_TRANSFER_CACHE_CAPACITY) {
    return 0;
  }

  const size_t span_bytes = (size_t)descriptor->bytes;
  const size_t class_cap =
      hz6_allocator_large_span_pool_bytes_capacity(descriptor->class_id);
  const size_t global_cap =
      hz6_allocator_large_span_pool_global_bytes_capacity();
  if (span_bytes > class_cap || span_bytes > global_cap ||
      bin->bytes_current > class_cap - span_bytes ||
      allocator->large_span_pool.bytes_current > global_cap - span_bytes) {
    return 0;
  }

  bin->descriptors[bin->count++] = descriptor;
  bin->bytes_current += span_bytes;
  allocator->large_span_pool.bytes_current += span_bytes;
  hz6_allocator_note_large_span_central_push(allocator);
  return 1;
}

int hz6_allocator_large_direct_pool_push(Hz6Allocator* allocator,
                                         Hz6ObjectDescriptor* descriptor) {
  if (!allocator || !descriptor ||
      descriptor->class_id >= HZ6_FRONT_CACHE_CLASS_COUNT ||
      descriptor->state != HZ6_STATE_CENTRAL_FREE ||
      descriptor->bytes == 0) {
    return 0;
  }

  Hz6LargeSpanPoolBin* bin =
      &allocator->large_span_pool.bins[descriptor->class_id];
  if (bin->count >= HZ6_TRANSFER_CACHE_CAPACITY) {
    return 0;
  }

  const size_t span_bytes = (size_t)descriptor->bytes;
  const size_t cap = HZ6_LARGE_DIRECT_RETAIN_BYTES_CAP;
  if (span_bytes > cap || bin->bytes_current > cap - span_bytes ||
      allocator->large_span_pool.bytes_current > cap - span_bytes) {
    return 0;
  }

  bin->descriptors[bin->count++] = descriptor;
  bin->bytes_current += span_bytes;
  allocator->large_span_pool.bytes_current += span_bytes;
  hz6_allocator_note_large_span_central_push(allocator);
  return 1;
}

int hz6_allocator_large_span_pool_pop(Hz6Allocator* allocator,
                                      uint16_t class_id,
                                      Hz6ObjectDescriptor** out) {
  if (!allocator || !out || class_id >= HZ6_FRONT_CACHE_CLASS_COUNT) {
    return 0;
  }

  Hz6LargeSpanPoolBin* bin = &allocator->large_span_pool.bins[class_id];
  if (bin->count == 0) {
    return 0;
  }

  Hz6ObjectDescriptor* descriptor = bin->descriptors[--bin->count];
  bin->descriptors[bin->count] = NULL;
  const size_t span_bytes = descriptor ? (size_t)descriptor->bytes : 0;
  if (span_bytes != 0) {
    if (bin->bytes_current >= span_bytes) {
      bin->bytes_current -= span_bytes;
    } else {
      bin->bytes_current = 0;
    }
    if (allocator->large_span_pool.bytes_current >= span_bytes) {
      allocator->large_span_pool.bytes_current -= span_bytes;
    } else {
      allocator->large_span_pool.bytes_current = 0;
    }
  }
  *out = descriptor;
  hz6_allocator_note_large_span_central_pop(allocator);
  return 1;
}

int hz6_allocator_large_span_pool_pop_exact_bytes(
    Hz6Allocator* allocator,
    uint16_t class_id,
    size_t bytes,
    Hz6ObjectDescriptor** out) {
  if (!allocator || !out || bytes == 0 ||
      class_id >= HZ6_FRONT_CACHE_CLASS_COUNT) {
    return 0;
  }

  Hz6LargeSpanPoolBin* bin = &allocator->large_span_pool.bins[class_id];
  for (size_t i = bin->count; i > 0; --i) {
    size_t index = i - 1u;
    Hz6ObjectDescriptor* descriptor = bin->descriptors[index];
    if (!descriptor || (size_t)descriptor->bytes != bytes) {
      continue;
    }

    bin->descriptors[index] = bin->descriptors[--bin->count];
    bin->descriptors[bin->count] = NULL;
    if (bin->bytes_current >= bytes) {
      bin->bytes_current -= bytes;
    } else {
      bin->bytes_current = 0;
    }
    if (allocator->large_span_pool.bytes_current >= bytes) {
      allocator->large_span_pool.bytes_current -= bytes;
    } else {
      allocator->large_span_pool.bytes_current = 0;
    }
    *out = descriptor;
    hz6_allocator_note_large_span_central_pop(allocator);
    return 1;
  }

  return 0;
}
