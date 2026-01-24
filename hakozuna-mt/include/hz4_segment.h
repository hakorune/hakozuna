// hz4_segment.h - HZ4 Segment Box
#ifndef HZ4_SEGMENT_H
#define HZ4_SEGMENT_H

#include <stdint.h>

#include "hz4_seg.h"

hz4_seg_t* hz4_seg_create(uint8_t owner);
void       hz4_seg_init_pages(hz4_seg_t* seg, uint8_t owner);

#endif // HZ4_SEGMENT_H
