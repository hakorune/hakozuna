#include "hz6_size_class.h"

#include "../include/hz6_config.h"

Hz6SizeClass hz6_size_class_for_request(size_t size) {
  Hz6SizeClass result;
  if (size <= 16) {
    result.id = 0;
    result.bytes = 16;
    return result;
  }
  if (size <= 64) {
    result.id = 1;
    result.bytes = 64;
    return result;
  }
  if (size <= 256) {
    result.id = 2;
    result.bytes = 256;
    return result;
  }
  if (size <= 1024) {
    result.id = 3;
    result.bytes = 1024;
    return result;
  }
  result.id = 4;
  result.bytes = 4096;
  return result;
}

int hz6_size_class_valid(Hz6SizeClass size_class) {
  return size_class.id < HZ6_FRONT_CACHE_CLASS_COUNT &&
         size_class.bytes != 0;
}
