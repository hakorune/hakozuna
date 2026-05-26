#include "hz5_trace.h"

#if BENCHLAB_HZ5_TRACE_LANE

#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>

static _Atomic(uint64_t) g_hz5_trace[HZ5_TRACE_COUNT];
static atomic_flag g_hz5_trace_registered = ATOMIC_FLAG_INIT;
static atomic_flag g_hz5_trace_printed = ATOMIC_FLAG_INIT;

static const char* hz5_trace_name(Hz5TraceCounter counter) {
  switch (counter) {
    case HZ5_TRACE_ALLOC_P25_BRIDGE:
      return "alloc_p25_bridge";
    case HZ5_TRACE_ALLOC_P43_SOURCE_TLS:
      return "alloc_p43_source_tls";
    case HZ5_TRACE_ALLOC_P43_SOURCE_COMMITTED:
      return "alloc_p43_source_committed";
    case HZ5_TRACE_ALLOC_P43_SOURCE_RELEASE_BUFFER:
      return "alloc_p43_source_release_buffer";
    case HZ5_TRACE_ALLOC_P43_SOURCE_COLD:
      return "alloc_p43_source_cold";
    case HZ5_TRACE_ALLOC_P43_SOURCE_FREE_SLOT:
      return "alloc_p43_source_free_slot";
    case HZ5_TRACE_ALLOC_P43_SOURCE_NEW_SEGMENT:
      return "alloc_p43_source_new_segment";
    case HZ5_TRACE_ALLOC_P43_TOKEN:
      return "alloc_p43_token";
    case HZ5_TRACE_FREE_P25_BRIDGE:
      return "free_p25_bridge";
    case HZ5_TRACE_FREE_P43_LOOKUP_PREPARED:
      return "free_p43_lookup_prepared";
    case HZ5_TRACE_FREE_P43_TOKEN_DIRECT:
      return "free_p43_token_direct";
    case HZ5_TRACE_FREE_P43_TOKEN_BRIDGE:
      return "free_p43_token_bridge";
    case HZ5_TRACE_FREE_TRUSTWRAP:
      return "free_trustwrap";
    case HZ5_TRACE_FREE_P25_BRIDGE_ATTR:
      return "free_p25_bridge_attr";
    case HZ5_TRACE_FREE_RAWLOOKUP:
      return "free_rawlookup";
    case HZ5_TRACE_FREE_FALLBACK_OR_INVALID:
      return "free_fallback_or_invalid";
    case HZ5_TRACE_WRAPPER_DECODE_OK:
      return "wrapper_decode_ok";
    case HZ5_TRACE_WRAPPER_DECODE_MISS:
      return "wrapper_decode_miss";
    case HZ5_TRACE_WRAPPER_TOKEN_VALID:
      return "wrapper_token_valid";
    case HZ5_TRACE_WRAPPER_TOKEN_INVALID:
      return "wrapper_token_invalid";
    case HZ5_TRACE_BRIDGE_ATTR_VALID:
      return "bridge_attr_valid";
    case HZ5_TRACE_BRIDGE_ATTR_INVALID:
      return "bridge_attr_invalid";
    case HZ5_TRACE_BRIDGE_ATTR_CAS_FAIL:
      return "bridge_attr_cas_fail";
    case HZ5_TRACE_ALLOC_LOCAL2P_TLS_HIT:
      return "alloc_local2p_tls_hit";
    case HZ5_TRACE_ALLOC_LOCAL2P_INBOX_HIT:
      return "alloc_local2p_inbox_hit";
    case HZ5_TRACE_ALLOC_LOCAL2P_GLOBAL_HIT:
      return "alloc_local2p_global_hit";
    case HZ5_TRACE_ALLOC_LOCAL2P_OS:
      return "alloc_local2p_os";
    case HZ5_TRACE_ALLOC_LOCAL2P_ESCAPE:
      return "alloc_local2p_escape";
    case HZ5_TRACE_ALLOC_LOCAL2P_EXACT_HIT:
      return "alloc_local2p_exact_hit";
    case HZ5_TRACE_ALLOC_LOCAL2P_INBOX_FAST_ACTIVATE:
      return "alloc_local2p_inbox_fast_activate";
    case HZ5_TRACE_ALLOC_LOCAL2P_ESCAPE_SIZE:
      return "alloc_local2p_escape_size";
    case HZ5_TRACE_ALLOC_LOCAL2P_ESCAPE_ALIGN:
      return "alloc_local2p_escape_align";
    case HZ5_TRACE_ALLOC_LOCAL2P_ESCAPE_GUARD:
      return "alloc_local2p_escape_guard";
    case HZ5_TRACE_FREE_LOCAL2P_TLS:
      return "free_local2p_tls";
    case HZ5_TRACE_FREE_LOCAL2P_INBOX:
      return "free_local2p_inbox";
    case HZ5_TRACE_FREE_LOCAL2P_GLOBAL:
      return "free_local2p_global";
    case HZ5_TRACE_FREE_LOCAL2P_REMOTE:
      return "free_local2p_remote";
    case HZ5_TRACE_FREE_LOCAL2P_INVALID_COOKIE:
      return "free_local2p_invalid_cookie";
    case HZ5_TRACE_FREE_LOCAL2P_INVALID_STATE:
      return "free_local2p_invalid_state";
    case HZ5_TRACE_FREE_LOCAL2P_OVERFLOW:
      return "free_local2p_overflow";
    case HZ5_TRACE_ALLOC_LOCAL2P_TLS_FAST_RETURN:
      return "alloc_local2p_tls_fast_return";
    case HZ5_TRACE_ALLOC_LOCAL2P_INBOX_CACHE_HIT:
      return "alloc_local2p_inbox_cache_hit";
    case HZ5_TRACE_ALLOC_LOCAL2P_INBOX_EXCHANGE:
      return "alloc_local2p_inbox_exchange";
    case HZ5_TRACE_ALLOC_LOCAL2P_HEADER_INIT:
      return "alloc_local2p_header_init";
    case HZ5_TRACE_FREE_LOCAL2P_SAME_OWNER_FAST:
      return "free_local2p_same_owner_fast";
    case HZ5_TRACE_FREE_LOCAL2P_REMOTE_BATCH_ENQUEUE:
      return "free_local2p_remote_batch_enqueue";
    case HZ5_TRACE_FREE_LOCAL2P_REMOTE_BATCH_FLUSH:
      return "free_local2p_remote_batch_flush";
    case HZ5_TRACE_FREE_LOCAL2P_REMOTE_BATCH_FLUSH_OWNER:
      return "free_local2p_remote_batch_flush_owner";
    case HZ5_TRACE_FREE_LOCAL2P_REMOTE_BATCH_FLUSH_CAP:
      return "free_local2p_remote_batch_flush_cap";
    case HZ5_TRACE_FREE_LOCAL2P_REMOTE_BATCH_FLUSH_SWITCH:
      return "free_local2p_remote_batch_flush_switch";
    case HZ5_TRACE_FREE_LOCAL2P_REMOTE_BATCH_FLUSH_NODES:
      return "free_local2p_remote_batch_flush_nodes";
    case HZ5_TRACE_LOCAL2P_REMOTE_OWNER_SWITCH:
      return "local2p_remote_owner_switch";
    case HZ5_TRACE_LOCAL2P_REMOTE_OWNER_GATE_SKIP:
      return "local2p_remote_owner_gate_skip";
    case HZ5_TRACE_LOCAL2P_REMOTE_DIRECT_INDEX_HIT:
      return "local2p_remote_direct_index_hit";
    case HZ5_TRACE_LOCAL2P_REMOTE_DIRECT_INDEX_COLLISION:
      return "local2p_remote_direct_index_collision";
    case HZ5_TRACE_LOCAL2P_REMOTE_ASSOC_LRU_VICTIM:
      return "local2p_remote_assoc_lru_victim";
    case HZ5_TRACE_LOCAL2P_REMOTE_ACTIVE_SLOTS_MAX:
      return "local2p_remote_active_slots_max";
    case HZ5_TRACE_LOCAL2P_REMOTE_BATCH_DEPTH_MAX:
      return "local2p_remote_batch_depth_max";
    case HZ5_TRACE_LOCAL2P_GLOBAL_POP_LOCK:
      return "local2p_global_pop_lock";
    case HZ5_TRACE_LOCAL2P_GLOBAL_PUSH_LOCK:
      return "local2p_global_push_lock";
    case HZ5_TRACE_LOCAL2P_ROUTE_COOKIE_OK:
      return "local2p_route_cookie_ok";
    case HZ5_TRACE_LOCAL2P_ROUTE_COOKIE_MISS:
      return "local2p_route_cookie_miss";
    case HZ5_TRACE_P20_CRT_BYPASS_ALLOC:
      return "p20_crt_bypass_alloc";
    case HZ5_TRACE_P20_CRT_BYPASS_FREE:
      return "p20_crt_bypass_free";
    case HZ5_TRACE_P20_CRT_BYPASS_ALLOC_BYTES:
      return "p20_crt_bypass_alloc_bytes";
    case HZ5_TRACE_P20_CRT_BYPASS_FREE_BYTES:
      return "p20_crt_bypass_free_bytes";
    case HZ5_TRACE_LOCAL2P_OS_ALLOC_BYTES:
      return "local2p_os_alloc_bytes";
    case HZ5_TRACE_LOCAL2P_OS_FREE_BYTES:
      return "local2p_os_free_bytes";
    case HZ5_TRACE_COUNT:
      break;
  }
  return "unknown";
}

