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

#ifdef __cplusplus
extern "C" {
#endif


/** \file include/FLAC/stream_encoder.h
 *
 *  \brief
 *  This module contains the functions which implement the stream
 *  encoder.
 *
 *  See the detailed documentation in the
 *  \link flac_stream_encoder stream encoder \endlink module.
 */

/** \defgroup flac_stream_encoder FLAC/stream_encoder.h: stream encoder interface
 *  \ingroup flac
 *
 *  \brief
 *  This module contains the functions which implement the stream
 *  encoder.
 *
 * The basic usage of a libFLAC encoder is as follows: 
 * - The program creates an instance of an encoder using
 *   FLAC__stream_encoder_new().
 * - The program overrides the default settings and sets callbacks for
 *   reading, writing, error reporting, and metadata reporting using
 *   FLAC__stream_encoder_set_*() functions.
 * - The program initializes the instance to validate the settings and
 *   prepare for encoding using FLAC__stream_encoder_init(). 
 * - The program calls FLAC__stream_encoder_process() or
 *   FLAC__stream_encoder_process_interleaved() to encode data, which
 *   subsequently calls the callbacks. 
 * - The program finishes the instance with FLAC__stream_encoder_finish(),
 *   which flushes the input and output and resets the encoder to the
 *   unitialized state. 
 * - The instance may be used again or deleted with
 *   FLAC__stream_encoder_delete(). 
 *
 * \note
 * The "set" functions may only be called when the encoder is in the
 * state FLAC__STREAM_ENCODER_UNINITIALIZED, i.e. after
 * FLAC__stream_encoder_new() or FLAC__stream_encoder_finish(), but
 * before FLAC__stream_encoder_init().  If this is the case they will
 * return true, otherwise false.
 *
 * \note
 * The "set" functions do not validate the values as many are
 * interdependent.  The FLAC__stream_encoder_init() function will do
 * this, so make sure to pay attention to the state returned by
 * FLAC__stream_encoder_init() to make sure that it is
 * FLAC__STREAM_ENCODER_OK.
 *
 * \note
 * Unlike the decoders, the stream encoder has many options that can
 * affect the speed and compression ratio.  When setting these parameters
 * you should have some basic knowledge of the format (see the
 * <A HREF="../documentation.html#format">user-level documentation</A>
 * or the <A HREF="../format.html">formal description</A>).
 *
 * \note
 * Any parameters that are not set before FLAC__stream_encoder_init()
 * will take on the defaults from the constructor.
 *
 * \note
 * FLAC__stream_encoder_finish() resets all settings to the constructor
 * defaults, including the callbacks.
 * 
 * \{
 */


/** State values for a FLAC__StreamEncoder
 *
 *  The encoder's state can be obtained by calling FLAC__stream_encoder_get_state().
 */
typedef enum {

	FLAC__STREAM_ENCODER_OK = 0,
	/**< The encoder is in the normal OK state. */

	FLAC__STREAM_ENCODER_INVALID_CALLBACK,
	/**< The encoder was initialized before setting all the required callbacks. */

	FLAC__STREAM_ENCODER_INVALID_NUMBER_OF_CHANNELS,
	/**< The encoder has an invalid setting for number of channels. */

	FLAC__STREAM_ENCODER_INVALID_BITS_PER_SAMPLE,
	/**< The encoder has an invalid setting for bits-per-sample.
	 * FLAC supports 4-32 bps but the reference encoder currently supports
	 * only up to 24 bps.
	 */

	FLAC__STREAM_ENCODER_INVALID_SAMPLE_RATE,
	/**< The encoder has an invalid setting for the input sample rate. */

	FLAC__STREAM_ENCODER_INVALID_BLOCK_SIZE,
	/**< The encoder has an invalid setting for the block size. */

	FLAC__STREAM_ENCODER_INVALID_QLP_COEFF_PRECISION,
	/**< The encoder has an invalid setting for the precision of the quantized linear predictor coefficients. */

	FLAC__STREAM_ENCODER_MID_SIDE_CHANNELS_MISMATCH,
	/**< Mid/side coding was specified but the number of channels is not equal to 2. */

	FLAC__STREAM_ENCODER_MID_SIDE_SAMPLE_SIZE_MISMATCH,
	/**< Deprecated. */

	FLAC__STREAM_ENCODER_ILLEGAL_MID_SIDE_FORCE,
	/**< Loose mid/side coding was specified but mid/side coding was not. */

	FLAC__STREAM_ENCODER_BLOCK_SIZE_TOO_SMALL_FOR_LPC_ORDER,
	/**< The specified block size is less than the maximum LPC order. */

	FLAC__STREAM_ENCODER_NOT_STREAMABLE,
	/**< The encoder is bound to the "streamable subset" but other settings violate it. */

	FLAC__STREAM_ENCODER_FRAMING_ERROR,
	/**< An error occurred while writing the stream; usually, the write_callback returned an error. */

	FLAC__STREAM_ENCODER_INVALID_METADATA,
	/**< The metadata input to the encoder is invalid. */

	FLAC__STREAM_ENCODER_FATAL_ERROR_WHILE_ENCODING,
	/**< An error occurred while writing the stream; usually, the write_callback returned an error. */

	FLAC__STREAM_ENCODER_FATAL_ERROR_WHILE_WRITING,
	/**< The write_callback returned an error. */

	FLAC__STREAM_ENCODER_MEMORY_ALLOCATION_ERROR,
	/**< Memory allocation failed. */

	FLAC__STREAM_ENCODER_ALREADY_INITIALIZED,
	/**< FLAC__stream_encoder_init() was called when the encoder was
	 * already initialized, usually because
	 * FLAC__stream_encoder_finish() was not called.
	 */

	FLAC__STREAM_ENCODER_UNINITIALIZED
	/**< The encoder is in the uninitialized state. */

} FLAC__StreamEncoderState;

/** Maps a FLAC__StreamEncoderState to a C string.
 *
 *  Using a FLAC__StreamEncoderState as the index to this array
 *  will give the string equivalent.  The contents should not be modified.
 */
extern const char * const FLAC__StreamEncoderStateString[];

/** Return values for the FLAC__StreamEncoder write callback.
 */
typedef enum {

	FLAC__STREAM_ENCODER_WRITE_OK = 0,
	/**< The write was OK and encoding can continue. */

	FLAC__STREAM_ENCODER_WRITE_FATAL_ERROR
	/**< An unrecoverable error occurred.  The encoder will return from the process call. */

} FLAC__StreamEncoderWriteStatus;

/** Maps a FLAC__StreamEncoderWriteStatus to a C string.
 *
 *  Using a FLAC__StreamEncoderWriteStatus as the index to this array
 *  will give the string equivalent.  The contents should not be modified.
 */
extern const char * const FLAC__StreamEncoderWriteStatusString[];


/***********************************************************************
 *
 * class FLAC__StreamEncoder
 *
 ***********************************************************************/

struct FLAC__StreamEncoderProtected;
struct FLAC__StreamEncoderPrivate;
/** The opaque structure definition for the stream encoder type.
 */
typedef struct {
	struct FLAC__StreamEncoderProtected *protected_; /* avoid the C++ keyword 'protected' */
	struct FLAC__StreamEncoderPrivate *private_; /* avoid the C++ keyword 'private' */
} FLAC__StreamEncoder;


/***********************************************************************
 *
 * Class constructor/destructor
 *
 ***********************************************************************/

/** Create a new stream encoder instance.  The instance is created with
 *  default settings; see the individual FLAC__stream_encoder_set_*()
 *  functions for each setting's default.
 *
 * \retval FLAC__StreamEncoder*
 *    \c NULL if there was an error allocating memory, else the new instance.
 */
FLAC__StreamEncoder *FLAC__stream_encoder_new();

/** Free an encoder instance.  Deletes the object pointed to by \a encoder.
 *
 * \param encoder  A pointer to an existing encoder.
 * \assert
 *    \code encoder != NULL \endcode
 */
void FLAC__stream_encoder_delete(FLAC__StreamEncoder *encoder);

/***********************************************************************
 *
 * Public class method prototypes
 *
 ***********************************************************************/

/*
 * FLAC__bool   do_qlp_coeff_prec_search      (DEFAULT: false ) false => use qlp_coeff_precision, true => search around qlp_coeff_precision, take best
 * FLAC__bool   do_escape_coding              (DEFAULT: false ) true => search for escape codes in the entropy coding stage for slightly better compression
 * FLAC__bool   do_exhaustive_model_search    (DEFAULT: false ) false => use estimated bits per residual for scoring, true => generate all, take shortest
 * unsigned     min_residual_partition_order  (DEFAULT: 0     ) 0 => estimate Rice parameter based on residual variance; >0 => partition residual, use parameter
 * unsigned     max_residual_partition_order  (DEFAULT: 0     )      for each based on mean; min_ and max_ specify the min and max Rice partition order
 * unsigned     rice_parameter_search_dist    (DEFAULT: 0     ) 0 => try only calc'd parameter k; else try all [k-dist..k+dist] parameters, use best
 * FLAC__uint64 total_samples_estimate        (DEFAULT: 0     ) may be 0 if unknown.  acts as a placeholder in the STREAMINFO until the actual total is calculated
 * FLAC__StreamMetadata **metadata            (DEFAULT: NULL,0) optional metadata blocks to prepend.  STREAMINFO is not allowed since it is done internally.
 * + unsigned num_blocks
 * void*        client_data                   (DEFAULT: NULL  ) passed back through the callbacks
 */

/** Set the "streamable subset" flag.  If \c true, the encoder will comply
 *  with the subset (see the format specification) and will check the
 *  settings during FLAC__stream_encoder_init() to see if all settings
 *  comply.  If \c false, the settings may take advantage of the full
 *  range that the format allows.
 *
 *  Make sure you know what it entails before setting this to \c false.
 *
 * \default \c true
 * \param  encoder  An encoder instance to set.
 * \param  value    Flag value (see above).
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC__bool FLAC__stream_encoder_set_streamable_subset(FLAC__StreamEncoder *encoder, FLAC__bool value);

/** Set to \c true to enable mid-side encoding on stereo input.  The
 *  number of channels must be 2.  Set to \c false to use only
 *  independent channel coding.
 *
 * \default \c false
 * \param  encoder  An encoder instance to set.
 * \param  value    Flag value (see above).
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC__bool FLAC__stream_encoder_set_do_mid_side_stereo(FLAC__StreamEncoder *encoder, FLAC__bool value);

/** Set to \c true to enable adaptive switching between mid-side and
 *  left-right encoding on stereo input.  The number of channels must
 *  be 2.  Set to \c false to use exhaustive searching.  In either
 *  case, the mid/side stereo setting must be \c true.
 *
 * \default \c false
 * \param  encoder  An encoder instance to set.
 * \param  value    Flag value (see above).
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC__bool FLAC__stream_encoder_set_loose_mid_side_stereo(FLAC__StreamEncoder *encoder, FLAC__bool value);

/** Set the number of channels to be encoded.
 *
 * \default \c 2
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC__bool FLAC__stream_encoder_set_channels(FLAC__StreamEncoder *encoder, unsigned value);

/** Set the sample resolution of the input to be encoded.
 *
 * \warning
 * Do not feed the encoder data that is wider than the value you
 * set here or you will generate an invalid stream.
 *
 * \default \c 16
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC__bool FLAC__stream_encoder_set_bits_per_sample(FLAC__StreamEncoder *encoder, unsigned value);

/** Set the sample rate (in Hz) of the input to be encoded.
 *
 * \default \c 44100
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC__bool FLAC__stream_encoder_set_sample_rate(FLAC__StreamEncoder *encoder, unsigned value);

/** Set the blocksize to use while encoding.
 *
 * \default \c 1152
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC__bool FLAC__stream_encoder_set_blocksize(FLAC__StreamEncoder *encoder, unsigned value);

/** Set the maximum LPC order, or \c 0 to use only the fixed predictors.
 *
 * \default \c 0
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC__bool FLAC__stream_encoder_set_max_lpc_order(FLAC__StreamEncoder *encoder, unsigned value);

/** Set the precision, in bits, of the quantized linear predictor
 *  coefficients, or \c 0 to let the encoder select it based on the
 *  blocksize.
 *
 * \note
 * In the current implementation, qlp_coeff_precision + bits_per_sample must
 * be less than 32.
 *
 * \default \c 0
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC__bool FLAC__stream_encoder_set_qlp_coeff_precision(FLAC__StreamEncoder *encoder, unsigned value);

/** Set the number of channels to be encoded.
 *
 * \default \c false
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC__bool FLAC__stream_encoder_set_do_qlp_coeff_prec_search(FLAC__StreamEncoder *encoder, FLAC__bool value);

/** Set the number of channels to be encoded.
 *
 * \default \c false
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC__bool FLAC__stream_encoder_set_do_escape_coding(FLAC__StreamEncoder *encoder, FLAC__bool value);

/** Set the number of channels to be encoded.
 *
 * \default \c false
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC__bool FLAC__stream_encoder_set_do_exhaustive_model_search(FLAC__StreamEncoder *encoder, FLAC__bool value);

/** Set the number of channels to be encoded.
 *
 * \default \c 0
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC__bool FLAC__stream_encoder_set_min_residual_partition_order(FLAC__StreamEncoder *encoder, unsigned value);

/** Set the number of channels to be encoded.
 *
 * \default \c 0
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC__bool FLAC__stream_encoder_set_max_residual_partition_order(FLAC__StreamEncoder *encoder, unsigned value);

/** Set the number of channels to be encoded.
 *
 * \default \c 0
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC__bool FLAC__stream_encoder_set_rice_parameter_search_dist(FLAC__StreamEncoder *encoder, unsigned value);

/** Set the number of channels to be encoded.
 *
 * \default \c 0
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC__bool FLAC__stream_encoder_set_total_samples_estimate(FLAC__StreamEncoder *encoder, FLAC__uint64 value);

/** Set the number of channels to be encoded.
 *
 * \default \c NULL, 0
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC__bool FLAC__stream_encoder_set_metadata(FLAC__StreamEncoder *encoder, FLAC__StreamMetadata **metadata, unsigned num_blocks);

/** Set the number of channels to be encoded.
 *
 * \default \c NULL; the callback is mandatory and must be set before initialization
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 *    \code value != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC__bool FLAC__stream_encoder_set_write_callback(FLAC__StreamEncoder *encoder, FLAC__StreamEncoderWriteStatus (*value)(const FLAC__StreamEncoder *encoder, const FLAC__byte buffer[], unsigned bytes, unsigned samples, unsigned current_frame, void *client_data));

/** Set the number of channels to be encoded.
 *
 * \default \c NULL; the callback is mandatory and must be set before initialization
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 *    \code value != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC__bool FLAC__stream_encoder_set_metadata_callback(FLAC__StreamEncoder *encoder, void (*value)(const FLAC__StreamEncoder *encoder, const FLAC__StreamMetadata *metadata, void *client_data));

/** Set the number of channels to be encoded.
 *
 * \default \c NULL
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC__bool FLAC__stream_encoder_set_client_data(FLAC__StreamEncoder *encoder, void *value);

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
FLAC__bool FLAC__stream_encoder_process(FLAC__StreamEncoder *encoder, const FLAC__int32 * const buffer[], unsigned samples);
FLAC__bool FLAC__stream_encoder_process_interleaved(FLAC__StreamEncoder *encoder, const FLAC__int32 buffer[], unsigned samples);

/* \} */

#ifdef __cplusplus
}
#endif

#endif
