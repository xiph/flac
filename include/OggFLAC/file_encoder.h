/* libOggFLAC - Free Lossless Audio Codec + Ogg library
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

#ifndef OggFLAC__FILE_ENCODER_H
#define OggFLAC__FILE_ENCODER_H

#include "FLAC/file_encoder.h"

#ifdef __cplusplus
extern "C" {
#endif


/** \file include/OggFLAC/file_encoder.h
 *
 *  \brief
 *  This module contains the functions which implement the file
 *  encoder.
 *
 *  See the detailed documentation in the
 *  \link oggflac_file_encoder file encoder \endlink module.
 */

/** \defgroup oggflac_file_encoder OggFLAC/file_encoder.h: file encoder interface
 *  \ingroup oggflac_encoder
 *
 *  \brief
 *  This module contains the functions which implement the file
 *  encoder.
 *
 * The basic usage of this encoder is as follows:
 * - The program creates an instance of an encoder using
 *   OggFLAC__file_encoder_new().
 * - The program overrides the default settings using
 *   OggFLAC__file_encoder_set_*() functions.
 * - The program initializes the instance to validate the settings and
 *   prepare for encoding using OggFLAC__file_encoder_init().
 * - The program calls OggFLAC__file_encoder_process() or
 *   OggFLAC__file_encoder_process_interleaved() to encode data, which
 *   subsequently writes data to the output file.
 * - The program finishes the encoding with OggFLAC__file_encoder_finish(),
 *   which causes the encoder to encode any data still in its input pipe,
 *   rewind and write the STREAMINFO metadata to file, and finally reset
 *   the encoder to the uninitialized state.
 * - The instance may be used again or deleted with
 *   OggFLAC__file_encoder_delete().
 *
 * The file encoder is a wrapper around the
 * \link oggflac_seekable_stream_encoder seekable stream encoder \endlink which supplies all
 * callbacks internally; the user need specify only the filename.
 *
 * Make sure to read the detailed description of the
 * \link oggflac_seekable_stream_encoder seekable stream encoder module \endlink since the
 * \link oggflac_stream_encoder stream encoder module \endlink since the
 * file encoder inherits much of its behavior from them.
 *
 * \note
 * The "set" functions may only be called when the encoder is in the
 * state OggFLAC__FILE_ENCODER_UNINITIALIZED, i.e. after
 * OggFLAC__file_encoder_new() or OggFLAC__file_encoder_finish(), but
 * before OggFLAC__file_encoder_init().  If this is the case they will
 * return \c true, otherwise \c false.
 *
 * \note
 * OggFLAC__file_encoder_finish() resets all settings to the constructor
 * defaults.
 *
 * \{
 */


/** State values for a OggFLAC__FileEncoder
 *
 *  The encoder's state can be obtained by calling OggFLAC__file_encoder_get_state().
 */
typedef enum {

	OggFLAC__FILE_ENCODER_OK = 0,
	/**< The encoder is in the normal OK state. */

	OggFLAC__FILE_ENCODER_NO_FILENAME,
	/**< OggFLAC__file_encoder_init() was called without first calling
	 * OggFLAC__file_encoder_set_filename().
	 */

	OggFLAC__FILE_ENCODER_SEEKABLE_STREAM_ENCODER_ERROR,
	/**< An error occurred in the underlying seekable stream encoder;
	 * check OggFLAC__file_encoder_get_seekable_stream_encoder_state().
	 */

	OggFLAC__FILE_ENCODER_FATAL_ERROR_WHILE_WRITING,
	/**< A fatal error occurred while writing to the encoded file. */

    OggFLAC__FILE_ENCODER_ERROR_OPENING_FILE,
	/**< An error occurred opening the output file for writing. */

	OggFLAC__FILE_ENCODER_MEMORY_ALLOCATION_ERROR,
	/**< Memory allocation failed. */

	OggFLAC__FILE_ENCODER_ALREADY_INITIALIZED,
	/**< OggFLAC__file_encoder_init() was called when the encoder was
	 * already initialized, usually because
	 * OggFLAC__file_encoder_finish() was not called.
	 */

	OggFLAC__FILE_ENCODER_UNINITIALIZED
	/**< The encoder is in the uninitialized state. */

} OggFLAC__FileEncoderState;

