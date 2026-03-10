#pragma once

#if !defined(_WIN32)
#error "ucontext shim is Windows-only"
#endif

typedef void ucontext_t;
