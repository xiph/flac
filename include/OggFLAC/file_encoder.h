/* libOggFLAC - Free Lossless Audio Codec + Ogg library
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

#ifndef OggFLAC__FILE_ENCODER_H
#define OggFLAC__FILE_ENCODER_H

#include "export.h"
#include "seekable_stream_encoder.h"

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
 *  encoder.  Unlink the Ogg stream and seekable stream encoders, which
 *  derive from their FLAC counterparts, the Ogg file encoder is derived
 *  from the Ogg seekable stream encoder.
 *
 * The interface here is nearly identical to FLAC's file
 * encoder, including the callbacks, with the addition of
 * OggFLAC__file_encoder_set_serial_number().  See the
 * \link flac_file_encoder FLAC file encoder module \endlink
 * for full documentation.
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

/** Maps a FLAC__FileEncoderState to a C string.
 *
 *  Using a FLAC__FileEncoderState as the index to this array
 *  will give the string equivalent.  The contents should not be modified.
 */
extern OggFLAC_API const char * const OggFLAC__FileEncoderStateString[];


/***********************************************************************
 *
 * class FLAC__FileEncoder
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

/** Signature for the progress callback.
 *  See OggFLAC__file_encoder_set_progress_callback()
 *  and FLAC__FileEncoderProgressCallback for more info.
 *
 * \param  encoder          The encoder instance calling the callback.
 * \param  bytes_written    Bytes written so far.
 * \param  samples_written  Samples written so far.
 * \param  frames_written   Frames written so far.
 * \param  total_frames_estimate  The estimate of the total number of
 *                                frames to be written.
 * \param  client_data      The callee's client data set through
 *                          OggFLAC__file_encoder_set_client_data().
 */
typedef void (*OggFLAC__FileEncoderProgressCallback)(const OggFLAC__FileEncoder *encoder, FLAC__uint64 bytes_written, FLAC__uint64 samples_written, unsigned frames_written, unsigned total_frames_estimate, void *client_data);


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
OggFLAC_API OggFLAC__FileEncoder *OggFLAC__file_encoder_new();

/** Free an encoder instance.  Deletes the object pointed to by \a encoder.
 *
 * \param encoder  A pointer to an existing encoder.
 * \assert
 *    \code encoder != NULL \endcode
 */
OggFLAC_API void OggFLAC__file_encoder_delete(OggFLAC__FileEncoder *encoder);

/***********************************************************************
 *
 * Public class method prototypes
 *
 ***********************************************************************/

/** Set the serial number for the FLAC stream.
 *
 * \default \c 0
 * \param  encoder        An encoder instance to set.
 * \param  serial_number  See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
OggFLAC_API FLAC__bool OggFLAC__file_encoder_set_serial_number(OggFLAC__FileEncoder *encoder, long serial_number);

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
OggFLAC_API FLAC__bool OggFLAC__file_encoder_set_verify(OggFLAC__FileEncoder *encoder, FLAC__bool value);

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
OggFLAC_API FLAC__bool OggFLAC__file_encoder_set_streamable_subset(OggFLAC__FileEncoder *encoder, FLAC__bool value);

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
OggFLAC_API FLAC__bool OggFLAC__file_encoder_set_do_mid_side_stereo(OggFLAC__FileEncoder *encoder, FLAC__bool value);

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
OggFLAC_API FLAC__bool OggFLAC__file_encoder_set_loose_mid_side_stereo(OggFLAC__FileEncoder *encoder, FLAC__bool value);

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
OggFLAC_API FLAC__bool OggFLAC__file_encoder_set_channels(OggFLAC__FileEncoder *encoder, unsigned value);

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
OggFLAC_API FLAC__bool OggFLAC__file_encoder_set_bits_per_sample(OggFLAC__FileEncoder *encoder, unsigned value);

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
OggFLAC_API FLAC__bool OggFLAC__file_encoder_set_sample_rate(OggFLAC__FileEncoder *encoder, unsigned value);

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
OggFLAC_API FLAC__bool OggFLAC__file_encoder_set_blocksize(OggFLAC__FileEncoder *encoder, unsigned value);

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
OggFLAC_API FLAC__bool OggFLAC__file_encoder_set_max_lpc_order(OggFLAC__FileEncoder *encoder, unsigned value);

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
OggFLAC_API FLAC__bool OggFLAC__file_encoder_set_qlp_coeff_precision(OggFLAC__FileEncoder *encoder, unsigned value);

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
OggFLAC_API FLAC__bool OggFLAC__file_encoder_set_do_qlp_coeff_prec_search(OggFLAC__FileEncoder *encoder, FLAC__bool value);

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
OggFLAC_API FLAC__bool OggFLAC__file_encoder_set_do_escape_coding(OggFLAC__FileEncoder *encoder, FLAC__bool value);

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
OggFLAC_API FLAC__bool OggFLAC__file_encoder_set_do_exhaustive_model_search(OggFLAC__FileEncoder *encoder, FLAC__bool value);

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
OggFLAC_API FLAC__bool OggFLAC__file_encoder_set_min_residual_partition_order(OggFLAC__FileEncoder *encoder, unsigned value);

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
OggFLAC_API FLAC__bool OggFLAC__file_encoder_set_max_residual_partition_order(OggFLAC__FileEncoder *encoder, unsigned value);

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
OggFLAC_API FLAC__bool OggFLAC__file_encoder_set_rice_parameter_search_dist(OggFLAC__FileEncoder *encoder, unsigned value);

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
OggFLAC_API FLAC__bool OggFLAC__file_encoder_set_total_samples_estimate(OggFLAC__FileEncoder *encoder, FLAC__uint64 value);

/** This is inherited from OggFLAC__SeekableStreamEncoder; see
 *  OggFLAC__seekable_stream_encoder_set_metadata().
 *
 * \note The Ogg FLAC mapping requires that the VORBIS_COMMENT block be
 * the second metadata block of the stream.  The encoder already supplies
 * the STREAMINFO block automatically.  If \a metadata does not contain a
 * VORBIS_COMMENT block, the encoder will supply that too.  Otherwise, if
 * \a metadata does contain a VORBIS_COMMENT block and it is not the
 * first, this function will reorder \a metadata by moving the
 * VORBIS_COMMENT block to the front; the relative ordering of the other
 * blocks will remain as they were.
 *
 * \note The Ogg FLAC mapping limits the number of metadata blocks per
 * stream to \c 65535.  If \a num_blocks exceeds this the function will
 * return \c false.
 *
 * \default \c NULL, 0
 * \param  encoder     An encoder instance to set.
 * \param  metadata    See above.
 * \param  num_blocks  See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, or if
 *    \a num_blocks > 65535, else \c true.
 */
