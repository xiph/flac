/* libFLAC - Free Lossless Audio Codec library
 * Copyright (C) 2001,2002  Josh Coalson
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA  02111-1307, USA.
 */

#ifndef FLAC__PROTECTED__STREAM_ENCODER_H
#define FLAC__PROTECTED__STREAM_ENCODER_H

#include "FLAC/stream_encoder.h"

typedef struct FLAC__StreamEncoderProtected {
	FLAC__StreamEncoderState state;
	FLAC__bool verify;
	FLAC__bool streamable_subset;
	FLAC__bool do_mid_side_stereo;
	FLAC__bool loose_mid_side_stereo;
	unsigned channels;
	unsigned bits_per_sample;
	unsigned sample_rate;
	unsigned blocksize;
	unsigned max_lpc_order;
	unsigned qlp_coeff_precision;
	FLAC__bool do_qlp_coeff_prec_search;
	FLAC__bool do_exhaustive_model_search;
	FLAC__bool do_escape_coding;
	unsigned min_residual_partition_order;
	unsigned max_residual_partition_order;
	unsigned rice_parameter_search_dist;
	FLAC__uint64 total_samples_estimate;
	FLAC__StreamMetadata **metadata;
	unsigned num_metadata_blocks;
} FLAC__StreamEncoderProtected;

#endif
