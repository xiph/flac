/* libFLAC - Free Lossless Audio Coder library
 * Copyright (C) 2000  Josh Coalson
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

#ifndef FLAC__ENCODER_H
#define FLAC__ENCODER_H

#include "format.h"

typedef enum {
	FLAC__ENCODER_WRITE_OK = 0,
	FLAC__ENCODER_WRITE_FATAL_ERROR = 1
} FLAC__EncoderWriteStatus;

typedef enum {
	FLAC__ENCODER_OK,
	FLAC__ENCODER_UNINITIALIZED,
	FLAC__ENCODER_INVALID_NUMBER_OF_CHANNELS,
	FLAC__ENCODER_INVALID_BITS_PER_SAMPLE,
	FLAC__ENCODER_INVALID_SAMPLE_RATE,
	FLAC__ENCODER_INVALID_BLOCK_SIZE,
	FLAC__ENCODER_INVALID_QLP_COEFF_PRECISION,
	FLAC__ENCODER_MID_SIDE_CHANNELS_MISMATCH,
	FLAC__ENCODER_MID_SIDE_SAMPLE_SIZE_MISMATCH,
	FLAC__ENCODER_BLOCK_SIZE_TOO_SMALL_FOR_LPC_ORDER,
	FLAC__ENCODER_NOT_STREAMABLE,
	FLAC__ENCODER_FRAMING_ERROR,
	FLAC__ENCODER_FATAL_ERROR_WHILE_ENCODING,
	FLAC__ENCODER_FATAL_ERROR_WHILE_WRITING, /* that is, the write_callback returned an error */
	FLAC__ENCODER_MEMORY_ALLOCATION_ERROR
} FLAC__EncoderState;

struct FLAC__EncoderPrivate;
typedef struct {
	/*
	 * none of these fields may change once FLAC__encoder_init() is called
	 */
	struct FLAC__EncoderPrivate *guts;    /* must be 0 when passed to FLAC__encoder_init() */
	FLAC__EncoderState state;             /* must be FLAC__ENCODER_UNINITIALIZED when passed to FLAC__encoder_init() */
	bool     streamable_subset;
	bool     do_mid_side_stereo;          /* 0 or 1; 1 only if channels==2 */
	unsigned channels;                    /* must be <= FLAC__MAX_CHANNELS */
	unsigned bits_per_sample;             /* do not give the encoder wider data than what you specify here or bad things will happen! */
	unsigned sample_rate;
	unsigned blocksize;
	unsigned max_lpc_order;               /* 0 => encoder will not try general LPC, only fixed predictors; must be <= FLAC__MAX_LPC_ORDER */
	unsigned qlp_coeff_precision;         /* >= FLAC__MIN_QLP_COEFF_PRECISION, or 0 to let encoder select based on blocksize; */
	                                      /* qlp_coeff_precision+bits_per_sample must be < 32 */
	bool     do_qlp_coeff_prec_search;    /* 0 => use qlp_coeff_precision, 1 => search around qlp_coeff_precision, take best */
	bool     do_exhaustive_model_search;  /* 0 => use estimated bits per residual for scoring, 1 => generate all, take shortest */
	unsigned rice_optimization_level;     /* 0 => estimate Rice parameter based on residual variance, 1-8 => partition residual, use parameter for each */
} FLAC__Encoder;


FLAC__Encoder *FLAC__encoder_get_new_instance();
void FLAC__encoder_free_instance(FLAC__Encoder *encoder);
FLAC__EncoderState FLAC__encoder_init(FLAC__Encoder *encoder, FLAC__EncoderWriteStatus (*write_callback)(const FLAC__Encoder *encoder, const byte buffer[], unsigned bytes, unsigned samples, unsigned current_frame, void *client_data), void (*metadata_callback)(const FLAC__Encoder *encoder, const FLAC__StreamMetaData *metadata, void *client_data), void *client_data);
void FLAC__encoder_finish(FLAC__Encoder *encoder);
bool FLAC__encoder_process(FLAC__Encoder *encoder, const int32 *buf[], unsigned samples);
bool FLAC__encoder_process_interleaved(FLAC__Encoder *encoder, const int32 buf[], unsigned samples);

#endif
