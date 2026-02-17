// hz4_config.h - HZ4 Configuration Constants (Meta Header)
// Box Theory: すべての knob は #ifndef で上書き可能
//
// This is a meta-header that includes all config submodules.
// For modular builds, include individual headers directly.
//
#ifndef HZ4_CONFIG_H
#define HZ4_CONFIG_H

// ============================================================================
// Core Configuration (System/Arch/Global)
// ============================================================================
#include "hz4_config_core.h"

// ============================================================================
// Archived (NO-GO) Box Guards
// ============================================================================
#include "hz4_config_archived.h"

// ============================================================================
// Remote/MT Configuration
// ============================================================================
#include "hz4_config_remote.h"

// ============================================================================
// TCache/TLS Configuration
// ============================================================================
#include "hz4_config_tcache.h"

// ============================================================================
// Collect/Decommit/RSSReturn Configuration
// ============================================================================
#include "hz4_config_collect.h"

#endif // HZ4_CONFIG_H
