/* libFLAC - Free Lossless Audio Codec library
 * Copyright (C) 2000,2001  Josh Coalson
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

#ifndef FLAC__STREAM_ENCODER_H
#define FLAC__STREAM_ENCODER_H

#include "format.h"

typedef enum {
	FLAC__STREAM_ENCODER_OK = 0,
	FLAC__STREAM_ENCODER_INVALID_NUMBER_OF_CHANNELS,
	FLAC__STREAM_ENCODER_INVALID_BITS_PER_SAMPLE,
	FLAC__STREAM_ENCODER_INVALID_SAMPLE_RATE,
	FLAC__STREAM_ENCODER_INVALID_BLOCK_SIZE,
	FLAC__STREAM_ENCODER_INVALID_QLP_COEFF_PRECISION,
	FLAC__STREAM_ENCODER_MID_SIDE_CHANNELS_MISMATCH,
	FLAC__STREAM_ENCODER_MID_SIDE_SAMPLE_SIZE_MISMATCH,
	FLAC__STREAM_ENCODER_ILLEGAL_MID_SIDE_FORCE,
	FLAC__STREAM_ENCODER_BLOCK_SIZE_TOO_SMALL_FOR_LPC_ORDER,
	FLAC__STREAM_ENCODER_NOT_STREAMABLE,
	FLAC__STREAM_ENCODER_FRAMING_ERROR,
	FLAC__STREAM_ENCODER_INVALID_SEEK_TABLE,
	FLAC__STREAM_ENCODER_FATAL_ERROR_WHILE_ENCODING,
	FLAC__STREAM_ENCODER_FATAL_ERROR_WHILE_WRITING, /* that is, the write_callback returned an error */
	FLAC__STREAM_ENCODER_MEMORY_ALLOCATION_ERROR,
	FLAC__STREAM_ENCODER_ALREADY_INITIALIZED,
	FLAC__STREAM_ENCODER_UNINITIALIZED
} FLAC__StreamEncoderState;
extern const char *FLAC__StreamEncoderStateString[];

typedef enum {
	FLAC__STREAM_ENCODER_WRITE_OK = 0,
	FLAC__STREAM_ENCODER_WRITE_FATAL_ERROR
} FLAC__StreamEncoderWriteStatus;
extern const char *FLAC__StreamEncoderWriteStatusString[];

/***********************************************************************
 *
 * class FLAC__StreamEncoder
 *
 ***********************************************************************/

struct FLAC__StreamEncoderProtected;
struct FLAC__StreamEncoderPrivate;
typedef struct {
	struct FLAC__StreamEncoderProtected *protected;
	struct FLAC__StreamEncoderPrivate *private;
} FLAC__StreamEncoder;

/***********************************************************************
 *
 * Class constructor/destructor
 *
 ***********************************************************************/

FLAC__StreamEncoder *FLAC__stream_encoder_new();
void FLAC__stream_encoder_delete(FLAC__StreamEncoder *encoder);

/***********************************************************************
 *
 * Public class method prototypes
 *
 ***********************************************************************/

/*
 * Initialize the instance; should be called after construction and
 * before any other calls.  Will set and return the encoder state,
 * which will be FLAC__STREAM_ENCODER_OK if initialization succeeded.
 */
