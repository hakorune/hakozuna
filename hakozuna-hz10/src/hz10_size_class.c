#include "hz10_size_class.h"

/*
 * Default:
 * 16, 32, then (1.5x, 2x) pairs for every doubling from 32 up to 65536:
 * 48/64, 96/128, 192/256, 384/512, 768/1024, 1536/2048, 3072/4096,
 * 6144/8192, 12288/16384, 24576/32768, 49152/65536.
 *
 * HZ10ClassGranularity-L1 opt-in:
 * - 16, 32, 48 preserve the tiny band.
 * - 64..8192 uses quarter-step classes: 1.0/1.25/1.5/1.75 per octave.
 * - 8192..65536 keeps the old 1.5x/2x large band to avoid wasting
 *   single-quantum page tail on quarter classes.
 */
const uint32_t hz10_size_class_table[HZ10_CLASS_COUNT] = {
#if HZ10_ENABLE_FINE_SIZE_CLASSES
    16u,    32u,    48u,    64u,    80u,    96u,    112u,   128u,
    160u,   192u,   224u,   256u,   320u,   384u,   448u,   512u,
    640u,   768u,   896u,   1024u,  1280u,  1536u,  1792u,  2048u,
    2560u,  3072u,  3584u,  4096u,  5120u,  6144u,  7168u,  8192u,
    12288u, 16384u, 24576u, 32768u, 49152u, 65536u
#else
    16u,    32u,    48u,    64u,    96u,     128u,    192u,    256u,
    384u,   512u,   768u,   1024u,  1536u,   2048u,   3072u,   4096u,
    6144u,  8192u,  12288u, 16384u, 24576u,  32768u,  49152u,  65536u
#endif
};
