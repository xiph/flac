/* flac - Command-line FLAC encoder/decoder
 * Copyright (C) 2000,2001,2002  Josh Coalson
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

#ifndef flac__decode_h
#define flac__decode_h

#include "analyze.h"
#include "utils.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

typedef struct {
	FLAC__bool verbose;
	FLAC__bool continue_through_decode_errors;
#ifdef FLAC__HAS_OGG
	FLAC__bool is_ogg;
	FLAC__bool use_first_serial_number;
	long serial_number;
#endif
	utils__SkipUntilSpecification skip_specification;
} decode_options_t;

typedef struct {
	decode_options_t common;
} wav_decode_options_t;

typedef struct {
	decode_options_t common;

	FLAC__bool is_big_endian;
	FLAC__bool is_unsigned_samples;
} raw_decode_options_t;

/* outfile == 0 => test only */
int flac__decode_wav(const char *infilename, const char *outfilename, FLAC__bool analysis_mode, analysis_options aopts, wav_decode_options_t options);
int flac__decode_raw(const char *infilename, const char *outfilename, FLAC__bool analysis_mode, analysis_options aopts, raw_decode_options_t options);

#endif