/** Maps a OggFLAC__FileEncoderState to a C string.
 *
 *  Using a OggFLAC__FileEncoderState as the index to this array
 *  will give the string equivalent.  The contents should not be modified.
 */
/* @@@@ double-check mapping */
extern const char * const OggFLAC__FileEncoderStateString[];


/***********************************************************************
 *
 * class OggFLAC__FileEncoder
 *
 ***********************************************************************/

struct OggFLAC__FileEncoderProtected;
struct OggFLAC__FileEncoderPrivate;
/** The opaque structure definition for the file encoder type.
 *  See the \link oggflac_file_encoder file encoder module \endlink
 *  for a detailed description.
 */
typedef struct {
	struct OggFLAC__FileEncoderProtected *protected_; /* avoid the C++ keyword 'protected' */
	struct OggFLAC__FileEncoderPrivate *private_; /* avoid the C++ keyword 'private' */
} OggFLAC__FileEncoder;

/*@@@ document: */
typedef void (*OggFLAC__FileEncoderProgressCallback)(const OggFLAC__FileEncoder *encoder, unsigned current_frame, unsigned total_frames_estimate, void *client_data);


/***********************************************************************
 *
 * Class constructor/destructor
 *
 ***********************************************************************/

/** Create a new file encoder instance.  The instance is created with
 *  default settings; see the individual OggFLAC__file_encoder_set_*()
 *  functions for each setting's default.
 *
 * \retval OggFLAC__FileEncoder*
 *    \c NULL if there was an error allocating memory, else the new instance.
 */
OggFLAC__FileEncoder *OggFLAC__file_encoder_new();

/** Free an encoder instance.  Deletes the object pointed to by \a encoder.
 *
 * \param encoder  A pointer to an existing encoder.
 * \assert
 *    \code encoder != NULL \endcode
 */
void OggFLAC__file_encoder_delete(OggFLAC__FileEncoder *encoder);

/***********************************************************************
 *
 * Public class method prototypes
 *
 ***********************************************************************/

