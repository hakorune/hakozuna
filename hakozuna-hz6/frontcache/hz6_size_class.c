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

Hz6SizeClass hz6_size_class_for_id(uint16_t class_id) {
  Hz6SizeClass result;
  result.id = class_id;
  switch (class_id) {
    case 0:
      result.bytes = 16;
      break;
    case 1:
      result.bytes = 64;
      break;
    case 2:
      result.bytes = 256;
      break;
    case 3:
      result.bytes = 1024;
      break;
    case 4:
      result.bytes = 4096;
      break;
    default:
      result.bytes = 0;
      break;
  }
  return result;
}

int hz6_size_class_valid(Hz6SizeClass size_class) {
  return size_class.id < HZ6_FRONT_CACHE_CLASS_COUNT &&
         size_class.bytes != 0;
}
