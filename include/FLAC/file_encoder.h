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

#ifndef FLAC__FILE_ENCODER_H
#define FLAC__FILE_ENCODER_H

#include "seekable_stream_encoder.h"

#ifdef __cplusplus
extern "C" {
#endif


/** \file include/FLAC/file_encoder.h
 *
 *  \brief
 *  This module contains the functions which implement the file
 *  encoder.
 *
 *  See the detailed documentation in the
 *  \link flac_file_encoder file encoder \endlink module.
 */

/** \defgroup flac_file_encoder FLAC/file_encoder.h: file encoder interface
 *  \ingroup flac_encoder
 *
 *  \brief
 *  This module contains the functions which implement the file
 *  encoder.
 *
 * The basic usage of this encoder is as follows:
 * - The program creates an instance of an encoder using
 *   FLAC__file_encoder_new().
 * - The program overrides the default settings using
 *   FLAC__file_encoder_set_*() functions.
 * - The program initializes the instance to validate the settings and
 *   prepare for encoding using FLAC__file_encoder_init().
 * - The program calls FLAC__file_encoder_process() or
 *   FLAC__file_encoder_process_interleaved() to encode data, which
 *   subsequently writes data to the output file.
 * - The program finishes the encoding with FLAC__file_encoder_finish(),
 *   which causes the encoder to encode any data still in its input pipe,
 *   rewind and write the STREAMINFO metadata to file, and finally reset
 *   the encoder to the uninitialized state.
 * - The instance may be used again or deleted with
 *   FLAC__file_encoder_delete().
 *
 * The file encoder is a wrapper around the
 * \link flac_seekable_stream_encoder seekable stream encoder \endlink which supplies all
 * callbacks internally; the user need specify only the filename.
 *
 * Make sure to read the detailed description of the
 * \link flac_seekable_stream_encoder seekable stream encoder module \endlink since the
 * \link flac_stream_encoder stream encoder module \endlink since the
 * file encoder inherits much of its behavior from them.
 *
 * \note
 * The "set" functions may only be called when the encoder is in the
 * state FLAC__FILE_ENCODER_UNINITIALIZED, i.e. after
 * FLAC__file_encoder_new() or FLAC__file_encoder_finish(), but
 * before FLAC__file_encoder_init().  If this is the case they will
 * return \c true, otherwise \c false.
 *
 * \note
 * FLAC__file_encoder_finish() resets all settings to the constructor
 * defaults.
 *
 * \{
 */


/** State values for a FLAC__FileEncoder
 *
 *  The encoder's state can be obtained by calling FLAC__file_encoder_get_state().
 */
typedef enum {

	FLAC__FILE_ENCODER_OK = 0,
	/**< The encoder is in the normal OK state. */

	FLAC__FILE_ENCODER_NO_FILENAME,
	/**< FLAC__file_encoder_init() was called without first calling
	 * FLAC__file_encoder_set_filename().
	 */

	FLAC__FILE_ENCODER_SEEKABLE_STREAM_ENCODER_ERROR,
	/**< An error occurred in the underlying seekable stream encoder;
	 * check FLAC__file_encoder_get_seekable_stream_encoder_state().
	 */

	FLAC__FILE_ENCODER_FATAL_ERROR_WHILE_WRITING,
	/**< A fatal error occurred while writing to the encoded file. */

    FLAC__FILE_ENCODER_ERROR_OPENING_FILE,
	/**< An error occurred opening the output file for writing. */

	FLAC__FILE_ENCODER_MEMORY_ALLOCATION_ERROR,
	/**< Memory allocation failed. */

	FLAC__FILE_ENCODER_ALREADY_INITIALIZED,
	/**< FLAC__file_encoder_init() was called when the encoder was
	 * already initialized, usually because
	 * FLAC__file_encoder_finish() was not called.
	 */

	FLAC__FILE_ENCODER_UNINITIALIZED
	/**< The encoder is in the uninitialized state. */

} FLAC__FileEncoderState;

/** Maps a FLAC__FileEncoderState to a C string.
 *
 *  Using a FLAC__FileEncoderState as the index to this array
 *  will give the string equivalent.  The contents should not be modified.
 */
/* @@@@ double-check mapping */
extern const char * const FLAC__FileEncoderStateString[];


/***********************************************************************
 *
 * class FLAC__FileEncoder
 *
 ***********************************************************************/

struct FLAC__FileEncoderProtected;
struct FLAC__FileEncoderPrivate;
/** The opaque structure definition for the file encoder type.
 *  See the \link flac_file_encoder file encoder module \endlink
 *  for a detailed description.
 */
typedef struct {
	struct FLAC__FileEncoderProtected *protected_; /* avoid the C++ keyword 'protected' */
	struct FLAC__FileEncoderPrivate *private_; /* avoid the C++ keyword 'private' */
} FLAC__FileEncoder;


/***********************************************************************
 *
 * Class constructor/destructor
 *
 ***********************************************************************/

/** Create a new file encoder instance.  The instance is created with
 *  default settings; see the individual FLAC__file_encoder_set_*()
 *  functions for each setting's default.
 *
 * \retval FLAC__FileEncoder*
 *    \c NULL if there was an error allocating memory, else the new instance.
 */
FLAC__FileEncoder *FLAC__file_encoder_new();

/** Free an encoder instance.  Deletes the object pointed to by \a encoder.
 *
 * \param encoder  A pointer to an existing encoder.
 * \assert
 *    \code encoder != NULL \endcode
 */
void FLAC__file_encoder_delete(FLAC__FileEncoder *encoder);

/***********************************************************************
 *
 * Public class method prototypes
 *
 ***********************************************************************/

/** This is inherited from FLAC__SeekableStreamEncoder; see
 *  FLAC__seekable_stream_encoder_set_streamable_subset().
 *
 * \default \c true
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC__bool FLAC__file_encoder_set_streamable_subset(FLAC__FileEncoder *encoder, FLAC__bool value);

/** This is inherited from FLAC__SeekableStreamEncoder; see
 *  FLAC__seekable_stream_encoder_set_do_mid_side_stereo().
 *
 * \default \c false
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC__bool FLAC__file_encoder_set_do_mid_side_stereo(FLAC__FileEncoder *encoder, FLAC__bool value);

/** This is inherited from FLAC__SeekableStreamEncoder; see
 *  FLAC__seekable_stream_encoder_set_loose_mid_side_stereo().
 *
 * \default \c false
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC__bool FLAC__file_encoder_set_loose_mid_side_stereo(FLAC__FileEncoder *encoder, FLAC__bool value);

/** This is inherited from FLAC__SeekableStreamEncoder; see
 *  FLAC__seekable_stream_encoder_set_channels().
 *
 * \default \c 2
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC__bool FLAC__file_encoder_set_channels(FLAC__FileEncoder *encoder, unsigned value);

/** This is inherited from FLAC__SeekableStreamEncoder; see
 *  FLAC__seekable_stream_encoder_set_bits_per_sample().
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
FLAC__bool FLAC__file_encoder_set_bits_per_sample(FLAC__FileEncoder *encoder, unsigned value);

/** This is inherited from FLAC__SeekableStreamEncoder; see
 *  FLAC__seekable_stream_encoder_set_sample_rate().
 *
 * \default \c 44100
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC__bool FLAC__file_encoder_set_sample_rate(FLAC__FileEncoder *encoder, unsigned value);

/** This is inherited from FLAC__SeekableStreamEncoder; see
 *  FLAC__seekable_stream_encoder_set_blocksize().
 *
 * \default \c 1152
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC__bool FLAC__file_encoder_set_blocksize(FLAC__FileEncoder *encoder, unsigned value);

/** This is inherited from FLAC__SeekableStreamEncoder; see
 *  FLAC__seekable_stream_encoder_set_max_lpc_order().
 *
 * \default \c 0
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC__bool FLAC__file_encoder_set_max_lpc_order(FLAC__FileEncoder *encoder, unsigned value);

/** This is inherited from FLAC__SeekableStreamEncoder; see
 *  FLAC__seekable_stream_encoder_set_qlp_coeff_precision().
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
FLAC__bool FLAC__file_encoder_set_qlp_coeff_precision(FLAC__FileEncoder *encoder, unsigned value);

/** This is inherited from FLAC__SeekableStreamEncoder; see
 *  FLAC__seekable_stream_encoder_set_do_qlp_coeff_prec_search().
 *
 * \default \c false
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC__bool FLAC__file_encoder_set_do_qlp_coeff_prec_search(FLAC__FileEncoder *encoder, FLAC__bool value);

/** This is inherited from FLAC__SeekableStreamEncoder; see
 *  FLAC__seekable_stream_encoder_set_do_escape_coding().
 *
 * \default \c false
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC__bool FLAC__file_encoder_set_do_escape_coding(FLAC__FileEncoder *encoder, FLAC__bool value);

/** This is inherited from FLAC__SeekableStreamEncoder; see
 *  FLAC__seekable_stream_encoder_set_do_exhaustive_model_search().
 *
 * \default \c false
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC__bool FLAC__file_encoder_set_do_exhaustive_model_search(FLAC__FileEncoder *encoder, FLAC__bool value);

/** This is inherited from FLAC__SeekableStreamEncoder; see
 *  FLAC__seekable_stream_encoder_set_min_residual_partition_order().
 *
 * \default \c 0
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC__bool FLAC__file_encoder_set_min_residual_partition_order(FLAC__FileEncoder *encoder, unsigned value);

/** This is inherited from FLAC__SeekableStreamEncoder; see
 *  FLAC__seekable_stream_encoder_set_max_residual_partition_order().
 *
 * \default \c 0
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC__bool FLAC__file_encoder_set_max_residual_partition_order(FLAC__FileEncoder *encoder, unsigned value);

/** This is inherited from FLAC__SeekableStreamEncoder; see
 *  FLAC__seekable_stream_encoder_set_rice_parameter_search_dist().
 *
 * \default \c 0
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC__bool FLAC__file_encoder_set_rice_parameter_search_dist(FLAC__FileEncoder *encoder, unsigned value);

/** This is inherited from FLAC__SeekableStreamEncoder; see
 *  FLAC__seekable_stream_encoder_set_total_samples_estimate().
 *
 * \default \c 0
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC__bool FLAC__file_encoder_set_total_samples_estimate(FLAC__FileEncoder *encoder, FLAC__uint64 value);

/** This is inherited from FLAC__SeekableStreamEncoder; see
 *  FLAC__seekable_stream_encoder_set_metadata().
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
FLAC__bool FLAC__file_encoder_set_metadata(FLAC__FileEncoder *encoder, FLAC__FileMetadata **metadata, unsigned num_blocks);

/** Set the output file name encode to.
 *
 * \note
 * The filename is mandatory and must be set before initialization.
 *
 * \note
 * Unlike the FLAC__FileDecoder, the filename does not interpret "-" for
 * \c stdout; writing to \c stdout is not relevant in the file encoder.
 *
 * \default \c NULL
 * \param  encoder  A encoder instance to set.
 * \param  value    The output file name.
 * \assert
 *    \code encoder != NULL \endcode
 *    \code value != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, or there was a memory
 *    allocation error, else \c true.
 */
FLAC__bool FLAC__file_encoder_set_filename(FLAC__FileEncoder *encoder, const char *value);

/** Get the current encoder state.
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__FileEncoderState
 *    The current encoder state.
 */
FLAC__FileEncoderState FLAC__file_encoder_get_state(const FLAC__FileEncoder *encoder);

/** Get the state of the underlying seekable stream encoder.
 *  Useful when the file encoder state is
 *  \c FLAC__FILE_ENCODER_SEEKABLE_STREAM_ENCODER_ERROR.
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__SeekableStreamEncoderState
 *    The seekable stream encoder state.
 */
FLAC__SeekableStreamEncoderState FLAC__file_encoder_get_seekable_stream_encoder_state(const FLAC__FileEncoder *encoder);

/** Get the "streamable subset" flag.
 *  This is inherited from FLAC__SeekableStreamEncoder; see
 *  FLAC__seekable_stream_encoder_get_streamable_subset().
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    See FLAC__file_encoder_set_streamable_subset().
 */
FLAC__bool FLAC__file_encoder_get_streamable_subset(const FLAC__FileEncoder *encoder);

/** Get the "mid/side stereo coding" flag.
 *  This is inherited from FLAC__SeekableStreamEncoder; see
 *  FLAC__seekable_stream_encoder_get_do_mid_side_stereo().
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    See FLAC__file_encoder_get_do_mid_side_stereo().
 */
FLAC__bool FLAC__file_encoder_get_do_mid_side_stereo(const FLAC__FileEncoder *encoder);

/** Get the "adaptive mid/side switching" flag.
 *  This is inherited from FLAC__SeekableStreamEncoder; see
 *  FLAC__seekable_stream_encoder_get_loose_mid_side_stereo().
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    See FLAC__file_encoder_set_loose_mid_side_stereo().
 */
FLAC__bool FLAC__file_encoder_get_loose_mid_side_stereo(const FLAC__FileEncoder *encoder);

/** Get the number of input channels being processed.
 *  This is inherited from FLAC__SeekableStreamEncoder; see
 *  FLAC__seekable_stream_encoder_get_channels().
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    See FLAC__file_encoder_set_channels().
 */
unsigned FLAC__file_encoder_get_channels(const FLAC__FileEncoder *encoder);

/** Get the input sample resolution setting.
 *  This is inherited from FLAC__SeekableStreamEncoder; see
 *  FLAC__seekable_stream_encoder_get_bits_per_sample().
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    See FLAC__file_encoder_set_bits_per_sample().
 */
unsigned FLAC__file_encoder_get_bits_per_sample(const FLAC__FileEncoder *encoder);

/** Get the input sample rate setting.
 *  This is inherited from FLAC__SeekableStreamEncoder; see
 *  FLAC__seekable_stream_encoder_get_sample_rate().
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    See FLAC__file_encoder_set_sample_rate().
 */
unsigned FLAC__file_encoder_get_sample_rate(const FLAC__FileEncoder *encoder);

/** Get the blocksize setting.
 *  This is inherited from FLAC__SeekableStreamEncoder; see
 *  FLAC__seekable_stream_encoder_get_blocksize().
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    See FLAC__file_encoder_set_blocksize().
 */
unsigned FLAC__file_encoder_get_blocksize(const FLAC__FileEncoder *encoder);

/** Get the maximum LPC order setting.
 *  This is inherited from FLAC__SeekableStreamEncoder; see
 *  FLAC__seekable_stream_encoder_get_max_lpc_order().
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    See FLAC__file_encoder_set_max_lpc_order().
 */
unsigned FLAC__file_encoder_get_max_lpc_order(const FLAC__FileEncoder *encoder);

/** Get the quantized linear predictor coefficient precision setting.
 *  This is inherited from FLAC__SeekableStreamEncoder; see
 *  FLAC__seekable_stream_encoder_get_qlp_coeff_precision().
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    See FLAC__file_encoder_set_qlp_coeff_precision().
 */
unsigned FLAC__file_encoder_get_qlp_coeff_precision(const FLAC__FileEncoder *encoder);

/** Get the qlp coefficient precision search flag.
 *  This is inherited from FLAC__SeekableStreamEncoder; see
 *  FLAC__seekable_stream_encoder_get_do_qlp_coeff_prec_search().
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    See FLAC__file_encoder_set_do_qlp_coeff_prec_search().
 */
FLAC__bool FLAC__file_encoder_get_do_qlp_coeff_prec_search(const FLAC__FileEncoder *encoder);

/** Get the "escape coding" flag.
 *  This is inherited from FLAC__SeekableStreamEncoder; see
 *  FLAC__seekable_stream_encoder_get_do_escape_coding().
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    See FLAC__file_encoder_set_do_escape_coding().
 */
FLAC__bool FLAC__file_encoder_get_do_escape_coding(const FLAC__FileEncoder *encoder);

/** Get the exhaustive model search flag.
 *  This is inherited from FLAC__SeekableStreamEncoder; see
 *  FLAC__seekable_stream_encoder_get_do_exhaustive_model_search().
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    See FLAC__file_encoder_set_do_exhaustive_model_search().
 */
FLAC__bool FLAC__file_encoder_get_do_exhaustive_model_search(const FLAC__FileEncoder *encoder);

/** Get the minimum residual partition order setting.
 *  This is inherited from FLAC__SeekableStreamEncoder; see
 *  FLAC__seekable_stream_encoder_get_min_residual_partition_order().
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    See FLAC__file_encoder_set_min_residual_partition_order().
 */
unsigned FLAC__file_encoder_get_min_residual_partition_order(const FLAC__FileEncoder *encoder);

/** Get maximum residual partition order setting.
 *  This is inherited from FLAC__SeekableStreamEncoder; see
 *  FLAC__seekable_stream_encoder_get_max_residual_partition_order().
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    See FLAC__file_encoder_set_max_residual_partition_order().
 */
unsigned FLAC__file_encoder_get_max_residual_partition_order(const FLAC__FileEncoder *encoder);

/** Get the Rice parameter search distance setting.
 *  This is inherited from FLAC__SeekableStreamEncoder; see
 *  FLAC__seekable_stream_encoder_get_rice_parameter_search_dist().
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    See FLAC__file_encoder_set_rice_parameter_search_dist().
 */
unsigned FLAC__file_encoder_get_rice_parameter_search_dist(const FLAC__FileEncoder *encoder);

/** Initialize the encoder instance.
 *  Should be called after FLAC__file_encoder_new() and
 *  FLAC__file_encoder_set_*() but before FLAC__file_encoder_process()
 *  or FLAC__file_encoder_process_interleaved().  Will set and return
 *  the encoder state, which will be FLAC__FILE_ENCODER_OK if
 *  initialization succeeded.
 *
 * \param  encoder  An uninitialized encoder instance.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__FileEncoderState
 *    \c FLAC__FILE_ENCODER_OK if initialization was successful; see
 *    FLAC__FileEncoderState for the meanings of other return values.
 */
FLAC__FileEncoderState FLAC__file_encoder_init(FLAC__FileEncoder *encoder);

/** Finish the encoding process.
 *  Flushes the encoding buffer, releases resources, resets the encoder
 *  settings to their defaults, and returns the encoder state to
 *  FLAC__FILE_ENCODER_UNINITIALIZED.
 *
 *  In the event of a prematurely-terminated encode, it is not strictly
 *  necessary to call this immediately before FLAC__file_encoder_delete()
 *  but it is good practice to match every FLAC__file_encoder_init()
 *  with a FLAC__file_encoder_finish().
 *
 * \param  encoder  An uninitialized encoder instance.
 * \assert
 *    \code encoder != NULL \endcode
 */
void FLAC__file_encoder_finish(FLAC__FileEncoder *encoder);

/** Submit data for encoding.
 *  This is inherited from FLAC__SeekableStreamEncoder; see
 *  FLAC__seekable_stream_encoder_process().
 *
 * \param  encoder  An initialized encoder instance in the OK state.
 * \param  buffer   An array of pointers to each channel's signal.
 * \param  samples  The number of samples in one channel.
 * \assert
 *    \code encoder != NULL \endcode
 *    \code FLAC__file_encoder_get_state(encoder) == FLAC__FILE_ENCODER_OK \endcode
 * \retval FLAC__bool
 *    \c true if successful, else \c false; in this case, check the
 *    encoder state with FLAC__file_encoder_get_state() to see what
 *    went wrong.
 */
FLAC__bool FLAC__file_encoder_process(FLAC__FileEncoder *encoder, const FLAC__int32 * const buffer[], unsigned samples);

/** Submit data for encoding.
 *  This is inherited from FLAC__SeekableStreamEncoder; see
 *  FLAC__seekable_stream_encoder_process_interleaved().
 *
 * \param  encoder  An initialized encoder instance in the OK state.
 * \param  buffer   An array of channel-interleaved data (see above).
 * \param  samples  The number of samples in one channel, the same as for
 *                  FLAC__file_encoder_process().  For example, if
 *                  encoding two channels, \c 1000 \a samples corresponds
 *                  to a \a buffer of 2000 values.
 * \assert
 *    \code encoder != NULL \endcode
 *    \code FLAC__file_encoder_get_state(encoder) == FLAC__FILE_ENCODER_OK \endcode
 * \retval FLAC__bool
 *    \c true if successful, else \c false; in this case, check the
 *    encoder state with FLAC__file_encoder_get_state() to see what
 *    went wrong.
 */
FLAC__bool FLAC__file_encoder_process_interleaved(FLAC__FileEncoder *encoder, const FLAC__int32 buffer[], unsigned samples);

/* \} */

#ifdef __cplusplus
}
#endif

#endif
