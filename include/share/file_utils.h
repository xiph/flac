/* file_utils - Convenience lib for manipulating file stats
 * Copyright (C) 2002  Josh Coalson
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

/*
 * This wraps the gain_analysis lib, which is LGPL.  This wrapper
 * allows analysis of different input resolutions by automatically
 * scaling the input signal
 */

#ifndef FLAC__SHARE__FILE_UTILS_H
#define FLAC__SHARE__FILE_UTILS_H

#if defined(FLAC__NO_DLL) || defined(unix) || defined(__CYGWIN__) || defined(__CYGWIN32__)
#define FILE_UTILS_API

#else

#ifdef FILE_UTILS_API_EXPORTS
#define	FILE_UTILS_API	_declspec(dllexport)
#else
#define FILE_UTILS_API	_declspec(dllimport)
#define __LIBNAME__ "file_utils.lib"
#pragma comment(lib, __LIBNAME__)
#undef __LIBNAME__

#endif
#endif

#include <sys/types.h> /* for off_t */
#include <stdio.h> /* for FILE */
#include "FLAC/ordinals.h"

#ifdef __cplusplus
extern "C" {
#endif

FILE_UTILS_API void FLAC__file_utils_copy_metadata(const char *srcpath, const char *destpath);
FILE_UTILS_API off_t FLAC__file_utils_get_filesize(const char *srcpath);
FILE_UTILS_API const char *FLAC__file_utils_get_basename(const char *srcpath);

/* read_only == false means "make file writable by user"
 * read_only == true means "make file read-only for everyone"
 */
FILE_UTILS_API FLAC__bool FLAC__file_utils_change_stats(const char *filename, FLAC__bool read_only);

/* attempts to make writable before unlinking */
FILE_UTILS_API FLAC__bool FLAC__file_utils_remove_file(const char *filename);

/* these will forcibly set stdin/stdout to binary mode (for OSes that require it) */
FILE_UTILS_API FILE *FLAC__file_utils_get_binary_stdin();
FILE_UTILS_API FILE *FLAC__file_utils_get_binary_stdout();

#ifdef __cplusplus
}
#endif

#endif
