/* libFLAC - Free Lossless Audio Codec library
 * Copyright (C) 2000,2001,2002  Josh Coalson
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
	FLAC__STREAM_ENCODER_INVALID_CALLBACK,
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
	struct FLAC__StreamEncoderProtected *protected_; /* avoid the C++ keyword 'protected' */
	struct FLAC__StreamEncoderPrivate *private_; /* avoid the C++ keyword 'private' */
} FLAC__StreamEncoder;

/***********************************************************************
 *
 * Class constructor/destructor
 *
 ***********************************************************************/

/*
 * Any parameters that are not set before FLAC__stream_encoder_init()
 * will take on the defaults from the constructor, shown below.
 * For more on what the parameters mean, see the documentation.
 *
 * FLAC__bool   streamable_subset              (DEFAULT: true ) true to limit encoder to generating a Subset stream, else false
 * FLAC__bool   do_mid_side_stereo             (DEFAULT: false) if true then channels must be 2
 * FLAC__bool   loose_mid_side_stereo          (DEFAULT: false) if true then do_mid_side_stereo must be true
 * unsigned     channels                       (DEFAULT: 2    ) must be <= FLAC__MAX_CHANNELS
 * unsigned     bits_per_sample                (DEFAULT: 16   ) do not give the encoder wider data than what you specify here or bad things will happen!
 * unsigned     sample_rate                    (DEFAULT: 44100)
 * unsigned     blocksize                      (DEFAULT: 1152 )
 * unsigned     max_lpc_order                  (DEFAULT: 0    ) 0 => encoder will not try general LPC, only fixed predictors; must be <= FLAC__MAX_LPC_ORDER
 * unsigned     qlp_coeff_precision            (DEFAULT: 0    ) >= FLAC__MIN_QLP_COEFF_PRECISION, or 0 to let encoder select based on blocksize;
 *                                                          qlp_coeff_precision+bits_per_sample must be < 32
 * FLAC__bool   do_qlp_coeff_prec_search       (DEFAULT: false) false => use qlp_coeff_precision, true => search around qlp_coeff_precision, take best
 * FLAC__bool   do_escape_coding               (DEFAULT: false) true => search for escape codes in the entropy coding stage for slightly better compression
 * FLAC__bool   do_exhaustive_model_search     (DEFAULT: false) false => use estimated bits per residual for scoring, true => generate all, take shortest
 * unsigned     min_residual_partition_order   (DEFAULT: 0    ) 0 => estimate Rice parameter based on residual variance; >0 => partition residual, use parameter
 * unsigned     max_residual_partition_order   (DEFAULT: 0    )      for each based on mean; min_ and max_ specify the min and max Rice partition order
 * unsigned     rice_parameter_search_dist     (DEFAULT: 0    ) 0 => try only calc'd parameter k; else try all [k-dist..k+dist] parameters, use best
 * FLAC__uint64 total_samples_estimate         (DEFAULT: 0    ) may be 0 if unknown.  acts as a placeholder in the STREAMINFO until the actual total is calculated
 * const FLAC__StreamMetaData_SeekTable *seek_table  (DEFAULT: NULL) optional seek_table to prepend, NULL => no seek table
 * unsigned     padding                        (DEFAULT: 0    ) size of PADDING block to add (goes after seek table); 0 => do not add a PADDING block
 * FLAC__bool   last_metadata_is_last          (DEFAULT: true ) the value the encoder will use for the 'is_last' flag of the last metadata block it writes; set
 *                                                          this to false if you will be adding more metadata blocks before the audio frames, else true
 *            (*write_callback)()              (DEFAULT: NULL ) The callbacks are the only values that MUST be set before FLAC__stream_encoder_init()
 *            (*metadata_callback)()           (DEFAULT: NULL )
 * void*        client_data                    (DEFAULT: NULL ) passed back through the callbacks
 */
FLAC__StreamEncoder *FLAC__stream_encoder_new();
void FLAC__stream_encoder_delete(FLAC__StreamEncoder *encoder);

/***********************************************************************
 *
 * Public class method prototypes
 *
 ***********************************************************************/

/*
 * Various "set" methods.  These may only be called when the encoder
 * is in the state FLAC__STREAM_ENCODER_UNINITIALIZED, i.e. after
 * FLAC__stream_encoder_new() or FLAC__stream_encoder_finish(), but
 * before FLAC__stream_encoder_init().  If this is the case they will
 * return true, otherwise false.
 *
 * NOTE that these functions do not validate the values as many are
 * interdependent.  The FLAC__stream_encoder_init() function will do
 * this, so make sure to pay attention to the state returned by
 * FLAC__stream_encoder_init().
 *
 * Any parameters that are not set before FLAC__stream_encoder_init()
 * will take on the defaults from the constructor.  NOTE that
 * FLAC__stream_encoder_finish() does NOT reset the values to the
 * constructor defaults.
 */
