#ifndef FLAC__SHARE__UTF8_H
#define FLAC__SHARE__UTF8_H

#if defined(unix) || defined(__CYGWIN__) || defined(__CYGWIN32__)
#define UTF8_API

#else

#ifdef UTF8_API_EXPORTS
#define	UTF8_API	_declspec(dllexport)
#else
#define UTF8_API	_declspec(dllimport)
#define __LIBNAME__ "utf8.lib"
#pragma comment(lib, __LIBNAME__)
#undef __LIBNAME__

#endif
#endif


/*
 * Convert a string between UTF-8 and the locale's charset.
 * Invalid bytes are replaced by '#', and characters that are
 * not available in the target encoding are replaced by '?'.
 *
 * If the locale's charset is not set explicitly then it is
 * obtained using nl_langinfo(CODESET), where available, the
 * environment variable CHARSET, or assumed to be US-ASCII.
 *
 * Return value of conversion functions:
 *
 *  -1 : memory allocation failed
 *   0 : data was converted exactly
 *   1 : valid data was converted approximately (using '?')
 *   2 : input was invalid (but still converted, using '#')
 *   3 : unknown encoding (but still converted, using '?')
 */

UTF8_API void convert_set_charset(const char *charset);

UTF8_API int utf8_encode(const char *from, char **to);
UTF8_API int utf8_decode(const char *from, char **to);

#endif
