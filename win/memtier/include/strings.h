#pragma once

#if !defined(_WIN32)
#error "strings shim is Windows-only"
#endif

#include <string.h>

#ifndef strcasecmp
#define strcasecmp _stricmp
#endif

#ifndef strncasecmp
#define strncasecmp _strnicmp
#endif
