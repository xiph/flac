/* libFLAC - Free Lossless Audio Codec library
 * Copyright (C) 2002  Josh Coalson
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

#ifndef FLAC__SEEKABLE_STREAM_ENCODER_H
#define FLAC__SEEKABLE_STREAM_ENCODER_H

#include "stream_encoder.h"

#ifdef __cplusplus
extern "C" {
#endif


/** \file include/FLAC/seekable_stream_encoder.h
 *
 *  \brief
 *  This module contains the functions which implement the seekable stream
 *  encoder.
 *
 *  See the detailed documentation in the
 *  \link flac_seekable_stream_encoder seekable stream encoder \endlink module.
 */

/** \defgroup flac_seekable_stream_encoder FLAC/seekable_stream_encoder.h: seekable stream encoder interface
 *  \ingroup flac_encoder
 *
 *  \brief
 *  This module contains the functions which implement the seekable stream
 *  encoder.
 *
 * XXX (import)
 *
 * The seekable stream encoder is a wrapper around the
 * \link flac_stream_encoder stream encoder \endlink which (XXXimport)
 *
 * Make sure to read the detailed description of the
 * \link flac_stream_encoder stream encoder module \endlink since the
 * seekable stream encoder inherits much of its behavior.
 *
 * \note
 * The "set" functions may only be called when the encoder is in the
 * state FLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED, i.e. after
 * FLAC__seekable_stream_encoder_new() or FLAC__seekable_stream_encoder_finish(), but
 * before FLAC__seekable_stream_encoder_init().  If this is the case they will
 * return \c true, otherwise \c false.
 *
 * \note
 * FLAC__seekable_stream_encoder_finish() resets all settings to the constructor
 * defaults, including the callbacks.
 *
 * \{
 */


/** State values for a FLAC__SeekableStreamEncoder
 *
 *  The encoder's state can be obtained by calling FLAC__seekable_stream_encoder_get_state().
 */
typedef enum {

	FLAC__SEEKABLE_STREAM_ENCODER_OK = 0,
	/**< The encoder is in the normal OK state. */

	FLAC__SEEKABLE_STREAM_ENCODER_STREAM_ENCODER_ERROR,
	/**< An error occurred in the underlying stream encoder;
	 * check FLAC__seekable_stream_encoder_get_stream_encoder_state().
	 */

	FLAC__SEEKABLE_STREAM_ENCODER_MEMORY_ALLOCATION_ERROR,
	/**< Memory allocation failed. */

	FLAC__SEEKABLE_STREAM_ENCODER_WRITE_ERROR,
	/**< The write callback returned an error. */

	FLAC__SEEKABLE_STREAM_ENCODER_READ_ERROR,
	/**< The read callback returned an error. */

	FLAC__SEEKABLE_STREAM_ENCODER_SEEK_ERROR,
	/**< The seek callback returned an error. */

	FLAC__SEEKABLE_STREAM_ENCODER_ALREADY_INITIALIZED,
	/**< FLAC__seekable_stream_encoder_init() was called when the encoder was
	 * already initialized, usually because
	 * FLAC__seekable_stream_encoder_finish() was not called.
	 */

    FLAC__SEEKABLE_STREAM_ENCODER_INVALID_CALLBACK,
	/**< FLAC__seekable_stream_encoder_init() was called without all
	 * callbacks being set.
	 */

    FLAC__SEEKABLE_STREAM_ENCODER_INVALID_SEEKTABLE,
	/**< An invalid seek table was passed is the metadata to
	 * FLAC__seekable_stream_encoder_set_metadata().
	 */

	FLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED
	/**< The encoder is in the uninitialized state. */

} FLAC__SeekableStreamEncoderState;

/** Maps a FLAC__SeekableStreamEncoderState to a C string.
 *
 *  Using a FLAC__SeekableStreamEncoderState as the index to this array
 *  will give the string equivalent.  The contents should not be modified.
 */
/* @@@@ double-check mapping */
extern const char * const FLAC__SeekableStreamEncoderStateString[];


