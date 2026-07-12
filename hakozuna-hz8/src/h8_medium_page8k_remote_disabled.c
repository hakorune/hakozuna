#include "h8_medium_page8k_remote.h"

#include <string.h>

#if !defined(H8_MEDIUM_PAGE8K_REMOTE_L1)

H8Page8KRemoteOwner* h8_page8k_remote_owner_create(uint32_t token) {
  (void)token;
  return NULL;
}
void h8_page8k_remote_owner_destroy(H8Page8KRemoteOwner* owner) {
  (void)owner;
}
H8Page8KRemotePage* h8_page8k_remote_page_create(
    H8Page8KRemoteOwner* owner) {
  (void)owner;
  return NULL;
}
void* h8_page8k_remote_alloc(H8Page8KRemoteOwner* owner,
                             H8Page8KRemotePage* page) {
  (void)owner;
  (void)page;
  return NULL;
}
bool h8_page8k_remote_free(H8Page8KRemoteOwner* current_owner, void* ptr,
                           bool* owned_out) {
  (void)current_owner;
  (void)ptr;
  if (owned_out) *owned_out = false;
  return false;
}
size_t h8_page8k_remote_drain(H8Page8KRemoteOwner* owner) {
  (void)owner;
  return 0u;
}
bool h8_page8k_remote_quiescent(const H8Page8KRemotePage* page) {
  (void)page;
  return false;
}
H8Page8KRemoteStats h8_page8k_remote_stats(void) {
  H8Page8KRemoteStats out;
  memset(&out, 0, sizeof(out));
  return out;
}
void* h8_page8k_remote_malloc_current(size_t size) {
  (void)size;
  return NULL;
}
bool h8_page8k_remote_free_current(void* ptr, bool* owned_out) {
  (void)ptr;
  if (owned_out) *owned_out = false;
  return false;
}
size_t h8_page8k_remote_drain_all_control(void) { return 0u; }
void h8_page8k_remote_thread_shutdown(void) {}
bool h8_page8k_remote_owner_close(H8Page8KRemoteOwner* owner) {
  (void)owner;
  return false;
}
bool h8_page8k_remote_adopt_one(H8Page8KRemoteOwner* owner) {
  (void)owner;
  return false;
}

#endif