/** This is inherited from OggFLAC__SeekableStreamEncoder; see
 *  OggFLAC__seekable_stream_encoder_set_verify().
 *
 * \default \c true
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC__bool OggFLAC__file_encoder_set_verify(OggFLAC__FileEncoder *encoder, FLAC__bool value);

/** This is inherited from OggFLAC__SeekableStreamEncoder; see
 *  OggFLAC__seekable_stream_encoder_set_streamable_subset().
 *
 * \default \c true
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC__bool OggFLAC__file_encoder_set_streamable_subset(OggFLAC__FileEncoder *encoder, FLAC__bool value);

/** This is inherited from OggFLAC__SeekableStreamEncoder; see
 *  OggFLAC__seekable_stream_encoder_set_do_mid_side_stereo().
 *
 * \default \c false
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC__bool OggFLAC__file_encoder_set_do_mid_side_stereo(OggFLAC__FileEncoder *encoder, FLAC__bool value);

/** This is inherited from OggFLAC__SeekableStreamEncoder; see
 *  OggFLAC__seekable_stream_encoder_set_loose_mid_side_stereo().
 *
 * \default \c false
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC__bool OggFLAC__file_encoder_set_loose_mid_side_stereo(OggFLAC__FileEncoder *encoder, FLAC__bool value);

/** This is inherited from OggFLAC__SeekableStreamEncoder; see
 *  OggFLAC__seekable_stream_encoder_set_channels().
 *
 * \default \c 2
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC__bool OggFLAC__file_encoder_set_channels(OggFLAC__FileEncoder *encoder, unsigned value);

/** This is inherited from OggFLAC__SeekableStreamEncoder; see
 *  OggFLAC__seekable_stream_encoder_set_bits_per_sample().
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
FLAC__bool OggFLAC__file_encoder_set_bits_per_sample(OggFLAC__FileEncoder *encoder, unsigned value);

/** This is inherited from OggFLAC__SeekableStreamEncoder; see
 *  OggFLAC__seekable_stream_encoder_set_sample_rate().
 *
 * \default \c 44100
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC__bool OggFLAC__file_encoder_set_sample_rate(OggFLAC__FileEncoder *encoder, unsigned value);

/** This is inherited from OggFLAC__SeekableStreamEncoder; see
 *  OggFLAC__seekable_stream_encoder_set_blocksize().
 *
 * \default \c 1152
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC__bool OggFLAC__file_encoder_set_blocksize(OggFLAC__FileEncoder *encoder, unsigned value);

/** This is inherited from OggFLAC__SeekableStreamEncoder; see
 *  OggFLAC__seekable_stream_encoder_set_max_lpc_order().
 *
 * \default \c 0
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC__bool OggFLAC__file_encoder_set_max_lpc_order(OggFLAC__FileEncoder *encoder, unsigned value);

/** This is inherited from OggFLAC__SeekableStreamEncoder; see
 *  OggFLAC__seekable_stream_encoder_set_qlp_coeff_precision().
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
FLAC__bool OggFLAC__file_encoder_set_qlp_coeff_precision(OggFLAC__FileEncoder *encoder, unsigned value);

/** This is inherited from OggFLAC__SeekableStreamEncoder; see
 *  OggFLAC__seekable_stream_encoder_set_do_qlp_coeff_prec_search().
 *
 * \default \c false
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC__bool OggFLAC__file_encoder_set_do_qlp_coeff_prec_search(OggFLAC__FileEncoder *encoder, FLAC__bool value);

/** This is inherited from OggFLAC__SeekableStreamEncoder; see
 *  OggFLAC__seekable_stream_encoder_set_do_escape_coding().
 *
 * \default \c false
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC__bool OggFLAC__file_encoder_set_do_escape_coding(OggFLAC__FileEncoder *encoder, FLAC__bool value);

/** This is inherited from OggFLAC__SeekableStreamEncoder; see
 *  OggFLAC__seekable_stream_encoder_set_do_exhaustive_model_search().
 *
 * \default \c false
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC__bool OggFLAC__file_encoder_set_do_exhaustive_model_search(OggFLAC__FileEncoder *encoder, FLAC__bool value);

/** This is inherited from OggFLAC__SeekableStreamEncoder; see
 *  OggFLAC__seekable_stream_encoder_set_min_residual_partition_order().
 *
 * \default \c 0
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC__bool OggFLAC__file_encoder_set_min_residual_partition_order(OggFLAC__FileEncoder *encoder, unsigned value);

/** This is inherited from OggFLAC__SeekableStreamEncoder; see
 *  OggFLAC__seekable_stream_encoder_set_max_residual_partition_order().
 *
 * \default \c 0
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC__bool OggFLAC__file_encoder_set_max_residual_partition_order(OggFLAC__FileEncoder *encoder, unsigned value);

/** This is inherited from OggFLAC__SeekableStreamEncoder; see
 *  OggFLAC__seekable_stream_encoder_set_rice_parameter_search_dist().
 *
 * \default \c 0
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC__bool OggFLAC__file_encoder_set_rice_parameter_search_dist(OggFLAC__FileEncoder *encoder, unsigned value);

/** This is inherited from OggFLAC__SeekableStreamEncoder; see
 *  OggFLAC__seekable_stream_encoder_set_total_samples_estimate().
 *
 * \default \c 0
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC__bool OggFLAC__file_encoder_set_total_samples_estimate(OggFLAC__FileEncoder *encoder, FLAC__uint64 value);

/** This is inherited from OggFLAC__SeekableStreamEncoder; see
 *  OggFLAC__seekable_stream_encoder_set_metadata().
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
FLAC__bool OggFLAC__file_encoder_set_metadata(OggFLAC__FileEncoder *encoder, OggFLAC__StreamMetadata **metadata, unsigned num_blocks);

/** Set the output file name encode to.
 *
 * \note
 * The filename is mandatory and must be set before initialization.
 *
 * \note
 * Unlike the OggFLAC__FileDecoder, the filename does not interpret "-" for
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
FLAC__bool OggFLAC__file_encoder_set_filename(OggFLAC__FileEncoder *encoder, const char *value);

/** Set the progress callback.
 *  The supplied function will be called when the encoder has finished
 *  writing a frame.  The \c total_frames_estimate argument to the callback
 *  will be based on the value from
 *  OggFLAC__file_encoder_set_total_samples_estimate().
 *
 * \note
 * Unlike most other callbacks, the progress callback is \b not mandatory
 * and need not be set before initialization.
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
FLAC__bool OggFLAC__file_encoder_set_progress_callback(OggFLAC__FileEncoder *encoder, OggFLAC__FileEncoderProgressCallback value);

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
FLAC__bool OggFLAC__file_encoder_set_client_data(OggFLAC__FileEncoder *encoder, void *value);

/** Get the current encoder state.
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval OggFLAC__FileEncoderState
 *    The current encoder state.
 */
