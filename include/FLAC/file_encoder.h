/* libFLAC - Free Lossless Audio Codec library
 * Copyright (C) 2002,2003,2004,2005  Josh Coalson
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * - Neither the name of the Xiph.org Foundation nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef FLAC__FILE_ENCODER_H
#define FLAC__FILE_ENCODER_H

#include "export.h"
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
extern FLAC_API const char * const FLAC__FileEncoderStateString[];


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

/** Signature for the progress callback.
 *  See FLAC__file_encoder_set_progress_callback() for more info.
 *
 * \param  encoder          The encoder instance calling the callback.
 * \param  bytes_written    Bytes written so far.
 * \param  samples_written  Samples written so far.
 * \param  frames_written   Frames written so far.
 * \param  total_frames_estimate  The estimate of the total number of
 *                                frames to be written.
 * \param  client_data      The callee's client data set through
 *                          FLAC__file_encoder_set_client_data().
 */
typedef void (*FLAC__FileEncoderProgressCallback)(const FLAC__FileEncoder *encoder, FLAC__uint64 bytes_written, FLAC__uint64 samples_written, unsigned frames_written, unsigned total_frames_estimate, void *client_data);


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
FLAC_API FLAC__FileEncoder *FLAC__file_encoder_new();

/** Free an encoder instance.  Deletes the object pointed to by \a encoder.
 *
 * \param encoder  A pointer to an existing encoder.
 * \assert
 *    \code encoder != NULL \endcode
 */
FLAC_API void FLAC__file_encoder_delete(FLAC__FileEncoder *encoder);

/***********************************************************************
 *
 * Public class method prototypes
 *
 ***********************************************************************/

/** This is inherited from FLAC__SeekableStreamEncoder; see
 *  FLAC__seekable_stream_encoder_set_verify().
 *
 * \default \c true
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC_API FLAC__bool FLAC__file_encoder_set_verify(FLAC__FileEncoder *encoder, FLAC__bool value);

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
FLAC_API FLAC__bool FLAC__file_encoder_set_streamable_subset(FLAC__FileEncoder *encoder, FLAC__bool value);

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
FLAC_API FLAC__bool FLAC__file_encoder_set_do_mid_side_stereo(FLAC__FileEncoder *encoder, FLAC__bool value);

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
FLAC_API FLAC__bool FLAC__file_encoder_set_loose_mid_side_stereo(FLAC__FileEncoder *encoder, FLAC__bool value);

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
FLAC_API FLAC__bool FLAC__file_encoder_set_channels(FLAC__FileEncoder *encoder, unsigned value);

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
FLAC_API FLAC__bool FLAC__file_encoder_set_bits_per_sample(FLAC__FileEncoder *encoder, unsigned value);

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
FLAC_API FLAC__bool FLAC__file_encoder_set_sample_rate(FLAC__FileEncoder *encoder, unsigned value);

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
FLAC_API FLAC__bool FLAC__file_encoder_set_blocksize(FLAC__FileEncoder *encoder, unsigned value);

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
FLAC_API FLAC__bool FLAC__file_encoder_set_max_lpc_order(FLAC__FileEncoder *encoder, unsigned value);

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
FLAC_API FLAC__bool FLAC__file_encoder_set_qlp_coeff_precision(FLAC__FileEncoder *encoder, unsigned value);

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
FLAC_API FLAC__bool FLAC__file_encoder_set_do_qlp_coeff_prec_search(FLAC__FileEncoder *encoder, FLAC__bool value);

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
FLAC_API FLAC__bool FLAC__file_encoder_set_do_escape_coding(FLAC__FileEncoder *encoder, FLAC__bool value);

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
FLAC_API FLAC__bool FLAC__file_encoder_set_do_exhaustive_model_search(FLAC__FileEncoder *encoder, FLAC__bool value);

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
FLAC_API FLAC__bool FLAC__file_encoder_set_min_residual_partition_order(FLAC__FileEncoder *encoder, unsigned value);

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
FLAC_API FLAC__bool FLAC__file_encoder_set_max_residual_partition_order(FLAC__FileEncoder *encoder, unsigned value);

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
FLAC_API FLAC__bool FLAC__file_encoder_set_rice_parameter_search_dist(FLAC__FileEncoder *encoder, unsigned value);

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
FLAC_API FLAC__bool FLAC__file_encoder_set_total_samples_estimate(FLAC__FileEncoder *encoder, FLAC__uint64 value);

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
FLAC_API FLAC__bool FLAC__file_encoder_set_metadata(FLAC__FileEncoder *encoder, FLAC__StreamMetadata **metadata, unsigned num_blocks);

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
FLAC_API FLAC__bool FLAC__file_encoder_set_filename(FLAC__FileEncoder *encoder, const char *value);

/** Set the progress callback.
 *  The supplied function will be called when the encoder has finished
 *  writing a frame.  The \c total_frames_estimate argument to the callback
 *  will be based on the value from
 *  FLAC__file_encoder_set_total_samples_estimate().
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
FLAC_API FLAC__bool FLAC__file_encoder_set_progress_callback(FLAC__FileEncoder *encoder, FLAC__FileEncoderProgressCallback value);

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
FLAC_API FLAC__bool FLAC__file_encoder_set_client_data(FLAC__FileEncoder *encoder, void *value);

/** Get the current encoder state.
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__FileEncoderState
 *    The current encoder state.
 */
