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

#ifndef FLAC__SEEKABLE_STREAM_ENCODER_H
#define FLAC__SEEKABLE_STREAM_ENCODER_H

#include "export.h"
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
 * The basic usage of this encoder is as follows:
 * - The program creates an instance of an encoder using
 *   FLAC__seekable_stream_encoder_new().
 * - The program overrides the default settings and sets callbacks using
 *   FLAC__seekable_stream_encoder_set_*() functions.
 * - The program initializes the instance to validate the settings and
 *   prepare for encoding using FLAC__seekable_stream_encoder_init().
 * - The program calls FLAC__seekable_stream_encoder_process() or
 *   FLAC__seekable_stream_encoder_process_interleaved() to encode data, which
 *   subsequently calls the callbacks when there is encoder data ready
 *   to be written.
 * - The program finishes the encoding with FLAC__seekable_stream_encoder_finish(),
 *   which causes the encoder to encode any data still in its input pipe,
 *   rewrite the metadata with the final encoding statistics, and finally
 *   reset the encoder to the uninitialized state.
 * - The instance may be used again or deleted with
 *   FLAC__seekable_stream_encoder_delete().
 *
 * The seekable stream encoder is a wrapper around the
 * \link flac_stream_encoder stream encoder \endlink with callbacks for
 * seeking the output and reporting the output stream position.  This
 * allows the encoder to go back and rewrite some of the metadata after
 * encoding if necessary, and provides the metadata callback of the stream
 * encoder internally.  However, you must provide seek and tell callbacks
 * (see FLAC__seekable_stream_encoder_set_seek_callback() and
 * FLAC__seekable_stream_encoder_set_tell_callback()).
 *
 * Make sure to read the detailed description of the
 * \link flac_stream_encoder stream encoder module \endlink since the
 * seekable stream encoder inherits much of its behavior.
 *
 * \note
 * If you are writing the FLAC data to a file, make sure it is open
 * for update (e.g. mode "w+" for stdio streams).  This is because after
 * the first encoding pass, the encoder will try to seek back to the
 * beginning of the stream, to the STREAMINFO block, to write some data
 * there.
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

	FLAC__SEEKABLE_STREAM_ENCODER_TELL_ERROR,
	/**< The tell callback returned an error. */

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
extern FLAC_API const char * const FLAC__SeekableStreamEncoderStateString[];


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
extern FLAC_API const char * const FLAC__SeekableStreamEncoderSeekStatusString[];


/** Return values for the FLAC__SeekableStreamEncoder tell callback.
 */
typedef enum {

	FLAC__SEEKABLE_STREAM_ENCODER_TELL_STATUS_OK,
	/**< The tell was OK and encoding can continue. */

	FLAC__SEEKABLE_STREAM_ENCODER_TELL_STATUS_ERROR
	/**< An unrecoverable error occurred.  The encoder will return from the process call. */

} FLAC__SeekableStreamEncoderTellStatus;

/** Maps a FLAC__SeekableStreamEncoderTellStatus to a C string.
 *
 *  Using a FLAC__SeekableStreamEncoderTellStatus as the index to this array
 *  will give the string equivalent.  The contents should not be modified.
 */
extern FLAC_API const char * const FLAC__SeekableStreamEncoderTellStatusString[];


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

/** Signature for the seek callback.
 *  See FLAC__seekable_stream_encoder_set_seek_callback() for more info.
 *
 * \param  encoder  The encoder instance calling the callback.
 * \param  absolute_byte_offset  The offset from the beginning of the stream
 *                               to seek to.
 * \param  client_data  The callee's client data set through
 *                      FLAC__seekable_stream_encoder_set_client_data().
 * \retval FLAC__SeekableStreamEncoderSeekStatus
 *    The callee's return status.
 */
typedef FLAC__SeekableStreamEncoderSeekStatus (*FLAC__SeekableStreamEncoderSeekCallback)(const FLAC__SeekableStreamEncoder *encoder, FLAC__uint64 absolute_byte_offset, void *client_data);

