#ifndef HZ12_CURRENT_SPAN_INSTALL_H
#define HZ12_CURRENT_SPAN_INSTALL_H

#include "hz12_thread_cache.h"

int h12_current_span_install(H12ThreadCache* cache, uint8_t class_id,
                             void* span_base);

#endif /* HZ12_CURRENT_SPAN_INSTALL_H */