OggFLAC_API FLAC__bool OggFLAC__file_encoder_set_metadata(OggFLAC__FileEncoder *encoder, FLAC__StreamMetadata **metadata, unsigned num_blocks);

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
OggFLAC_API FLAC__bool OggFLAC__file_encoder_set_filename(OggFLAC__FileEncoder *encoder, const char *value);

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
OggFLAC_API FLAC__bool OggFLAC__file_encoder_set_progress_callback(OggFLAC__FileEncoder *encoder, OggFLAC__FileEncoderProgressCallback value);

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
OggFLAC_API FLAC__bool OggFLAC__file_encoder_set_client_data(OggFLAC__FileEncoder *encoder, void *value);

/** Get the current encoder state.
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__FileEncoderState
 *    The current encoder state.
 */
OggFLAC_API OggFLAC__FileEncoderState OggFLAC__file_encoder_get_state(const OggFLAC__FileEncoder *encoder);

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
OggFLAC_API OggFLAC__SeekableStreamEncoderState OggFLAC__file_encoder_get_seekable_stream_encoder_state(const OggFLAC__FileEncoder *encoder);

/** Get the state of the underlying FLAC stream encoder.
 *  Useful when the file encoder state is
 *  \c OggFLAC__FILE_ENCODER_SEEKABLE_STREAM_ENCODER_ERROR
 *  and the seekable stream encoder state is
 *  \c OggFLAC__SEEKABLE_STREAM_ENCODER_FLAC_STREAM_ENCODER_ERROR.
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__StreamEncoderState
 *    The seekable stream encoder state.
 */
OggFLAC_API FLAC__StreamEncoderState OggFLAC__file_encoder_get_FLAC_stream_encoder_state(const OggFLAC__FileEncoder *encoder);

/** Get the state of the underlying stream encoder's verify decoder.
 *  Useful when the file encoder state is
 *  \c OggFLAC__FILE_ENCODER_SEEKABLE_STREAM_ENCODER_ERROR
 *  and the seekable stream encoder state is
 *  \c OggFLAC__SEEKABLE_STREAM_ENCODER_FLAC_STREAM_ENCODER_ERROR
 *  and the FLAC stream encoder state is
 *  \c FLAC__STREAM_ENCODER_VERIFY_DECODER_ERROR.
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__StreamDecoderState
 *    The stream encoder state.
 */
OggFLAC_API FLAC__StreamDecoderState OggFLAC__file_encoder_get_verify_decoder_state(const OggFLAC__FileEncoder *encoder);

/** Get the current encoder state as a C string.
 *  This version automatically resolves
 *  \c OggFLAC__FILE_ENCODER_SEEKABLE_STREAM_ENCODER_ERROR by getting the
 *  seekable stream encoder's state.
 *
 * \param  encoder  A encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval const char *
 *    The encoder state as a C string.  Do not modify the contents.
 */
