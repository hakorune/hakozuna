#ifndef HZ6_FRONT_H
#define HZ6_FRONT_H

#include "../include/hz6_contract.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Hz6Allocator Hz6Allocator;

typedef struct Hz6FrontOps {
  uint16_t front_id;
  const char* name;
  int (*can_allocate)(size_t size, size_t align, uint16_t* class_id);
  void* (*alloc)(Hz6Allocator* allocator, uint16_t class_id, size_t size);
  int (*free_tagged)(Hz6Allocator* allocator, void* ptr, Hz6RouteResult route);
} Hz6FrontOps;

#ifdef __cplusplus
}
#endif

#endif
