#ifndef FLAC__PRIVATE__MACROS_H
#define FLAC__PRIVATE__MACROS_H

#if defined(__GNUC__)

#define flac_max(a,b) \
    ({ __typeof__ (a) _a = (a); \
    __typeof__ (b) _b = (b); \
    _a > _b ? _a : _b; })

#define flac_min(a,b) \
    ({ __typeof__ (a) _a = (a); \
    __typeof__ (b) _b = (b); \
    _a < _b ? _a : _b; })

/* Whatever other unix that has sys/param.h */
#elif defined(HAVE_SYS_PARAM_H)
#include <sys/param.h>
#define flac_max(a,b) MAX(a,b)
#define flac_min(a,b) MIN(a,b)

/* Windows VS has them in stdlib.h.. XXX:Untested */
#elif defined(_MSC_VER)
#include <stdlib.h>
#define flac_max(a,b) __max(a,b)
#define flac_min(a,b) __min(a,b)
#endif

#endif