#ifndef HZ6_SCAVENGE_H
#define HZ6_SCAVENGE_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Hz6ScavengeBudget {
  size_t byte_limit;
  size_t bytes_released;
  size_t objects_released;
} Hz6ScavengeBudget;

void hz6_scavenge_budget_init(Hz6ScavengeBudget* budget,
                              size_t byte_limit);

int hz6_scavenge_can_release(const Hz6ScavengeBudget* budget,
                             size_t bytes);

int hz6_scavenge_account_release(Hz6ScavengeBudget* budget,
                                 size_t bytes);

size_t hz6_scavenge_remaining_bytes(const Hz6ScavengeBudget* budget);

#ifdef __cplusplus
}
#endif

#endif