OggFLAC_API const char *OggFLAC__file_encoder_get_resolved_state_string(const OggFLAC__FileEncoder *encoder);

/** Get relevant values about the nature of a verify decoder error.
 *  Inherited from OggFLAC__seekable_stream_encoder_get_verify_decoder_error_stats().
 *  Useful when the file encoder state is
 *  \c OggFLAC__FILE_ENCODER_SEEKABLE_STREAM_ENCODER_ERROR
 *  and the seekable stream encoder state is
 *  \c OggFLAC__SEEKABLE_STREAM_ENCODER_FLAC_SEEKABLE_STREAM_ENCODER_ERROR
 *  and the FLAC seekable stream encoder state is
 *  \c FLAC__SEEKABLE_STREAM_ENCODER_STREAM_ENCODER_ERROR
 *  and the FLAC stream encoder state is
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
OggFLAC_API void OggFLAC__file_encoder_get_verify_decoder_error_stats(const OggFLAC__FileEncoder *encoder, FLAC__uint64 *absolute_sample, unsigned *frame_number, unsigned *channel, unsigned *sample, FLAC__int32 *expected, FLAC__int32 *got);

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
OggFLAC_API FLAC__bool OggFLAC__file_encoder_get_verify(const OggFLAC__FileEncoder *encoder);

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
OggFLAC_API FLAC__bool OggFLAC__file_encoder_get_streamable_subset(const OggFLAC__FileEncoder *encoder);

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
OggFLAC_API FLAC__bool OggFLAC__file_encoder_get_do_mid_side_stereo(const OggFLAC__FileEncoder *encoder);

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
OggFLAC_API FLAC__bool OggFLAC__file_encoder_get_loose_mid_side_stereo(const OggFLAC__FileEncoder *encoder);

/** Get the number of input channels being processed.
 *  This is inherited from OggFLAC__SeekableStreamEncoder; see
 *  OggFLAC__seekable_stream_encoder_get_channels().
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval unsigned
 *    See FLAC__file_encoder_set_channels().
 */
OggFLAC_API unsigned OggFLAC__file_encoder_get_channels(const OggFLAC__FileEncoder *encoder);

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
OggFLAC_API unsigned OggFLAC__file_encoder_get_bits_per_sample(const OggFLAC__FileEncoder *encoder);

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
OggFLAC_API unsigned OggFLAC__file_encoder_get_sample_rate(const OggFLAC__FileEncoder *encoder);

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
OggFLAC_API unsigned OggFLAC__file_encoder_get_blocksize(const OggFLAC__FileEncoder *encoder);

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
OggFLAC_API unsigned OggFLAC__file_encoder_get_max_lpc_order(const OggFLAC__FileEncoder *encoder);

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
OggFLAC_API unsigned OggFLAC__file_encoder_get_qlp_coeff_precision(const OggFLAC__FileEncoder *encoder);

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
OggFLAC_API FLAC__bool OggFLAC__file_encoder_get_do_qlp_coeff_prec_search(const OggFLAC__FileEncoder *encoder);

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
OggFLAC_API FLAC__bool OggFLAC__file_encoder_get_do_escape_coding(const OggFLAC__FileEncoder *encoder);

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
OggFLAC_API FLAC__bool OggFLAC__file_encoder_get_do_exhaustive_model_search(const OggFLAC__FileEncoder *encoder);

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
OggFLAC_API unsigned OggFLAC__file_encoder_get_min_residual_partition_order(const OggFLAC__FileEncoder *encoder);

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
OggFLAC_API unsigned OggFLAC__file_encoder_get_max_residual_partition_order(const OggFLAC__FileEncoder *encoder);

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
OggFLAC_API unsigned OggFLAC__file_encoder_get_rice_parameter_search_dist(const OggFLAC__FileEncoder *encoder);

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
OggFLAC_API FLAC__uint64 OggFLAC__file_encoder_get_total_samples_estimate(const OggFLAC__FileEncoder *encoder);

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
OggFLAC_API OggFLAC__FileEncoderState OggFLAC__file_encoder_init(OggFLAC__FileEncoder *encoder);

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
OggFLAC_API void OggFLAC__file_encoder_finish(OggFLAC__FileEncoder *encoder);

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
OggFLAC_API FLAC__bool OggFLAC__file_encoder_process(OggFLAC__FileEncoder *encoder, const FLAC__int32 * const buffer[], unsigned samples);

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
OggFLAC_API FLAC__bool OggFLAC__file_encoder_process_interleaved(OggFLAC__FileEncoder *encoder, const FLAC__int32 buffer[], unsigned samples);

/* \} */

#ifdef __cplusplus
}
#endif

#endif