/** Signature for the tell callback.
 *  See FLAC__seekable_stream_encoder_set_tell_callback() for more info.
 *
 * \warning
 * The callback must return the true current byte offset of the output to
 * which the encoder is writing.  If you are buffering the output, make
 * sure and take this into account.  If you are writing directly to a
 * FILE* from your write callback, ftell() is sufficient.  If you are
 * writing directly to a file descriptor from your write callback, you
 * can use lseek(fd, SEEK_CUR, 0).  The encoder may later seek back to
 * these points to rewrite metadata after encoding.
 *
 * \param  encoder  The encoder instance calling the callback.
 * \param  absolute_byte_offset  The address at which to store the current
 *                               position of the output.
 * \param  client_data  The callee's client data set through
 *                      FLAC__seekable_stream_encoder_set_client_data().
 * \retval FLAC__SeekableStreamEncoderTellStatus
 *    The callee's return status.
 */
typedef FLAC__SeekableStreamEncoderTellStatus (*FLAC__SeekableStreamEncoderTellCallback)(const FLAC__SeekableStreamEncoder *encoder, FLAC__uint64 *absolute_byte_offset, void *client_data);

/** Signature for the write callback.
 *  See FLAC__seekable_stream_encoder_set_write_callback()
 *  and FLAC__StreamEncoderWriteCallback for more info.
 *
 * \param  encoder  The encoder instance calling the callback.
 * \param  buffer   An array of encoded data of length \a bytes.
 * \param  bytes    The byte length of \a buffer.
 * \param  samples  The number of samples encoded by \a buffer.
 *                  \c 0 has a special meaning; see
 *                  FLAC__stream_encoder_set_write_callback().
 * \param  current_frame  The number of current frame being encoded.
 * \param  client_data  The callee's client data set through
 *                      FLAC__seekable_stream_encoder_set_client_data().
 * \retval FLAC__StreamEncoderWriteStatus
 *    The callee's return status.
 */
typedef FLAC__StreamEncoderWriteStatus (*FLAC__SeekableStreamEncoderWriteCallback)(const FLAC__SeekableStreamEncoder *encoder, const FLAC__byte buffer[], unsigned bytes, unsigned samples, unsigned current_frame, void *client_data);


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
FLAC_API FLAC__SeekableStreamEncoder *FLAC__seekable_stream_encoder_new();

/** Free an encoder instance.  Deletes the object pointed to by \a encoder.
 *
 * \param encoder  A pointer to an existing encoder.
 * \assert
 *    \code encoder != NULL \endcode
 */
FLAC_API void FLAC__seekable_stream_encoder_delete(FLAC__SeekableStreamEncoder *encoder);

/***********************************************************************
 *
 * Public class method prototypes
 *
 ***********************************************************************/

