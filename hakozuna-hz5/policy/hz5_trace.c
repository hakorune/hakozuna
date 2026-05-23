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

#endif