/** Return values for the FLAC__SeekableStreamEncoder seek callback.
 */
typedef enum {

	FLAC__SEEKABLE_STREAM_ENCODER_SEEK_STATUS_OK,
	/**< The seek was OK and encoding can continue. */

	FLAC__SEEKABLE_STREAM_ENCODER_SEEK_STATUS_ERROR
	/**< An unrecoverable error occurred.  The encoder will return from the process call. */

} FLAC__SeekableStreamEncoderSeekStatus;

/** Maps a FLAC__SeekableStreamEncoderSeekStatus to a C string.
 *
 *  Using a FLAC__SeekableStreamEncoderSeekStatus as the index to this array
 *  will give the string equivalent.  The contents should not be modified.
 */
extern const char * const FLAC__SeekableStreamEncoderSeekStatusString[];


/***********************************************************************
 *
 * class FLAC__SeekableStreamEncoder
 *
 ***********************************************************************/

struct FLAC__SeekableStreamEncoderProtected;
struct FLAC__SeekableStreamEncoderPrivate;
/** The opaque structure definition for the seekable stream encoder type.
 *  See the \link flac_seekable_stream_encoder seekable stream encoder module \endlink
 *  for a detailed description.
 */
typedef struct {
	struct FLAC__SeekableStreamEncoderProtected *protected_; /* avoid the C++ keyword 'protected' */
	struct FLAC__SeekableStreamEncoderPrivate *private_; /* avoid the C++ keyword 'private' */
} FLAC__SeekableStreamEncoder;


/***********************************************************************
 *
 * Class constructor/destructor
 *
 ***********************************************************************/

/** Create a new seekable stream encoder instance.  The instance is created with
 *  default settings; see the individual FLAC__seekable_stream_encoder_set_*()
 *  functions for each setting's default.
 *
 * \retval FLAC__SeekableStreamEncoder*
 *    \c NULL if there was an error allocating memory, else the new instance.
 */
FLAC__SeekableStreamEncoder *FLAC__seekable_stream_encoder_new();

/** Free an encoder instance.  Deletes the object pointed to by \a encoder.
 *
 * \param encoder  A pointer to an existing encoder.
 * \assert
 *    \code encoder != NULL \endcode
 */
void FLAC__seekable_stream_encoder_delete(FLAC__SeekableStreamEncoder *encoder);

/***********************************************************************
 *
 * Public class method prototypes
 *
 ***********************************************************************/

