#pragma once

#include <stdint.h>

#define HZ3_TAG_KIND_NONE  0u
#define HZ3_TAG_KIND_SMALL 1u
#define HZ3_TAG_KIND_LARGE 2u

static inline uint16_t hz3_tag_make(uint8_t kind, uint16_t sc) {
    return (uint16_t)(((uint16_t)kind << 8) | (uint16_t)(sc + 1u));
}

static inline uint8_t hz3_tag_kind(uint16_t tag) {
    return (uint8_t)(tag >> 8);
}

static inline int hz3_tag_sc(uint16_t tag) {
    uint16_t v = (uint16_t)(tag & 0xFFu);
    return v ? (int)v - 1 : -1;
}

static inline uint16_t hz3_tag_make_small(int sc) {
    return hz3_tag_make(HZ3_TAG_KIND_SMALL, (uint16_t)sc);
}

static inline uint16_t hz3_tag_make_large(int sc) {
    return hz3_tag_make(HZ3_TAG_KIND_LARGE, (uint16_t)sc);
}
