#ifndef HZ6_SIZE_CLASS_H
#define HZ6_SIZE_CLASS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Hz6SizeClass {
  uint16_t id;
  size_t bytes;
} Hz6SizeClass;

Hz6SizeClass hz6_size_class_for_request(size_t size);

int hz6_size_class_valid(Hz6SizeClass size_class);

#ifdef __cplusplus
}
#endif

#endif
