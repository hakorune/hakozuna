#include "hz6_owner.h"

int hz6_owner_is_alive(const Hz6OwnerRecord* record, Hz6OwnerToken token) {
  return record && record->state == HZ6_OWNER_ALIVE &&
         hz6_owner_equal(record->token, token);
}
