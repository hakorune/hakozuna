#include "hz6_scavenge.h"

void hz6_scavenge_budget_init(Hz6ScavengeBudget* budget,
                              size_t byte_limit) {
  if (!budget) {
    return;
  }
  budget->byte_limit = byte_limit;
  budget->bytes_released = 0;
  budget->objects_released = 0;
}

int hz6_scavenge_can_release(const Hz6ScavengeBudget* budget,
                             size_t bytes) {
  if (!budget || bytes == 0) {
    return 0;
  }
  if (budget->bytes_released > budget->byte_limit) {
    return 0;
  }
  return bytes <= budget->byte_limit - budget->bytes_released;
}

int hz6_scavenge_account_release(Hz6ScavengeBudget* budget,
                                 size_t bytes) {
  if (!hz6_scavenge_can_release(budget, bytes)) {
    return 0;
  }
  budget->bytes_released += bytes;
  ++budget->objects_released;
  return 1;
}

size_t hz6_scavenge_remaining_bytes(const Hz6ScavengeBudget* budget) {
  if (!budget || budget->bytes_released >= budget->byte_limit) {
    return 0;
  }
  return budget->byte_limit - budget->bytes_released;
}
