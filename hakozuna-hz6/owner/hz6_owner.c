#include "hz6_owner.h"

int hz6_owner_equal(Hz6OwnerToken a, Hz6OwnerToken b) {
  return a.slot == b.slot && a.generation == b.generation;
}

int hz6_owner_is_alive(const Hz6OwnerRecord* record, Hz6OwnerToken token) {
  return record && record->state == HZ6_OWNER_ALIVE &&
         hz6_owner_equal(record->token, token);
}
