/* grabbag - Convenience lib for various routines common to several tools
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

/* Convenience routines for manipulating files */

/* This .h cannot be included by itself; #include "share/grabbag.h" instead. */

#ifndef GRABAG__FILE_H
#define GRABAG__FILE_H

#include <sys/types.h> /* for off_t */
#include <stdio.h> /* for FILE */
#include "FLAC/ordinals.h"

#ifdef __cplusplus
extern "C" {
#endif

void grabbag__file_copy_metadata(const char *srcpath, const char *destpath);
off_t grabbag__file_get_filesize(const char *srcpath);
const char *grabbag__file_get_basename(const char *srcpath);

/* read_only == false means "make file writable by user"
 * read_only == true means "make file read-only for everyone"
 */
FLAC__bool grabbag__file_change_stats(const char *filename, FLAC__bool read_only);

/* attempts to make writable before unlinking */
FLAC__bool grabbag__file_remove_file(const char *filename);

/* these will forcibly set stdin/stdout to binary mode (for OSes that require it) */
FILE *grabbag__file_get_binary_stdin();
FILE *grabbag__file_get_binary_stdout();

#ifdef __cplusplus
}
#endif

#endif
