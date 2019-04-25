check_include_file("libintl.h" HAVE_LIBINTL_H)
check_include_file("string.h" HAVE_STRING_H)

add_library(getopt STATIC getopt.c getopt1.c)
