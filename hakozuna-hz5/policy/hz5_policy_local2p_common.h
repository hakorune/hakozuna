#ifndef HZ5_POLICY_LOCAL2P_COMMON_H
#define HZ5_POLICY_LOCAL2P_COMMON_H

/*
 * Cross-platform Local2P algorithm vocabulary.
 *
 * Platform backends still own their raw source, lock primitive, and memory
 * return policy. This header only names the shared route/cache objects so
 * Linux and Windows lanes stay comparable without forcing one implementation.
 */
#include "hz5_policy_local2p_cache.h"
#include "hz5_policy_local2p_layout.h"
#include "hz5_policy_local2p_remote.h"
#include "hz5_policy_local2p_state.h"

#endif
