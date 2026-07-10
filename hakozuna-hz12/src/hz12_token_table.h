#ifndef HZ12_TOKEN_TABLE_H
#define HZ12_TOKEN_TABLE_H

#include <stddef.h>
#include <stdint.h>

#define HZ12_TOKEN_COUNT 1024u /* power of two */

typedef struct H12Token {
  void* ptr;
  uint8_t class_id;
} H12Token;

static inline size_t hz12_token_hash(const void* ptr) {
  uintptr_t x = (uintptr_t)ptr;
  x ^= x >> 33;
  x *= 0xff51afd7ed558ccdULL;
  x ^= x >> 33;
  return (size_t)(x & (HZ12_TOKEN_COUNT - 1u));
}

static inline void hz12_token_set(H12Token* tokens, void* ptr,
                                  uint8_t class_id) {
  size_t i = hz12_token_hash(ptr);
  tokens[i].ptr = ptr;
  tokens[i].class_id = class_id;
}

static inline int hz12_token_lookup(H12Token* tokens, void* ptr,
                                    uint8_t* class_id_out) {
  size_t i = hz12_token_hash(ptr);
  if (tokens[i].ptr == ptr) {
    *class_id_out = tokens[i].class_id;
    return 1;
  }
  return 0;
}

static inline void hz12_token_invalidate(H12Token* tokens, void* ptr) {
  size_t i = hz12_token_hash(ptr);
  if (tokens[i].ptr == ptr) {
    tokens[i].ptr = NULL;
  }
}

#endif /* HZ12_TOKEN_TABLE_H */