FLAC_API FLAC__FileEncoderState FLAC__file_encoder_get_state(const FLAC__FileEncoder *encoder);

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
FLAC_API FLAC__SeekableStreamEncoderState FLAC__file_encoder_get_seekable_stream_encoder_state(const FLAC__FileEncoder *encoder);

/** Get the state of the underlying stream encoder.
 *  Useful when the file encoder state is
 *  \c FLAC__FILE_ENCODER_SEEKABLE_STREAM_ENCODER_ERROR and the seekable stream
 *  encoder state is \c FLAC__SEEKABLE_STREAM_ENCODER_STREAM_ENCODER_ERROR.
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__StreamEncoderState
 *    The seekable stream encoder state.
 */
FLAC_API FLAC__StreamEncoderState FLAC__file_encoder_get_stream_encoder_state(const FLAC__FileEncoder *encoder);

/** Get the state of the underlying stream encoder's verify decoder.
 *  Useful when the file encoder state is
 *  \c FLAC__FILE_ENCODER_SEEKABLE_STREAM_ENCODER_ERROR and the seekable stream
 *  encoder state is \c FLAC__SEEKABLE_STREAM_ENCODER_STREAM_ENCODER_ERROR and
 *  the stream encoder state is \c FLAC__STREAM_ENCODER_VERIFY_DECODER_ERROR.
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__StreamDecoderState
 *    The stream encoder state.
 */
FLAC_API FLAC__StreamDecoderState FLAC__file_encoder_get_verify_decoder_state(const FLAC__FileEncoder *encoder);

/** Get the current encoder state as a C string.
 *  This version automatically resolves
 *  \c FLAC__FILE_ENCODER_SEEKABLE_STREAM_ENCODER_ERROR by getting the
 *  seekable stream encoder's state.
 *
 * \param  encoder  A encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval const char *
 *    The encoder state as a C string.  Do not modify the contents.
 */
FLAC_API const char *FLAC__file_encoder_get_resolved_state_string(const FLAC__FileEncoder *encoder);

/** Get relevant values about the nature of a verify decoder error.
 *  Inherited from FLAC__seekable_stream_encoder_get_verify_decoder_error_stats().
 *  Useful when the file encoder state is
 *  \c FLAC__FILE_ENCODER_SEEKABLE_STREAM_ENCODER_ERROR and the seekable stream
 *  encoder state is
 *  \c FLAC__SEEKABLE_STREAM_ENCODER_STREAM_ENCODER_ERROR and the
 *  stream encoder state is
 *  \c FLAC__STREAM_ENCODER_VERIFY_DECODER_ERROR.
 *
 * \param  encoder  An encoder instance to query.
 * \param  absolute_sample  The absolute sample number of the mismatch.
 * \param  frame_number  The number of the frame in which the mismatch occurred.
 * \param  channel       The channel in which the mismatch occurred.
 * \param  sample        The number of the sample (relative to the frame) in
 *                       which the mismatch occurred.
 * \param  expected      The expected value for the sample in question.
 * \param  got           The actual value returned by the decoder.
 * \assert
 *    \code encoder != NULL \endcode
 */
FLAC_API void FLAC__file_encoder_get_verify_decoder_error_stats(const FLAC__FileEncoder *encoder, FLAC__uint64 *absolute_sample, unsigned *frame_number, unsigned *channel, unsigned *sample, FLAC__int32 *expected, FLAC__int32 *got);

/** Get the "verify" flag.
 *  This is inherited from FLAC__SeekableStreamEncoder; see
 *  FLAC__seekable_stream_encoder_get_verify().
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    See FLAC__file_encoder_set_verify().
 */
FLAC_API FLAC__bool FLAC__file_encoder_get_verify(const FLAC__FileEncoder *encoder);

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
FLAC_API FLAC__bool FLAC__file_encoder_get_streamable_subset(const FLAC__FileEncoder *encoder);

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
FLAC_API FLAC__bool FLAC__file_encoder_get_do_mid_side_stereo(const FLAC__FileEncoder *encoder);

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
FLAC_API FLAC__bool FLAC__file_encoder_get_loose_mid_side_stereo(const FLAC__FileEncoder *encoder);

/** Get the number of input channels being processed.
 *  This is inherited from FLAC__SeekableStreamEncoder; see
 *  FLAC__seekable_stream_encoder_get_channels().
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval unsigned
 *    See FLAC__file_encoder_set_channels().
 */
