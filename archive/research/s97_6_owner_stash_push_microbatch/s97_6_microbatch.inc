// Archived code: S97-6 OwnerStashPushMicroBatchBox (n==1 staging)
//
// This is a reference snapshot only. The feature is archived (NO-GO) and must
// not be re-enabled in mainline builds. See:
// - hakozuna/hz3/archive/research/s97_6_owner_stash_push_microbatch/README.md
//
// Intended insertion point (historical):
// - hakozuna/hz3/src/small_v2/hz3_small_v2_push_remote.inc
//
// Notes:
// - The best-known form used push_one() for staged n==1 flush to avoid paying
//   push_list() fixed costs when batching failed to materialize.

#if 0
// (Intentionally disabled; see README for the full design and outcome.)
#endif