/** This is inherited from FLAC__StreamEncoder; see
 *  FLAC__stream_encoder_set_streamable_subset().
 *
 * \default \c true
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC__bool FLAC__seekable_stream_encoder_set_streamable_subset(FLAC__SeekableStreamEncoder *encoder, FLAC__bool value);

/** This is inherited from FLAC__StreamEncoder; see
 *  FLAC__stream_encoder_set_do_mid_side_stereo().
 *
 * \default \c false
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC__bool FLAC__seekable_stream_encoder_set_do_mid_side_stereo(FLAC__SeekableStreamEncoder *encoder, FLAC__bool value);

/** This is inherited from FLAC__StreamEncoder; see
 *  FLAC__stream_encoder_set_loose_mid_side_stereo().
 *
 * \default \c false
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC__bool FLAC__seekable_stream_encoder_set_loose_mid_side_stereo(FLAC__SeekableStreamEncoder *encoder, FLAC__bool value);

/** This is inherited from FLAC__StreamEncoder; see
 *  FLAC__stream_encoder_set_channels().
 *
 * \default \c 2
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC__bool FLAC__seekable_stream_encoder_set_channels(FLAC__SeekableStreamEncoder *encoder, unsigned value);

/** This is inherited from FLAC__StreamEncoder; see
 *  FLAC__stream_encoder_set_bits_per_sample().
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
FLAC__bool FLAC__seekable_stream_encoder_set_bits_per_sample(FLAC__SeekableStreamEncoder *encoder, unsigned value);

/** This is inherited from FLAC__StreamEncoder; see
 *  FLAC__stream_encoder_set_sample_rate().
 *
 * \default \c 44100
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC__bool FLAC__seekable_stream_encoder_set_sample_rate(FLAC__SeekableStreamEncoder *encoder, unsigned value);

/** This is inherited from FLAC__StreamEncoder; see
 *  FLAC__stream_encoder_set_blocksize().
 *
 * \default \c 1152
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC__bool FLAC__seekable_stream_encoder_set_blocksize(FLAC__SeekableStreamEncoder *encoder, unsigned value);

/** This is inherited from FLAC__StreamEncoder; see
 *  FLAC__stream_encoder_set_max_lpc_order().
 *
 * \default \c 0
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC__bool FLAC__seekable_stream_encoder_set_max_lpc_order(FLAC__SeekableStreamEncoder *encoder, unsigned value);

/** This is inherited from FLAC__StreamEncoder; see
 *  FLAC__stream_encoder_set_qlp_coeff_precision().
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
FLAC__bool FLAC__seekable_stream_encoder_set_qlp_coeff_precision(FLAC__SeekableStreamEncoder *encoder, unsigned value);

/** This is inherited from FLAC__StreamEncoder; see
 *  FLAC__stream_encoder_set_do_qlp_coeff_prec_search().
 *
 * \default \c false
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC__bool FLAC__seekable_stream_encoder_set_do_qlp_coeff_prec_search(FLAC__SeekableStreamEncoder *encoder, FLAC__bool value);

/** This is inherited from FLAC__StreamEncoder; see
 *  FLAC__stream_encoder_set_do_escape_coding().
 *
 * \default \c false
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC__bool FLAC__seekable_stream_encoder_set_do_escape_coding(FLAC__SeekableStreamEncoder *encoder, FLAC__bool value);

/** This is inherited from FLAC__StreamEncoder; see
 *  FLAC__stream_encoder_set_do_exhaustive_model_search().
 *
 * \default \c false
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC__bool FLAC__seekable_stream_encoder_set_do_exhaustive_model_search(FLAC__SeekableStreamEncoder *encoder, FLAC__bool value);

/** This is inherited from FLAC__StreamEncoder; see
 *  FLAC__stream_encoder_set_min_residual_partition_order().
 *
 * \default \c 0
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC__bool FLAC__seekable_stream_encoder_set_min_residual_partition_order(FLAC__SeekableStreamEncoder *encoder, unsigned value);

/** This is inherited from FLAC__StreamEncoder; see
 *  FLAC__stream_encoder_set_max_residual_partition_order().
 *
 * \default \c 0
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC__bool FLAC__seekable_stream_encoder_set_max_residual_partition_order(FLAC__SeekableStreamEncoder *encoder, unsigned value);

/** This is inherited from FLAC__StreamEncoder; see
 *  FLAC__stream_encoder_set_rice_parameter_search_dist().
 *
 * \default \c 0
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC__bool FLAC__seekable_stream_encoder_set_rice_parameter_search_dist(FLAC__SeekableStreamEncoder *encoder, unsigned value);

/** This is inherited from FLAC__StreamEncoder; see
 *  FLAC__stream_encoder_set_total_samples_estimate().
 *
 * \default \c 0
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC__bool FLAC__seekable_stream_encoder_set_total_samples_estimate(FLAC__SeekableStreamEncoder *encoder, FLAC__uint64 value);

/** This is inherited from FLAC__StreamEncoder; see
 *  FLAC__stream_encoder_set_metadata().
 *
 * \note
 * The decoder instance \b will modify the first \c SEEKTABLE block
 * as it transforms the template to a valid seektable while encoding,
 * but it is still up to the caller to free all metadata blocks after
 * encoding.
 *
 * \default \c NULL, 0
 * \param  encoder     An encoder instance to set.
 * \param  metadata    See above.
 * \param  num_blocks  See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC__bool FLAC__seekable_stream_encoder_set_metadata(FLAC__SeekableStreamEncoder *encoder, FLAC__StreamMetadata **metadata, unsigned num_blocks);

/** Set the seek callback.
 *  The supplied function will be called when the encoder needs to seek
 *  the output stream.  The encoder will pass the absolute byte offset
 *  to seek to, 0 meaning the beginning of the stream.
 *
 * \note
 * The callback is mandatory and must be set before initialization.
 *
 * \default \c NULL
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 *    \code value != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC__bool FLAC__seekable_stream_encoder_set_seek_callback(FLAC__SeekableStreamEncoder *encoder, FLAC__SeekableStreamEncoderSeekStatus (*value)(const FLAC__SeekableStreamEncoder *encoder, FLAC__uint64 absolute_byte_offset, void *client_data));

/** Set the write callback.
 *  This is inherited from FLAC__StreamEncoder; see
 *  FLAC__stream_encoder_set_write_callback().
 *
 * \note
 * The callback is mandatory and must be set before initialization.
 *
 * \default \c NULL
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 *    \code value != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC__bool FLAC__seekable_stream_encoder_set_write_callback(FLAC__SeekableStreamEncoder *encoder, FLAC__StreamEncoderWriteStatus (*value)(const FLAC__SeekableStreamEncoder *encoder, const FLAC__byte buffer[], unsigned bytes, unsigned samples, unsigned current_frame, void *client_data));

/** Set the client data to be passed back to callbacks.
 *  This value will be supplied to callbacks in their \a client_data
 *  argument.
 *
 * \default \c NULL
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC__bool FLAC__seekable_stream_encoder_set_client_data(FLAC__SeekableStreamEncoder *encoder, void *value);

/** Get the current encoder state.
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__SeekableStreamEncoderState
 *    The current encoder state.
 */