/** This is inherited from FLAC__StreamEncoder; see
 *  FLAC__stream_encoder_set_verify().
 *
 * \default \c true
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC_API FLAC__bool FLAC__seekable_stream_encoder_set_verify(FLAC__SeekableStreamEncoder *encoder, FLAC__bool value);

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
FLAC_API FLAC__bool FLAC__seekable_stream_encoder_set_streamable_subset(FLAC__SeekableStreamEncoder *encoder, FLAC__bool value);

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
FLAC_API FLAC__bool FLAC__seekable_stream_encoder_set_do_mid_side_stereo(FLAC__SeekableStreamEncoder *encoder, FLAC__bool value);

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
FLAC_API FLAC__bool FLAC__seekable_stream_encoder_set_loose_mid_side_stereo(FLAC__SeekableStreamEncoder *encoder, FLAC__bool value);

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
FLAC_API FLAC__bool FLAC__seekable_stream_encoder_set_channels(FLAC__SeekableStreamEncoder *encoder, unsigned value);

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
FLAC_API FLAC__bool FLAC__seekable_stream_encoder_set_bits_per_sample(FLAC__SeekableStreamEncoder *encoder, unsigned value);

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
FLAC_API FLAC__bool FLAC__seekable_stream_encoder_set_sample_rate(FLAC__SeekableStreamEncoder *encoder, unsigned value);

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
FLAC_API FLAC__bool FLAC__seekable_stream_encoder_set_blocksize(FLAC__SeekableStreamEncoder *encoder, unsigned value);

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
FLAC_API FLAC__bool FLAC__seekable_stream_encoder_set_max_lpc_order(FLAC__SeekableStreamEncoder *encoder, unsigned value);

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
FLAC_API FLAC__bool FLAC__seekable_stream_encoder_set_qlp_coeff_precision(FLAC__SeekableStreamEncoder *encoder, unsigned value);

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
FLAC_API FLAC__bool FLAC__seekable_stream_encoder_set_do_qlp_coeff_prec_search(FLAC__SeekableStreamEncoder *encoder, FLAC__bool value);

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
FLAC_API FLAC__bool FLAC__seekable_stream_encoder_set_do_escape_coding(FLAC__SeekableStreamEncoder *encoder, FLAC__bool value);

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
FLAC_API FLAC__bool FLAC__seekable_stream_encoder_set_do_exhaustive_model_search(FLAC__SeekableStreamEncoder *encoder, FLAC__bool value);

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
FLAC_API FLAC__bool FLAC__seekable_stream_encoder_set_min_residual_partition_order(FLAC__SeekableStreamEncoder *encoder, unsigned value);

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
FLAC_API FLAC__bool FLAC__seekable_stream_encoder_set_max_residual_partition_order(FLAC__SeekableStreamEncoder *encoder, unsigned value);

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
FLAC_API FLAC__bool FLAC__seekable_stream_encoder_set_rice_parameter_search_dist(FLAC__SeekableStreamEncoder *encoder, unsigned value);

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
FLAC_API FLAC__bool FLAC__seekable_stream_encoder_set_total_samples_estimate(FLAC__SeekableStreamEncoder *encoder, FLAC__uint64 value);

/** This is inherited from FLAC__StreamEncoder; see
 *  FLAC__stream_encoder_set_metadata().
 *
 * \note
 * SEEKTABLE blocks are handled specially.  Since you will not know
 * the values for the seek point stream offsets, you should pass in
 * a SEEKTABLE 'template', that is, a SEEKTABLE object with the
 * required sample numbers (or placeholder points), with \c 0 for the
 * \a frame_samples and \a stream_offset fields for each point.  While
 * encoding, the encoder will fill them in for you and when encoding
 * is finished, it will seek back and write the real values into the
 * SEEKTABLE block in the stream.  There are helper routines for
 * manipulating seektable template blocks; see metadata.h:
 * FLAC__metadata_object_seektable_template_*().
 *
 * \note
 * The encoder instance \b will modify the first \c SEEKTABLE block
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
FLAC_API FLAC__bool FLAC__seekable_stream_encoder_set_metadata(FLAC__SeekableStreamEncoder *encoder, FLAC__StreamMetadata **metadata, unsigned num_blocks);

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
FLAC_API FLAC__bool FLAC__seekable_stream_encoder_set_seek_callback(FLAC__SeekableStreamEncoder *encoder, FLAC__SeekableStreamEncoderSeekCallback value);

/** Set the tell callback.
 *  The supplied function will be called when the encoder needs to know
 *  the current position of the output stream.
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
FLAC_API FLAC__bool FLAC__seekable_stream_encoder_set_tell_callback(FLAC__SeekableStreamEncoder *encoder, FLAC__SeekableStreamEncoderTellCallback value);

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
FLAC_API FLAC__bool FLAC__seekable_stream_encoder_set_write_callback(FLAC__SeekableStreamEncoder *encoder, FLAC__SeekableStreamEncoderWriteCallback value);

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
FLAC_API FLAC__bool FLAC__seekable_stream_encoder_set_client_data(FLAC__SeekableStreamEncoder *encoder, void *value);

/** Get the current encoder state.
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__SeekableStreamEncoderState
 *    The current encoder state.
 */
FLAC_API FLAC__SeekableStreamEncoderState FLAC__seekable_stream_encoder_get_state(const FLAC__SeekableStreamEncoder *encoder);

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
FLAC_API FLAC__StreamEncoderState FLAC__seekable_stream_encoder_get_stream_encoder_state(const FLAC__SeekableStreamEncoder *encoder);

