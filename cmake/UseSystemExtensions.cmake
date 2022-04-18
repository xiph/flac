include(CheckCSourceCompiles)

check_c_source_compiles("
    #include <wchar.h>
    mbstate_t x;
    int main() { return 0; }"
    HAVE_MBSTATE)
if(NOT HAVE_MBSTATE)
    check_c_source_compiles("
        #define _XOPEN_SOURCE 500
        #include <wchar.h>
        mbstate_t x;
        int main() { return 0; }"
        DODEFINE_XOPEN_SOURCE)
endif()
check_c_source_compiles("
    #define __EXTENSIONS__ 1
    #include <stdio.h>
    #ifdef STDC_HEADERS
    # include <stdlib.h>
    # include <stddef.h>
    #endif
    #ifdef HAVE_STRING_H
    # include <string.h>
    #endif
    #ifdef HAVE_INTTYPES_H
    # include <inttypes.h>
    #endif
    #ifdef HAVE_STDINT_H
    # include <stdint.h>
    #endif
    int main() { return 0; }"
    DODEFINE_EXTENSIONS)

add_definitions(
    -D_DARWIN_C_SOURCE
    -D_POSIX_PTHREAD_SEMANTICS
    -D__STDC_WANT_IEC_60559_BFP_EXT__
    -D__STDC_WANT_IEC_60559_DFP_EXT__
    -D__STDC_WANT_IEC_60559_FUNCS_EXT__
    -D__STDC_WANT_IEC_60559_TYPES_EXT__
    -D__STDC_WANT_LIB_EXT2__
    -D__STDC_WANT_MATH_SPEC_FUNCS__
    -D_TANDEM_SOURCE)
