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

#ifndef flac__encode_h
#define flac__encode_h

#include "FLAC/metadata.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

typedef struct {
	FLAC__bool verbose;
	FLAC__uint64 skip;
	FLAC__bool verify;
#ifdef FLAC__HAS_OGG
	FLAC__bool use_ogg;
	FLAC__bool has_serial_number;
	long serial_number;
#endif
	FLAC__bool lax;
	FLAC__bool do_mid_side;
	FLAC__bool loose_mid_side;
	FLAC__bool do_exhaustive_model_search;
	FLAC__bool do_escape_coding;
	FLAC__bool do_qlp_coeff_prec_search;
	unsigned min_residual_partition_order;
	unsigned max_residual_partition_order;
	unsigned rice_parameter_search_dist;
	unsigned max_lpc_order;
	unsigned blocksize;
	unsigned qlp_coeff_precision;
	int padding;
	char *requested_seek_points;
	int num_requested_seek_points;

	/* options related to --replay-gain and --sector-align */
	FLAC__bool is_first_file;
	FLAC__bool is_last_file;
	FLAC__int32 **align_reservoir;
	unsigned *align_reservoir_samples;
	FLAC__bool replay_gain;
	FLAC__bool sector_align;

	FLAC__StreamMetadata *vorbis_comment;

	struct {
		FLAC__bool disable_constant_subframes;
		FLAC__bool disable_fixed_subframes;
		FLAC__bool disable_verbatim_subframes;
	} debug;
} encode_options_t;

typedef struct {
	encode_options_t common;
} wav_encode_options_t;

typedef struct {
	encode_options_t common;

	FLAC__bool is_big_endian;
	FLAC__bool is_unsigned_samples;
	unsigned channels;
	unsigned bps;
	unsigned sample_rate;
} raw_encode_options_t;

int flac__encode_aif(FILE *infile, long infilesize, const char *infilename, const char *outfilename, const FLAC__byte *lookahead, unsigned lookahead_length, wav_encode_options_t options);
int flac__encode_wav(FILE *infile, long infilesize, const char *infilename, const char *outfilename, const FLAC__byte *lookahead, unsigned lookahead_length, wav_encode_options_t options);
int flac__encode_raw(FILE *infile, long infilesize, const char *infilename, const char *outfilename, const FLAC__byte *lookahead, unsigned lookahead_length, raw_encode_options_t options);

#endif
