#ifndef HZ10_SIZE_CLASS_H
#define HZ10_SIZE_CLASS_H

#include <stddef.h>
#include <stdint.h>

/*
 * Size-class table for the multi-class public entry (src/hz10_public_entry.h).
 * 13 classes, doubling 16B..64KiB (HZ10_PAGE_QUANTUM) -- covers both the
 * main_local0-style range (16..32768) and the medium_local0-style range
 * (4097..65536) this session's HZ8/HZ9/tcmalloc/mimalloc benches use, so a
 * future same-run comparison has real overlap to compare against.
 *
 * L0 scope: every class fits exactly one Box 1 quantum (slot_size *
 * slot_count == HZ10_PAGE_QUANTUM), matching every prior box's
 * single-quantum-per-page limit. Sizes over HZ10_PAGE_QUANTUM are not
 * supported by this box -- that would need spanning multiple quanta per
 * allocation, which is out of scope here.
 */

#define HZ10_CLASS_COUNT 13u

/* Returns the class index [0, HZ10_CLASS_COUNT) whose slot_size is the
 * smallest one >= size, or HZ10_CLASS_COUNT (invalid) if size is 0 or
 * larger than the biggest class (HZ10_PAGE_QUANTUM). */
uint32_t hz10_size_class_for(size_t size);

uint32_t hz10_size_class_slot_size(uint32_t class_id);
uint32_t hz10_size_class_slot_count(uint32_t class_id);

#endif