void hz5_trace_print_once(void) {
  if (atomic_flag_test_and_set_explicit(&g_hz5_trace_printed,
                                        memory_order_acq_rel)) {
    return;
  }
  fprintf(stderr, "[HZ5_TRACE]");
  for (uint32_t i = 0; i < (uint32_t)HZ5_TRACE_COUNT; ++i) {
    uint64_t value =
        atomic_load_explicit(&g_hz5_trace[i], memory_order_relaxed);
    if (value != 0) {
      fprintf(stderr, " %s=%llu", hz5_trace_name((Hz5TraceCounter)i),
              (unsigned long long)value);
    }
  }
  fputc('\n', stderr);
}

void hz5_trace_register_once(void) {
  if (!atomic_flag_test_and_set_explicit(&g_hz5_trace_registered,
                                         memory_order_acq_rel)) {
    atexit(hz5_trace_print_once);
  }
}

void hz5_trace_inc(Hz5TraceCounter counter) {
  hz5_trace_add(counter, 1u);
}

void hz5_trace_add(Hz5TraceCounter counter, uint64_t value) {
  if (counter >= HZ5_TRACE_COUNT) {
    return;
  }
  atomic_fetch_add_explicit(&g_hz5_trace[counter], value,
                            memory_order_relaxed);
}

void hz5_trace_max(Hz5TraceCounter counter, uint64_t value) {
  if (counter >= HZ5_TRACE_COUNT) {
    return;
  }
  uint64_t observed =
      atomic_load_explicit(&g_hz5_trace[counter], memory_order_relaxed);
  while (observed < value &&
         !atomic_compare_exchange_weak_explicit(
             &g_hz5_trace[counter], &observed, value, memory_order_relaxed,
             memory_order_relaxed)) {
  }
}

#endif