OggFLAC__FileEncoderState OggFLAC__file_encoder_get_state(const OggFLAC__FileEncoder *encoder);

/** Get the state of the underlying seekable stream encoder.
 *  Useful when the file encoder state is
 *  \c OggFLAC__FILE_ENCODER_SEEKABLE_STREAM_ENCODER_ERROR.
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval OggFLAC__SeekableStreamEncoderState
 *    The seekable stream encoder state.
 */
OggFLAC__SeekableStreamEncoderState OggFLAC__file_encoder_get_seekable_stream_encoder_state(const OggFLAC__FileEncoder *encoder);

/** Get the state of the underlying stream encoder.
 *  Useful when the file encoder state is
 *  \c OggFLAC__FILE_ENCODER_SEEKABLE_STREAM_ENCODER_ERROR and the seekable stream
 *  encoder state is \c OggFLAC__SEEKABLE_STREAM_ENCODER_STREAM_ENCODER_ERROR.
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval OggFLAC__SeekableStreamEncoderState
 *    The seekable stream encoder state.
 */
OggFLAC__StreamEncoderState OggFLAC__file_encoder_get_stream_encoder_state(const OggFLAC__FileEncoder *encoder);

/** Get the state of the underlying stream encoder's verify decoder.
 *  Useful when the file encoder state is
 *  \c OggFLAC__FILE_ENCODER_SEEKABLE_STREAM_ENCODER_ERROR and the seekable stream
 *  encoder state is \c OggFLAC__SEEKABLE_STREAM_ENCODER_STREAM_ENCODER_ERROR and
 *  the stream encoder state is \c OggFLAC__STREAM_ENCODER_VERIFY_DECODER_ERROR.
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval OggFLAC__StreamEncoderState
 *    The stream encoder state.
 */
OggFLAC__StreamDecoderState OggFLAC__file_encoder_get_verify_decoder_state(const OggFLAC__FileEncoder *encoder);

/** Get the "verify" flag.
 *  This is inherited from OggFLAC__SeekableStreamEncoder; see
 *  OggFLAC__seekable_stream_encoder_get_verify().
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    See OggFLAC__file_encoder_set_verify().
 */
FLAC__bool OggFLAC__file_encoder_get_verify(const OggFLAC__FileEncoder *encoder);

/** Get the "streamable subset" flag.
 *  This is inherited from OggFLAC__SeekableStreamEncoder; see
 *  OggFLAC__seekable_stream_encoder_get_streamable_subset().
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    See OggFLAC__file_encoder_set_streamable_subset().
 */
FLAC__bool OggFLAC__file_encoder_get_streamable_subset(const OggFLAC__FileEncoder *encoder);

/** Get the "mid/side stereo coding" flag.
 *  This is inherited from OggFLAC__SeekableStreamEncoder; see
 *  OggFLAC__seekable_stream_encoder_get_do_mid_side_stereo().
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    See OggFLAC__file_encoder_get_do_mid_side_stereo().
 */
FLAC__bool OggFLAC__file_encoder_get_do_mid_side_stereo(const OggFLAC__FileEncoder *encoder);

/** Get the "adaptive mid/side switching" flag.
 *  This is inherited from OggFLAC__SeekableStreamEncoder; see
 *  OggFLAC__seekable_stream_encoder_get_loose_mid_side_stereo().
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    See OggFLAC__file_encoder_set_loose_mid_side_stereo().
 */
FLAC__bool OggFLAC__file_encoder_get_loose_mid_side_stereo(const OggFLAC__FileEncoder *encoder);

/** Get the number of input channels being processed.
 *  This is inherited from OggFLAC__SeekableStreamEncoder; see
 *  OggFLAC__seekable_stream_encoder_get_channels().
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval unsigned
 *    See OggFLAC__file_encoder_set_channels().
 */
unsigned OggFLAC__file_encoder_get_channels(const OggFLAC__FileEncoder *encoder);