FLAC__SeekableStreamEncoderState FLAC__seekable_stream_encoder_get_state(const FLAC__SeekableStreamEncoder *encoder);

/** Get the state of the underlying stream encoder.
 *  Useful when the seekable stream encoder state is
 *  \c FLAC__SEEKABLE_STREAM_ENCODER_STREAM_ENCODER_ERROR.
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__StreamEncoderState
 *    The stream encoder state.
 */
FLAC__StreamEncoderState FLAC__seekable_stream_encoder_get_stream_encoder_state(const FLAC__SeekableStreamEncoder *encoder);

/** Get the "streamable subset" flag.
 *  This is inherited from FLAC__StreamEncoder; see
 *  FLAC__stream_encoder_get_streamable_subset().
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    See FLAC__seekable_stream_encoder_set_streamable_subset().
 */
FLAC__bool FLAC__seekable_stream_encoder_get_streamable_subset(const FLAC__SeekableStreamEncoder *encoder);

/** Get the "mid/side stereo coding" flag.
 *  This is inherited from FLAC__StreamEncoder; see
 *  FLAC__stream_encoder_get_do_mid_side_stereo().
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    See FLAC__seekable_stream_encoder_get_do_mid_side_stereo().
 */
FLAC__bool FLAC__seekable_stream_encoder_get_do_mid_side_stereo(const FLAC__SeekableStreamEncoder *encoder);

/** Get the "adaptive mid/side switching" flag.
 *  This is inherited from FLAC__StreamEncoder; see
 *  FLAC__stream_encoder_get_loose_mid_side_stereo().
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    See FLAC__seekable_stream_encoder_set_loose_mid_side_stereo().
 */
FLAC__bool FLAC__seekable_stream_encoder_get_loose_mid_side_stereo(const FLAC__SeekableStreamEncoder *encoder);

/** Get the number of input channels being processed.
 *  This is inherited from FLAC__StreamEncoder; see
 *  FLAC__stream_encoder_get_channels().
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    See FLAC__seekable_stream_encoder_set_channels().
 */
unsigned FLAC__seekable_stream_encoder_get_channels(const FLAC__SeekableStreamEncoder *encoder);

/** Get the input sample resolution setting.
 *  This is inherited from FLAC__StreamEncoder; see
 *  FLAC__stream_encoder_get_bits_per_sample().
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    See FLAC__seekable_stream_encoder_set_bits_per_sample().
 */
