#include "../include/h8.h"
#include "../src/h8_medium_page_backend.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

int main(void) {
  void* ptr = h8_malloc(8192u);
  if (!ptr) return 1;

  bool owned = false;
  bool interior = h8_medium_page_backend_free(
      (void*)((uintptr_t)ptr + 16u), &owned);
  if (interior || !owned) return 2;

  owned = false;
  bool valid = h8_medium_page_backend_free(ptr, &owned);
  if (!valid || !owned) return 3;

  owned = false;
  bool duplicate = h8_medium_page_backend_free(ptr, &owned);
  if (duplicate || !owned) return 4;

  printf("[H8_MEDIUM_PAGE_BACKEND_SMOKE] interior=invalid valid=freed "
         "duplicate=invalid\n");
  return 0;
}
