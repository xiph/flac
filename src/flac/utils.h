/* flac - Command-line FLAC encoder/decoder
 * Copyright (C) 2002,2003,2004  Josh Coalson
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

#ifndef flac__utils_h
#define flac__utils_h

#include "FLAC/ordinals.h"
#include <stdio.h> /* for FILE */

typedef struct {
	FLAC__bool is_relative; /* i.e. specification string started with + or - */
	FLAC__bool value_is_samples;
	union {
		double seconds;
		FLAC__int64 samples;
	} value;
} utils__SkipUntilSpecification;

#ifdef FLAC__VALGRIND_TESTING
size_t flac__utils_fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
#else
#define flac__utils_fwrite fwrite
#endif
FLAC__bool flac__utils_parse_skip_until_specification(const char *s, utils__SkipUntilSpecification *spec);
void flac__utils_canonicalize_skip_until_specification(utils__SkipUntilSpecification *spec, unsigned sample_rate);

#endif