unsigned FLAC__seekable_stream_encoder_get_bits_per_sample(const FLAC__SeekableStreamEncoder *encoder);

/** Get the input sample rate setting.
 *  This is inherited from FLAC__StreamEncoder; see
 *  FLAC__stream_encoder_get_sample_rate().
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    See FLAC__seekable_stream_encoder_set_sample_rate().
 */
unsigned FLAC__seekable_stream_encoder_get_sample_rate(const FLAC__SeekableStreamEncoder *encoder);

/** Get the blocksize setting.
 *  This is inherited from FLAC__StreamEncoder; see
 *  FLAC__stream_encoder_get_blocksize().
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    See FLAC__seekable_stream_encoder_set_blocksize().
 */
unsigned FLAC__seekable_stream_encoder_get_blocksize(const FLAC__SeekableStreamEncoder *encoder);

/** Get the maximum LPC order setting.
 *  This is inherited from FLAC__StreamEncoder; see
 *  FLAC__stream_encoder_get_max_lpc_order().
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    See FLAC__seekable_stream_encoder_set_max_lpc_order().
 */
unsigned FLAC__seekable_stream_encoder_get_max_lpc_order(const FLAC__SeekableStreamEncoder *encoder);

/** Get the quantized linear predictor coefficient precision setting.
 *  This is inherited from FLAC__StreamEncoder; see
 *  FLAC__stream_encoder_get_qlp_coeff_precision().
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    See FLAC__seekable_stream_encoder_set_qlp_coeff_precision().
 */
unsigned FLAC__seekable_stream_encoder_get_qlp_coeff_precision(const FLAC__SeekableStreamEncoder *encoder);

/** Get the qlp coefficient precision search flag.
 *  This is inherited from FLAC__StreamEncoder; see
 *  FLAC__stream_encoder_get_do_qlp_coeff_prec_search().
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    See FLAC__seekable_stream_encoder_set_do_qlp_coeff_prec_search().
 */
FLAC__bool FLAC__seekable_stream_encoder_get_do_qlp_coeff_prec_search(const FLAC__SeekableStreamEncoder *encoder);

/** Get the "escape coding" flag.
 *  This is inherited from FLAC__StreamEncoder; see
 *  FLAC__stream_encoder_get_do_escape_coding().
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    See FLAC__seekable_stream_encoder_set_do_escape_coding().
 */
FLAC__bool FLAC__seekable_stream_encoder_get_do_escape_coding(const FLAC__SeekableStreamEncoder *encoder);

/** Get the exhaustive model search flag.
 *  This is inherited from FLAC__StreamEncoder; see
 *  FLAC__stream_encoder_get_do_exhaustive_model_search().
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    See FLAC__seekable_stream_encoder_set_do_exhaustive_model_search().
 */
FLAC__bool FLAC__seekable_stream_encoder_get_do_exhaustive_model_search(const FLAC__SeekableStreamEncoder *encoder);

/** Get the minimum residual partition order setting.
 *  This is inherited from FLAC__StreamEncoder; see
 *  FLAC__stream_encoder_get_min_residual_partition_order().
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    See FLAC__seekable_stream_encoder_set_min_residual_partition_order().
 */
unsigned FLAC__seekable_stream_encoder_get_min_residual_partition_order(const FLAC__SeekableStreamEncoder *encoder);

/** Get maximum residual partition order setting.
 *  This is inherited from FLAC__StreamEncoder; see
 *  FLAC__stream_encoder_get_max_residual_partition_order().
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    See FLAC__seekable_stream_encoder_set_max_residual_partition_order().
 */
unsigned FLAC__seekable_stream_encoder_get_max_residual_partition_order(const FLAC__SeekableStreamEncoder *encoder);

/** Get the Rice parameter search distance setting.
 *  This is inherited from FLAC__StreamEncoder; see
 *  FLAC__stream_encoder_get_rice_parameter_search_dist().
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    See FLAC__seekable_stream_encoder_set_rice_parameter_search_dist().
 */
