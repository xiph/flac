cmake_minimum_required(VERSION 3.12)

check_include_file("libintl.h" HAVE_LIBINTL_H)
check_include_file("string.h" HAVE_STRING_H)

add_compile_definitions(
    $<$<BOOL:${HAVE_LIBINTL_H}>:HAVE_LIBINTL_H>
    $<$<BOOL:${HAVE_STRING_H}>:HAVE_STRING_H>)

add_library(getopt STATIC
    getopt.c
    getopt1.c)
