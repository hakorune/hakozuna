#ifndef HZ11_TOKEN_TABLE_H
#define HZ11_TOKEN_TABLE_H

#include <stddef.h>
#include <stdint.h>

#define HZ11_TOKEN_COUNT 1024u /* power of two */

typedef struct H11Token {
  void* ptr;
  uint8_t class_id;
} H11Token;

static inline size_t hz11_token_hash(const void* ptr) {
  uintptr_t x = (uintptr_t)ptr;
  x ^= x >> 33;
  x *= 0xff51afd7ed558ccdULL;
  x ^= x >> 33;
  return (size_t)(x & (HZ11_TOKEN_COUNT - 1u));
}

static inline void hz11_token_set(H11Token* tokens, void* ptr,
                                  uint8_t class_id) {
  size_t i = hz11_token_hash(ptr);
  tokens[i].ptr = ptr;
  tokens[i].class_id = class_id;
}

static inline int hz11_token_lookup(H11Token* tokens, void* ptr,
                                    uint8_t* class_id_out) {
  size_t i = hz11_token_hash(ptr);
  if (tokens[i].ptr == ptr) {
    *class_id_out = tokens[i].class_id;
    return 1;
  }
  return 0;
}

static inline void hz11_token_invalidate(H11Token* tokens, void* ptr) {
  size_t i = hz11_token_hash(ptr);
  if (tokens[i].ptr == ptr) {
    tokens[i].ptr = NULL;
  }
}

#endif /* HZ11_TOKEN_TABLE_H */
