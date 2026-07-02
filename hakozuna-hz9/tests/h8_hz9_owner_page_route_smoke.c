#include "../src/h8_internal.h"

#include <stdio.h>

#if !defined(H9_OWNER_LOCAL_PAGE_POOL_ROUTE_SMOKE)
#error "owner page route smoke requires H9_OWNER_LOCAL_PAGE_POOL_ROUTE_SMOKE"
#endif

int main(void) {
  if (!h9_owner_page_route_smoke_run()) {
    fprintf(stderr, "owner page route smoke failed\n");
    return 1;
  }
  return 0;
}