/** Get the state of the underlying stream encoder's verify decoder.
 *  Useful when the seekable stream encoder state is
 *  \c FLAC__SEEKABLE_STREAM_ENCODER_STREAM_ENCODER_ERROR and the
 *  stream encoder state is \c FLAC__STREAM_ENCODER_VERIFY_DECODER_ERROR.
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__StreamDecoderState
 *    The stream encoder state.
 */
FLAC_API FLAC__StreamDecoderState FLAC__seekable_stream_encoder_get_verify_decoder_state(const FLAC__SeekableStreamEncoder *encoder);

/** Get the current encoder state as a C string.
 *  This version automatically resolves
 *  \c FLAC__SEEKABLE_STREAM_ENCODER_STREAM_ENCODER_ERROR by getting the
 *  stream encoder's state.
 *
 * \param  encoder  A encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval const char *
 *    The encoder state as a C string.  Do not modify the contents.
 */
FLAC_API const char *FLAC__seekable_stream_encoder_get_resolved_state_string(const FLAC__SeekableStreamEncoder *encoder);

/** Get relevant values about the nature of a verify decoder error.
 *  Inherited from FLAC__stream_encoder_get_verify_decoder_error_stats().
 *  Useful when the seekable stream encoder state is
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
FLAC_API void FLAC__seekable_stream_encoder_get_verify_decoder_error_stats(const FLAC__SeekableStreamEncoder *encoder, FLAC__uint64 *absolute_sample, unsigned *frame_number, unsigned *channel, unsigned *sample, FLAC__int32 *expected, FLAC__int32 *got);

/** Get the "verify" flag.
 *  This is inherited from FLAC__StreamEncoder; see
 *  FLAC__stream_encoder_get_verify().
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    See FLAC__seekable_stream_encoder_set_verify().
 */
FLAC_API FLAC__bool FLAC__seekable_stream_encoder_get_verify(const FLAC__SeekableStreamEncoder *encoder);

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
FLAC_API FLAC__bool FLAC__seekable_stream_encoder_get_streamable_subset(const FLAC__SeekableStreamEncoder *encoder);

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
FLAC_API FLAC__bool FLAC__seekable_stream_encoder_get_do_mid_side_stereo(const FLAC__SeekableStreamEncoder *encoder);

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
FLAC_API FLAC__bool FLAC__seekable_stream_encoder_get_loose_mid_side_stereo(const FLAC__SeekableStreamEncoder *encoder);

/** Get the number of input channels being processed.
 *  This is inherited from FLAC__StreamEncoder; see
 *  FLAC__stream_encoder_get_channels().
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval unsigned
 *    See FLAC__seekable_stream_encoder_set_channels().
 */
FLAC_API unsigned FLAC__seekable_stream_encoder_get_channels(const FLAC__SeekableStreamEncoder *encoder);

/** Get the input sample resolution setting.
 *  This is inherited from FLAC__StreamEncoder; see
 *  FLAC__stream_encoder_get_bits_per_sample().
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval unsigned
 *    See FLAC__seekable_stream_encoder_set_bits_per_sample().
 */
FLAC_API unsigned FLAC__seekable_stream_encoder_get_bits_per_sample(const FLAC__SeekableStreamEncoder *encoder);

/** Get the input sample rate setting.
 *  This is inherited from FLAC__StreamEncoder; see
 *  FLAC__stream_encoder_get_sample_rate().
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval unsigned
 *    See FLAC__seekable_stream_encoder_set_sample_rate().
 */
FLAC_API unsigned FLAC__seekable_stream_encoder_get_sample_rate(const FLAC__SeekableStreamEncoder *encoder);

/** Get the blocksize setting.
 *  This is inherited from FLAC__StreamEncoder; see
 *  FLAC__stream_encoder_get_blocksize().
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval unsigned
 *    See FLAC__seekable_stream_encoder_set_blocksize().
 */