FLAC__StreamEncoderState FLAC__stream_encoder_init(
	FLAC__StreamEncoder *encoder,
	bool     streamable_subset,
	bool     do_mid_side_stereo,          /* 0 or 1; 1 only if channels==2 */
	bool     loose_mid_side_stereo,       /* 0 or 1; 1 only if channels==2 and do_mid_side_stereo==true */
	unsigned channels,                    /* must be <= FLAC__MAX_CHANNELS */
	unsigned bits_per_sample,             /* do not give the encoder wider data than what you specify here or bad things will happen! */
	unsigned sample_rate,
	unsigned blocksize,
	unsigned max_lpc_order,               /* 0 => encoder will not try general LPC, only fixed predictors; must be <= FLAC__MAX_LPC_ORDER */
	unsigned qlp_coeff_precision,         /* >= FLAC__MIN_QLP_COEFF_PRECISION, or 0 to let encoder select based on blocksize; */
	                                      /* qlp_coeff_precision+bits_per_sample must be < 32 */
	bool     do_qlp_coeff_prec_search,    /* 0 => use qlp_coeff_precision, 1 => search around qlp_coeff_precision, take best */
	bool     do_exhaustive_model_search,  /* 0 => use estimated bits per residual for scoring, 1 => generate all, take shortest */
	unsigned min_residual_partition_order, /* 0 => estimate Rice parameter based on residual variance; >0 => partition residual, use parameter for each */
	unsigned max_residual_partition_order, /*      based on mean; min_ and max_ specify the min and max Rice partition order */
	unsigned rice_parameter_search_dist,  /* 0 => try only calc'd parameter k; else try all [k-dist..k+dist] parameters, use best */
	uint64   total_samples_estimate,      /* may be 0 if unknown.  this will be a placeholder in the metadata block until the actual total is calculated */
	const FLAC__StreamMetaData_SeekTable *seek_table, /* optional seek_table to prepend, 0 => no seek table */
	unsigned padding,                     /* size of PADDING block to add (goes after seek table); 0 => do not add a PADDING block */
	FLAC__StreamEncoderWriteStatus (*write_callback)(const FLAC__StreamEncoder *encoder, const byte buffer[], unsigned bytes, unsigned samples, unsigned current_frame, void *client_data),
	void (*metadata_callback)(const FLAC__StreamEncoder *encoder, const FLAC__StreamMetaData *metadata, void *client_data),
	void *client_data
);
void FLAC__stream_encoder_finish(FLAC__StreamEncoder *encoder);

/*
 * various "get" methods
 */
FLAC__StreamEncoderState FLAC__stream_encoder_state(const FLAC__StreamEncoder *encoder);
bool     FLAC__stream_encoder_streamable_subset(const FLAC__StreamEncoder *encoder);
bool     FLAC__stream_encoder_do_mid_side_stereo(const FLAC__StreamEncoder *encoder);
bool     FLAC__stream_encoder_loose_mid_side_stereo(const FLAC__StreamEncoder *encoder);
unsigned FLAC__stream_encoder_channels(const FLAC__StreamEncoder *encoder);
unsigned FLAC__stream_encoder_bits_per_sample(const FLAC__StreamEncoder *encoder);
unsigned FLAC__stream_encoder_sample_rate(const FLAC__StreamEncoder *encoder);
unsigned FLAC__stream_encoder_blocksize(const FLAC__StreamEncoder *encoder);
unsigned FLAC__stream_encoder_max_lpc_order(const FLAC__StreamEncoder *encoder);
unsigned FLAC__stream_encoder_qlp_coeff_precision(const FLAC__StreamEncoder *encoder);
bool     FLAC__stream_encoder_do_qlp_coeff_prec_search(const FLAC__StreamEncoder *encoder);
bool     FLAC__stream_encoder_do_exhaustive_model_search(const FLAC__StreamEncoder *encoder);
unsigned FLAC__stream_encoder_min_residual_partition_order(const FLAC__StreamEncoder *encoder);
unsigned FLAC__stream_encoder_max_residual_partition_order(const FLAC__StreamEncoder *encoder);
unsigned FLAC__stream_encoder_rice_parameter_search_dist(const FLAC__StreamEncoder *encoder);

/*
 * methods for encoding the data
 */
bool FLAC__stream_encoder_process(FLAC__StreamEncoder *encoder, const int32 *buf[], unsigned samples);
bool FLAC__stream_encoder_process_interleaved(FLAC__StreamEncoder *encoder, const int32 buf[], unsigned samples);

#endif