/** Get the input sample resolution setting.
 *  This is inherited from OggFLAC__SeekableStreamEncoder; see
 *  OggFLAC__seekable_stream_encoder_get_bits_per_sample().
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval unsigned
 *    See OggFLAC__file_encoder_set_bits_per_sample().
 */
unsigned OggFLAC__file_encoder_get_bits_per_sample(const OggFLAC__FileEncoder *encoder);

/** Get the input sample rate setting.
 *  This is inherited from OggFLAC__SeekableStreamEncoder; see
 *  OggFLAC__seekable_stream_encoder_get_sample_rate().
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval unsigned
 *    See OggFLAC__file_encoder_set_sample_rate().
 */
unsigned OggFLAC__file_encoder_get_sample_rate(const OggFLAC__FileEncoder *encoder);

/** Get the blocksize setting.
 *  This is inherited from OggFLAC__SeekableStreamEncoder; see
 *  OggFLAC__seekable_stream_encoder_get_blocksize().
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval unsigned
 *    See OggFLAC__file_encoder_set_blocksize().
 */
unsigned OggFLAC__file_encoder_get_blocksize(const OggFLAC__FileEncoder *encoder);

/** Get the maximum LPC order setting.
 *  This is inherited from OggFLAC__SeekableStreamEncoder; see
 *  OggFLAC__seekable_stream_encoder_get_max_lpc_order().
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval unsigned
 *    See OggFLAC__file_encoder_set_max_lpc_order().
 */
unsigned OggFLAC__file_encoder_get_max_lpc_order(const OggFLAC__FileEncoder *encoder);

/** Get the quantized linear predictor coefficient precision setting.
 *  This is inherited from OggFLAC__SeekableStreamEncoder; see
 *  OggFLAC__seekable_stream_encoder_get_qlp_coeff_precision().
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval unsigned
 *    See OggFLAC__file_encoder_set_qlp_coeff_precision().
 */
unsigned OggFLAC__file_encoder_get_qlp_coeff_precision(const OggFLAC__FileEncoder *encoder);

/** Get the qlp coefficient precision search flag.
 *  This is inherited from OggFLAC__SeekableStreamEncoder; see
 *  OggFLAC__seekable_stream_encoder_get_do_qlp_coeff_prec_search().
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    See OggFLAC__file_encoder_set_do_qlp_coeff_prec_search().
 */
FLAC__bool OggFLAC__file_encoder_get_do_qlp_coeff_prec_search(const OggFLAC__FileEncoder *encoder);

/** Get the "escape coding" flag.
 *  This is inherited from OggFLAC__SeekableStreamEncoder; see
 *  OggFLAC__seekable_stream_encoder_get_do_escape_coding().
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    See OggFLAC__file_encoder_set_do_escape_coding().
 */
FLAC__bool OggFLAC__file_encoder_get_do_escape_coding(const OggFLAC__FileEncoder *encoder);

/** Get the exhaustive model search flag.
 *  This is inherited from OggFLAC__SeekableStreamEncoder; see
 *  OggFLAC__seekable_stream_encoder_get_do_exhaustive_model_search().
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    See OggFLAC__file_encoder_set_do_exhaustive_model_search().
 */
FLAC__bool OggFLAC__file_encoder_get_do_exhaustive_model_search(const OggFLAC__FileEncoder *encoder);

/** Get the minimum residual partition order setting.
 *  This is inherited from OggFLAC__SeekableStreamEncoder; see
 *  OggFLAC__seekable_stream_encoder_get_min_residual_partition_order().
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval unsigned
 *    See OggFLAC__file_encoder_set_min_residual_partition_order().
 */
unsigned OggFLAC__file_encoder_get_min_residual_partition_order(const OggFLAC__FileEncoder *encoder);

/** Get maximum residual partition order setting.
 *  This is inherited from OggFLAC__SeekableStreamEncoder; see
 *  OggFLAC__seekable_stream_encoder_get_max_residual_partition_order().
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval unsigned
 *    See OggFLAC__file_encoder_set_max_residual_partition_order().
 */
unsigned OggFLAC__file_encoder_get_max_residual_partition_order(const OggFLAC__FileEncoder *encoder);