FLAC_API unsigned FLAC__seekable_stream_encoder_get_blocksize(const FLAC__SeekableStreamEncoder *encoder);

/** Get the maximum LPC order setting.
 *  This is inherited from FLAC__StreamEncoder; see
 *  FLAC__stream_encoder_get_max_lpc_order().
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval unsigned
 *    See FLAC__seekable_stream_encoder_set_max_lpc_order().
 */
FLAC_API unsigned FLAC__seekable_stream_encoder_get_max_lpc_order(const FLAC__SeekableStreamEncoder *encoder);

/** Get the quantized linear predictor coefficient precision setting.
 *  This is inherited from FLAC__StreamEncoder; see
 *  FLAC__stream_encoder_get_qlp_coeff_precision().
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval unsigned
 *    See FLAC__seekable_stream_encoder_set_qlp_coeff_precision().
 */
FLAC_API unsigned FLAC__seekable_stream_encoder_get_qlp_coeff_precision(const FLAC__SeekableStreamEncoder *encoder);

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
FLAC_API FLAC__bool FLAC__seekable_stream_encoder_get_do_qlp_coeff_prec_search(const FLAC__SeekableStreamEncoder *encoder);

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
FLAC_API FLAC__bool FLAC__seekable_stream_encoder_get_do_escape_coding(const FLAC__SeekableStreamEncoder *encoder);

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
FLAC_API FLAC__bool FLAC__seekable_stream_encoder_get_do_exhaustive_model_search(const FLAC__SeekableStreamEncoder *encoder);

/** Get the minimum residual partition order setting.
 *  This is inherited from FLAC__StreamEncoder; see
 *  FLAC__stream_encoder_get_min_residual_partition_order().
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval unsigned
 *    See FLAC__seekable_stream_encoder_set_min_residual_partition_order().
 */
FLAC_API unsigned FLAC__seekable_stream_encoder_get_min_residual_partition_order(const FLAC__SeekableStreamEncoder *encoder);

/** Get maximum residual partition order setting.
 *  This is inherited from FLAC__StreamEncoder; see
 *  FLAC__stream_encoder_get_max_residual_partition_order().
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval unsigned
 *    See FLAC__seekable_stream_encoder_set_max_residual_partition_order().
 */
FLAC_API unsigned FLAC__seekable_stream_encoder_get_max_residual_partition_order(const FLAC__SeekableStreamEncoder *encoder);

/** Get the Rice parameter search distance setting.
 *  This is inherited from FLAC__StreamEncoder; see
 *  FLAC__stream_encoder_get_rice_parameter_search_dist().
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval unsigned
 *    See FLAC__seekable_stream_encoder_set_rice_parameter_search_dist().
 */
FLAC_API unsigned FLAC__seekable_stream_encoder_get_rice_parameter_search_dist(const FLAC__SeekableStreamEncoder *encoder);

/** Get the previously set estimate of the total samples to be encoded.
 *  This is inherited from FLAC__StreamEncoder; see
 *  FLAC__stream_encoder_get_total_samples_estimate().
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__uint64
 *    See FLAC__seekable_stream_encoder_set_total_samples_estimate().
 */
FLAC_API FLAC__uint64 FLAC__seekable_stream_encoder_get_total_samples_estimate(const FLAC__SeekableStreamEncoder *encoder);

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
FLAC_API FLAC__SeekableStreamEncoderState FLAC__seekable_stream_encoder_init(FLAC__SeekableStreamEncoder *encoder);

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
FLAC_API void FLAC__seekable_stream_encoder_finish(FLAC__SeekableStreamEncoder *encoder);

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
FLAC_API FLAC__bool FLAC__seekable_stream_encoder_process(FLAC__SeekableStreamEncoder *encoder, const FLAC__int32 * const buffer[], unsigned samples);

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
FLAC_API FLAC__bool FLAC__seekable_stream_encoder_process_interleaved(FLAC__SeekableStreamEncoder *encoder, const FLAC__int32 buffer[], unsigned samples);

/* \} */

#ifdef __cplusplus
}
#endif

#endif
