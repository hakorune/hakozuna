// hz4_inbox.c - HZ4 Inbox Box (global instance)
#include "hz4_config.h"

#if HZ4_REMOTE_INBOX

#include "hz4_types.h"

// Inbox head type definition (matches hz4_inbox.inc)
typedef struct {
    _Alignas(64) _Atomic(void*) head;
} hz4_inbox_head_t;

// Global inbox instance: owner x sc
hz4_inbox_head_t g_hz4_inbox[HZ4_NUM_SHARDS][HZ4_SC_MAX];

#endif  // HZ4_REMOTE_INBOX
