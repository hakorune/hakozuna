#if HZ3_LARGE_ALIGNED_OBS
#include "hz3_large_cache_policy_aligned_base.inc"
#include "hz3_large_cache_policy_residual_obs.inc"
#include "hz3_large_cache_policy_activation_obs.inc"
#include "hz3_large_cache_policy_free_route_obs.inc"
#include "hz3_large_cache_policy_aligned_runtime.inc"
#else
#include "hz3_large_cache_policy_aligned_fallback.inc"
#endif