unsigned FLAC__seekable_stream_encoder_get_rice_parameter_search_dist(const FLAC__SeekableStreamEncoder *encoder);

/** Initialize the encoder instance.
 *  Should be called after FLAC__seekable_stream_encoder_new() and
 *  FLAC__seekable_stream_encoder_set_*() but before FLAC__seekable_stream_encoder_process()
 *  or FLAC__seekable_stream_encoder_process_interleaved().  Will set and return
 *  the encoder state, which will be FLAC__SEEKABLE_STREAM_ENCODER_OK if
 *  initialization succeeded.
 *
 *  The call to FLAC__seekable_stream_encoder_init() currently will also immediately
 *  call the write callback with the \c fLaC signature and all the encoded
 *  metadata.
 *
 * \param  encoder  An uninitialized encoder instance.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__SeekableStreamEncoderState
 *    \c FLAC__SEEKABLE_STREAM_ENCODER_OK if initialization was successful; see
 *    FLAC__SeekableStreamEncoderState for the meanings of other return values.
 */
FLAC__SeekableStreamEncoderState FLAC__seekable_stream_encoder_init(FLAC__SeekableStreamEncoder *encoder);

/** Finish the encoding process.
 *  Flushes the encoding buffer, releases resources, resets the encoder
 *  settings to their defaults, and returns the encoder state to
 *  FLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED.
 *
 *  In the event of a prematurely-terminated encode, it is not strictly
 *  necessary to call this immediately before FLAC__seekable_stream_encoder_delete()
 *  but it is good practice to match every FLAC__seekable_stream_encoder_init()
 *  with a FLAC__seekable_stream_encoder_finish().
 *
 * \param  encoder  An uninitialized encoder instance.
 * \assert
 *    \code encoder != NULL \endcode
 */
void FLAC__seekable_stream_encoder_finish(FLAC__SeekableStreamEncoder *encoder);

/** Submit data for encoding.
 *  This is inherited from FLAC__StreamEncoder; see
 *  FLAC__stream_encoder_process().
 *
 * \param  encoder  An initialized encoder instance in the OK state.
 * \param  buffer   An array of pointers to each channel's signal.
 * \param  samples  The number of samples in one channel.
 * \assert
 *    \code encoder != NULL \endcode
 *    \code FLAC__seekable_stream_encoder_get_state(encoder) == FLAC__SEEKABLE_STREAM_ENCODER_OK \endcode
 * \retval FLAC__bool
 *    \c true if successful, else \c false; in this case, check the
 *    encoder state with FLAC__seekable_stream_encoder_get_state() to see what
 *    went wrong.
 */
FLAC__bool FLAC__seekable_stream_encoder_process(FLAC__SeekableStreamEncoder *encoder, const FLAC__int32 * const buffer[], unsigned samples);

/** Submit data for encoding.
 *  This is inherited from FLAC__StreamEncoder; see
 *  FLAC__stream_encoder_process_interleaved().
 *
 * \param  encoder  An initialized encoder instance in the OK state.
 * \param  buffer   An array of channel-interleaved data (see above).
 * \param  samples  The number of samples in one channel, the same as for
 *                  FLAC__seekable_stream_encoder_process().  For example, if
 *                  encoding two channels, \c 1000 \a samples corresponds
 *                  to a \a buffer of 2000 values.
 * \assert
 *    \code encoder != NULL \endcode
 *    \code FLAC__seekable_stream_encoder_get_state(encoder) == FLAC__SEEKABLE_STREAM_ENCODER_OK \endcode
 * \retval FLAC__bool
 *    \c true if successful, else \c false; in this case, check the
 *    encoder state with FLAC__seekable_stream_encoder_get_state() to see what
 *    went wrong.
 */
FLAC__bool FLAC__seekable_stream_encoder_process_interleaved(FLAC__SeekableStreamEncoder *encoder, const FLAC__int32 buffer[], unsigned samples);

/* \} */

#ifdef __cplusplus
}
#endif

#endif
