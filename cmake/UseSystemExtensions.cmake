cmake_minimum_required(VERSION 3.12)

include(CheckCSourceCompiles)

check_c_source_compiles("
    int main()
    {
    #ifndef _FORTIFY_SOURCE
        return 0;
    #else
        this_is_an_error;
    #endif
    }"
    DODEFINE_FORTIFY_SOURCE)
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
    #ifdef HAVE_SYS_TYPES_H
    # include <sys/types.h>
    #endif
    #ifdef HAVE_SYS_STAT_H
    # include <sys/stat.h>
    #endif
    #ifdef STDC_HEADERS
    # include <stdlib.h>
    # include <stddef.h>
    #else
    # ifdef HAVE_STDLIB_H
    #  include <stdlib.h>
    # endif
    #endif
    #ifdef HAVE_STRING_H
    # if !defined STDC_HEADERS && defined HAVE_MEMORY_H
    #  include <memory.h>
    # endif
    # include <string.h>
    #endif
    #ifdef HAVE_STRINGS_H
    # include <strings.h>
    #endif
    #ifdef HAVE_INTTYPES_H
    # include <inttypes.h>
    #endif
    #ifdef HAVE_STDINT_H
    # include <stdint.h>
    #endif
    #ifdef HAVE_UNISTD_H
    # include <unistd.h>
    #endif
    int main() { return 0; }"
    DODEFINE_EXTENSIONS)

add_compile_definitions(
    _ALL_SOURCE
    _DARWIN_C_SOURCE
    _GNU_SOURCE
    _POSIX_PTHREAD_SEMANTICS
    __STDC_WANT_IEC_60559_ATTRIBS_EXT__
    __STDC_WANT_IEC_60559_BFP_EXT__
    __STDC_WANT_IEC_60559_DFP_EXT__
    __STDC_WANT_IEC_60559_FUNCS_EXT__
    __STDC_WANT_IEC_60559_TYPES_EXT__
    __STDC_WANT_LIB_EXT2__
    __STDC_WANT_MATH_SPEC_FUNCS__
    _TANDEM_SOURCE
    $<$<AND:$<BOOL:${DODEFINE_FORTIFY_SOURCE}>,$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>>:_FORTIFY_SOURCE=2>
    $<$<BOOL:${DODEFINE_XOPEN_SOURCE}>:_XOPEN_SOURCE=500>
    $<$<BOOL:${DODEFINE_EXTENTIONS}>:__EXTENSIONS__>)
