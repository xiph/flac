/* flac - Command-line FLAC encoder/decoder
 * Copyright (C) 2000,2001  Josh Coalson
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

#include "FLAC/ordinals.h"

int flac__encode_wav(FILE *infile, long infilesize, const char *infilename, const char *outfilename, const byte *lookahead, unsigned lookahead_length, bool verbose, uint64 skip, bool verify, bool lax, bool do_mid_side, bool loose_mid_side, bool do_exhaustive_model_search, bool do_qlp_coeff_prec_search, unsigned min_residual_partition_order, unsigned max_residual_partition_order, unsigned rice_parameter_search_dist, unsigned max_lpc_order, unsigned blocksize, unsigned qlp_coeff_precision, unsigned padding, char *requested_seek_points, int num_requested_seek_points);
int flac__encode_raw(FILE *infile, long infilesize, const char *infilename, const char *outfilename, const byte *lookahead, unsigned lookahead_length, bool verbose, uint64 skip, bool verify, bool lax, bool do_mid_side, bool loose_mid_side, bool do_exhaustive_model_search, bool do_qlp_coeff_prec_search, unsigned min_residual_partition_order, unsigned max_residual_partition_order, unsigned rice_parameter_search_dist, unsigned max_lpc_order, unsigned blocksize, unsigned qlp_coeff_precision, unsigned padding, char *requested_seek_points, int num_requested_seek_points, bool is_big_endian, bool is_unsigned_samples, unsigned channels, unsigned bps, unsigned sample_rate);

#endif