/** Get the Rice parameter search distance setting.
 *  This is inherited from OggFLAC__SeekableStreamEncoder; see
 *  OggFLAC__seekable_stream_encoder_get_rice_parameter_search_dist().
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval unsigned
 *    See OggFLAC__file_encoder_set_rice_parameter_search_dist().
 */
unsigned OggFLAC__file_encoder_get_rice_parameter_search_dist(const OggFLAC__FileEncoder *encoder);

/** Get the previously set estimate of the total samples to be encoded.
 *  This is inherited from OggFLAC__SeekableStreamEncoder; see
 *  OggFLAC__seekable_stream_encoder_get_total_samples_estimate().
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__uint64
 *    See OggFLAC__file_encoder_set_total_samples_estimate().
 */
FLAC__uint64 OggFLAC__file_encoder_get_total_samples_estimate(const OggFLAC__FileEncoder *encoder);

/** Initialize the encoder instance.
 *  Should be called after OggFLAC__file_encoder_new() and
 *  OggFLAC__file_encoder_set_*() but before OggFLAC__file_encoder_process()
 *  or OggFLAC__file_encoder_process_interleaved().  Will set and return
 *  the encoder state, which will be OggFLAC__FILE_ENCODER_OK if
 *  initialization succeeded.
 *
 * \param  encoder  An uninitialized encoder instance.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval OggFLAC__FileEncoderState
 *    \c OggFLAC__FILE_ENCODER_OK if initialization was successful; see
 *    OggFLAC__FileEncoderState for the meanings of other return values.
 */
OggFLAC__FileEncoderState OggFLAC__file_encoder_init(OggFLAC__FileEncoder *encoder);

/** Finish the encoding process.
 *  Flushes the encoding buffer, releases resources, resets the encoder
 *  settings to their defaults, and returns the encoder state to
 *  OggFLAC__FILE_ENCODER_UNINITIALIZED.
 *
 *  In the event of a prematurely-terminated encode, it is not strictly
 *  necessary to call this immediately before OggFLAC__file_encoder_delete()
 *  but it is good practice to match every OggFLAC__file_encoder_init()
 *  with a OggFLAC__file_encoder_finish().
 *
 * \param  encoder  An uninitialized encoder instance.
 * \assert
 *    \code encoder != NULL \endcode
 */
void OggFLAC__file_encoder_finish(OggFLAC__FileEncoder *encoder);

/** Submit data for encoding.
 *  This is inherited from OggFLAC__SeekableStreamEncoder; see
 *  OggFLAC__seekable_stream_encoder_process().
 *
 * \param  encoder  An initialized encoder instance in the OK state.
 * \param  buffer   An array of pointers to each channel's signal.
 * \param  samples  The number of samples in one channel.
 * \assert
 *    \code encoder != NULL \endcode
 *    \code OggFLAC__file_encoder_get_state(encoder) == OggFLAC__FILE_ENCODER_OK \endcode
 * \retval FLAC__bool
 *    \c true if successful, else \c false; in this case, check the
 *    encoder state with OggFLAC__file_encoder_get_state() to see what
 *    went wrong.
 */
FLAC__bool OggFLAC__file_encoder_process(OggFLAC__FileEncoder *encoder, const FLAC__int32 * const buffer[], unsigned samples);

/** Submit data for encoding.
 *  This is inherited from OggFLAC__SeekableStreamEncoder; see
 *  OggFLAC__seekable_stream_encoder_process_interleaved().
 *
 * \param  encoder  An initialized encoder instance in the OK state.
 * \param  buffer   An array of channel-interleaved data (see above).
 * \param  samples  The number of samples in one channel, the same as for
 *                  OggFLAC__file_encoder_process().  For example, if
 *                  encoding two channels, \c 1000 \a samples corresponds
 *                  to a \a buffer of 2000 values.
 * \assert
 *    \code encoder != NULL \endcode
 *    \code OggFLAC__file_encoder_get_state(encoder) == OggFLAC__FILE_ENCODER_OK \endcode
 * \retval FLAC__bool
 *    \c true if successful, else \c false; in this case, check the
 *    encoder state with OggFLAC__file_encoder_get_state() to see what
 *    went wrong.
 */
FLAC__bool OggFLAC__file_encoder_process_interleaved(OggFLAC__FileEncoder *encoder, const FLAC__int32 buffer[], unsigned samples);

/* \} */

#ifdef __cplusplus
}
#endif

#endif
