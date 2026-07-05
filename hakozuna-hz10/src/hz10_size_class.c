#include "hz10_size_class.h"

/*
 * 16, 32, then (1.5x, 2x) pairs for every doubling from 32 up to 65536:
 * 48/64, 96/128, 192/256, 384/512, 768/1024, 1536/2048, 3072/4096,
 * 6144/8192, 12288/16384, 24576/32768, 49152/65536 -- see hz10_size_class.h
 * for why the very first doubling (16..32) has no 1.5x member.
 */
const uint32_t hz10_size_class_table[HZ10_CLASS_COUNT] = {
    16u,    32u,    48u,    64u,    96u,     128u,    192u,    256u,
    384u,   512u,   768u,   1024u,  1536u,   2048u,   3072u,   4096u,
    6144u,  8192u,  12288u, 16384u, 24576u,  32768u,  49152u,  65536u};