FLAC_API unsigned FLAC__file_encoder_get_channels(const FLAC__FileEncoder *encoder);

/** Get the input sample resolution setting.
 *  This is inherited from FLAC__SeekableStreamEncoder; see
 *  FLAC__seekable_stream_encoder_get_bits_per_sample().
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval unsigned
 *    See FLAC__file_encoder_set_bits_per_sample().
 */
FLAC_API unsigned FLAC__file_encoder_get_bits_per_sample(const FLAC__FileEncoder *encoder);

/** Get the input sample rate setting.
 *  This is inherited from FLAC__SeekableStreamEncoder; see
 *  FLAC__seekable_stream_encoder_get_sample_rate().
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval unsigned
 *    See FLAC__file_encoder_set_sample_rate().
 */
FLAC_API unsigned FLAC__file_encoder_get_sample_rate(const FLAC__FileEncoder *encoder);

/** Get the blocksize setting.
 *  This is inherited from FLAC__SeekableStreamEncoder; see
 *  FLAC__seekable_stream_encoder_get_blocksize().
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval unsigned
 *    See FLAC__file_encoder_set_blocksize().
 */
FLAC_API unsigned FLAC__file_encoder_get_blocksize(const FLAC__FileEncoder *encoder);

/** Get the maximum LPC order setting.
 *  This is inherited from FLAC__SeekableStreamEncoder; see
 *  FLAC__seekable_stream_encoder_get_max_lpc_order().
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval unsigned
 *    See FLAC__file_encoder_set_max_lpc_order().
 */
FLAC_API unsigned FLAC__file_encoder_get_max_lpc_order(const FLAC__FileEncoder *encoder);

/** Get the quantized linear predictor coefficient precision setting.
 *  This is inherited from FLAC__SeekableStreamEncoder; see
 *  FLAC__seekable_stream_encoder_get_qlp_coeff_precision().
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval unsigned
 *    See FLAC__file_encoder_set_qlp_coeff_precision().
 */
FLAC_API unsigned FLAC__file_encoder_get_qlp_coeff_precision(const FLAC__FileEncoder *encoder);

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
FLAC_API FLAC__bool FLAC__file_encoder_get_do_qlp_coeff_prec_search(const FLAC__FileEncoder *encoder);

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
FLAC_API FLAC__bool FLAC__file_encoder_get_do_escape_coding(const FLAC__FileEncoder *encoder);

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
FLAC_API FLAC__bool FLAC__file_encoder_get_do_exhaustive_model_search(const FLAC__FileEncoder *encoder);

/** Get the minimum residual partition order setting.
 *  This is inherited from FLAC__SeekableStreamEncoder; see
 *  FLAC__seekable_stream_encoder_get_min_residual_partition_order().
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval unsigned
 *    See FLAC__file_encoder_set_min_residual_partition_order().
 */
FLAC_API unsigned FLAC__file_encoder_get_min_residual_partition_order(const FLAC__FileEncoder *encoder);

/** Get maximum residual partition order setting.
 *  This is inherited from FLAC__SeekableStreamEncoder; see
 *  FLAC__seekable_stream_encoder_get_max_residual_partition_order().
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval unsigned
 *    See FLAC__file_encoder_set_max_residual_partition_order().
 */
FLAC_API unsigned FLAC__file_encoder_get_max_residual_partition_order(const FLAC__FileEncoder *encoder);

/** Get the Rice parameter search distance setting.
 *  This is inherited from FLAC__SeekableStreamEncoder; see
 *  FLAC__seekable_stream_encoder_get_rice_parameter_search_dist().
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval unsigned
 *    See FLAC__file_encoder_set_rice_parameter_search_dist().
 */
FLAC_API unsigned FLAC__file_encoder_get_rice_parameter_search_dist(const FLAC__FileEncoder *encoder);

/** Get the previously set estimate of the total samples to be encoded.
 *  This is inherited from FLAC__SeekableStreamEncoder; see
 *  FLAC__seekable_stream_encoder_get_total_samples_estimate().
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__uint64
 *    See FLAC__file_encoder_set_total_samples_estimate().
 */
FLAC_API FLAC__uint64 FLAC__file_encoder_get_total_samples_estimate(const FLAC__FileEncoder *encoder);

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
FLAC_API FLAC__FileEncoderState FLAC__file_encoder_init(FLAC__FileEncoder *encoder);

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
FLAC_API void FLAC__file_encoder_finish(FLAC__FileEncoder *encoder);

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
FLAC_API FLAC__bool FLAC__file_encoder_process(FLAC__FileEncoder *encoder, const FLAC__int32 * const buffer[], unsigned samples);

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
FLAC_API FLAC__bool FLAC__file_encoder_process_interleaved(FLAC__FileEncoder *encoder, const FLAC__int32 buffer[], unsigned samples);

/* \} */

#ifdef __cplusplus
}
#endif

#endif
