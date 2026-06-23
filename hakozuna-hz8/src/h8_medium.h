#ifndef H8_MEDIUM_H
#define H8_MEDIUM_H

#include "../include/h8.h"
#include "h8_class_map.h"

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <pthread.h>
#include <stdint.h>

#define H8_MEDIUM_MIN_SIZE (H8_MAX_SMALL_SIZE + 1u)
#define H8_MEDIUM_MAX_SIZE 65536u
#define H8_MEDIUM_RUN_BYTES 65536u
#define H8_MEDIUM_PAGE_BYTES 4096u
#define H8_MEDIUM_CLASS_COUNT 4u

typedef enum H8MediumRunState {
  H8_MEDIUM_RUN_UNUSED = 0,
  H8_MEDIUM_RUN_ACTIVE = 1,
  H8_MEDIUM_RUN_RETIRING = 2,
  H8_MEDIUM_RUN_RETIRED = 3
} H8MediumRunState;

typedef struct H8MediumClassSpec {
  uint32_t slot_size;
  uint32_t run_size;
  uint16_t slot_count;
  uint16_t bitmap_words;
} H8MediumClassSpec;

typedef struct H8MediumRun {
  uint8_t* base;
  uint16_t class_id;
  uint16_t slot_count;
  uint32_t slot_size;
  uint32_t run_size;
  _Atomic uint64_t owner_word;
  _Atomic uint8_t state;
  _Atomic uint64_t pending_word_mask;
  _Atomic uint64_t* pending_bits;
  _Atomic uint32_t* slot_state;
  pthread_mutex_t lock;
  uint64_t free_mask;
  uint64_t allocated_mask;
  void* meta_alloc_base;
  struct H8MediumRun* next_owner;
  struct H8MediumRun* next_global;
} H8MediumRun;

typedef struct H8OwnerRecord H8OwnerRecord;

bool h8_medium_size_supported(size_t size);
uint32_t h8_medium_class_for_size(size_t size);
const H8MediumClassSpec* h8_medium_class_spec(uint32_t class_id);
uint32_t h8_medium_rounded_size(size_t size);
bool h8_medium_slot_index_from_ptr_checked(const H8MediumRun* run,
                                           const void* ptr,
                                           size_t* slot_out);
void* h8_medium_slot_ptr(const H8MediumRun* run, size_t slot);
H8MediumRun* h8_medium_run_create_scaffold(uint32_t class_id);
void h8_medium_run_destroy_scaffold(H8MediumRun* run);
void* h8_medium_run_alloc_local_scaffold(H8MediumRun* run);
bool h8_medium_run_free_local_scaffold(H8MediumRun* run, void* ptr);
void* h8_medium_malloc_inner(size_t size);
bool h8_medium_free_inner(void* ptr, bool* owned_out);
H8RouteKind h8_medium_route_inner(void* ptr);
void h8_medium_owner_detach_all(H8OwnerRecord* owner);

#endif