FLAC__bool FLAC__stream_encoder_set_streamable_subset(const FLAC__StreamEncoder *encoder, FLAC__bool value);
FLAC__bool FLAC__stream_encoder_set_do_mid_side_stereo(const FLAC__StreamEncoder *encoder, FLAC__bool value);
FLAC__bool FLAC__stream_encoder_set_loose_mid_side_stereo(const FLAC__StreamEncoder *encoder, FLAC__bool value);
FLAC__bool FLAC__stream_encoder_set_channels(const FLAC__StreamEncoder *encoder, unsigned value);
FLAC__bool FLAC__stream_encoder_set_bits_per_sample(const FLAC__StreamEncoder *encoder, unsigned value);
FLAC__bool FLAC__stream_encoder_set_sample_rate(const FLAC__StreamEncoder *encoder, unsigned value);
FLAC__bool FLAC__stream_encoder_set_blocksize(const FLAC__StreamEncoder *encoder, unsigned value);
FLAC__bool FLAC__stream_encoder_set_max_lpc_order(const FLAC__StreamEncoder *encoder, unsigned value);
FLAC__bool FLAC__stream_encoder_set_qlp_coeff_precision(const FLAC__StreamEncoder *encoder, unsigned value);
FLAC__bool FLAC__stream_encoder_set_do_qlp_coeff_prec_search(const FLAC__StreamEncoder *encoder, FLAC__bool value);
FLAC__bool FLAC__stream_encoder_set_do_escape_coding(const FLAC__StreamEncoder *encoder, FLAC__bool value);
FLAC__bool FLAC__stream_encoder_set_do_exhaustive_model_search(const FLAC__StreamEncoder *encoder, FLAC__bool value);
FLAC__bool FLAC__stream_encoder_set_min_residual_partition_order(const FLAC__StreamEncoder *encoder, unsigned value);
FLAC__bool FLAC__stream_encoder_set_max_residual_partition_order(const FLAC__StreamEncoder *encoder, unsigned value);
FLAC__bool FLAC__stream_encoder_set_rice_parameter_search_dist(const FLAC__StreamEncoder *encoder, unsigned value);
FLAC__bool FLAC__stream_encoder_set_total_samples_estimate(const FLAC__StreamEncoder *encoder, FLAC__uint64 value);
FLAC__bool FLAC__stream_encoder_set_seek_table(const FLAC__StreamEncoder *encoder, const FLAC__StreamMetaData_SeekTable *value);
FLAC__bool FLAC__stream_encoder_set_padding(const FLAC__StreamEncoder *encoder, unsigned value);
FLAC__bool FLAC__stream_encoder_set_last_metadata_is_last(const FLAC__StreamEncoder *encoder, FLAC__bool value);
FLAC__bool FLAC__stream_encoder_set_write_callback(const FLAC__StreamEncoder *encoder, FLAC__StreamEncoderWriteStatus (*value)(const FLAC__StreamEncoder *encoder, const FLAC__byte buffer[], unsigned bytes, unsigned samples, unsigned current_frame, void *client_data));
FLAC__bool FLAC__stream_encoder_set_metadata_callback(const FLAC__StreamEncoder *encoder, void (*value)(const FLAC__StreamEncoder *encoder, const FLAC__StreamMetaData *metadata, void *client_data));
FLAC__bool FLAC__stream_encoder_set_client_data(const FLAC__StreamEncoder *encoder, void *value);

/*
 * Various "get" methods
 */
FLAC__StreamEncoderState FLAC__stream_encoder_get_state(const FLAC__StreamEncoder *encoder);
FLAC__bool FLAC__stream_encoder_get_streamable_subset(const FLAC__StreamEncoder *encoder);
FLAC__bool FLAC__stream_encoder_get_do_mid_side_stereo(const FLAC__StreamEncoder *encoder);
FLAC__bool FLAC__stream_encoder_get_loose_mid_side_stereo(const FLAC__StreamEncoder *encoder);
unsigned   FLAC__stream_encoder_get_channels(const FLAC__StreamEncoder *encoder);
unsigned   FLAC__stream_encoder_get_bits_per_sample(const FLAC__StreamEncoder *encoder);
unsigned   FLAC__stream_encoder_get_sample_rate(const FLAC__StreamEncoder *encoder);
unsigned   FLAC__stream_encoder_get_blocksize(const FLAC__StreamEncoder *encoder);
unsigned   FLAC__stream_encoder_get_max_lpc_order(const FLAC__StreamEncoder *encoder);
unsigned   FLAC__stream_encoder_get_qlp_coeff_precision(const FLAC__StreamEncoder *encoder);
FLAC__bool FLAC__stream_encoder_get_do_qlp_coeff_prec_search(const FLAC__StreamEncoder *encoder);
FLAC__bool FLAC__stream_encoder_get_do_escape_coding(const FLAC__StreamEncoder *encoder);
FLAC__bool FLAC__stream_encoder_get_do_exhaustive_model_search(const FLAC__StreamEncoder *encoder);
unsigned   FLAC__stream_encoder_get_min_residual_partition_order(const FLAC__StreamEncoder *encoder);
unsigned   FLAC__stream_encoder_get_max_residual_partition_order(const FLAC__StreamEncoder *encoder);
unsigned   FLAC__stream_encoder_get_rice_parameter_search_dist(const FLAC__StreamEncoder *encoder);

/*
 * Initialize the instance; should be called after construction and
 * 'set' calls but before any of the 'process' calls.  Will set and
 * return the encoder state, which will be FLAC__STREAM_ENCODER_OK
 * if initialization succeeded.
 */
FLAC__StreamEncoderState FLAC__stream_encoder_init(FLAC__StreamEncoder *encoder);

/*
 * Flush the encoding buffer, release resources, and return the encoder
 * state to FLAC__STREAM_ENCODER_UNINITIALIZED.  Note that this can
 * generate one or more write_callback()s before returning.
 */
void FLAC__stream_encoder_finish(FLAC__StreamEncoder *encoder);

/*
 * Methods for encoding the data
 */
FLAC__bool FLAC__stream_encoder_process(FLAC__StreamEncoder *encoder, const FLAC__int32 *buf[], unsigned samples);
FLAC__bool FLAC__stream_encoder_process_interleaved(FLAC__StreamEncoder *encoder, const FLAC__int32 buf[], unsigned samples);

#endif
