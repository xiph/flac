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

#ifndef OggFLAC__SEEKABLE_STREAM_ENCODER_H
#define OggFLAC__SEEKABLE_STREAM_ENCODER_H

#include "export.h"

#include "FLAC/stream_encoder.h"
#include "FLAC/seekable_stream_encoder.h"

#ifdef __cplusplus
extern "C" {
#endif


/** \file include/OggFLAC/seekable_stream_encoder.h
 *
 *  \brief
 *  This module contains the functions which implement the seekable
 *  stream encoder.
 *
 *  See the detailed documentation in the
 *  \link oggflac_seekable_stream_encoder seekable stream encoder \endlink module.
 */

/** \defgroup oggflac_seekable_stream_encoder OggFLAC/seekable_stream_encoder.h: seekable stream encoder interface
 *  \ingroup oggflac_encoder
 *
 *  \brief
 *  This module contains the functions which implement the seekable
 *  stream encoder.  The Ogg seekable stream encoder is derived
 *  from the FLAC seekable stream encoder.
 *
 * The interface here is nearly identical to FLAC's seekable stream
 * encoder, including the callbacks, with the addition of a new required
 * read callback (needed when writing back STREAMINFO after encoding) and
 * OggFLAC__seekable_stream_encoder_set_serial_number().  See the
 * \link flac_seekable_stream_encoder FLAC seekable stream encoder module \endlink
 * for full documentation.
 *
 * \{
 */


/** State values for an OggFLAC__SeekableStreamEncoder
 *
 *  The encoder's state can be obtained by calling OggFLAC__stream_encoder_get_state().
 */
typedef enum {

	OggFLAC__SEEKABLE_STREAM_ENCODER_OK = 0,
	/**< The encoder is in the normal OK state. */

	OggFLAC__SEEKABLE_STREAM_ENCODER_OGG_ERROR,
	/**< An error occurred in the underlying Ogg layer.  */

	OggFLAC__SEEKABLE_STREAM_ENCODER_FLAC_STREAM_ENCODER_ERROR,
	/**< An error occurred in the underlying FLAC stream encoder;
	 * check OggFLAC__seekable_stream_encoder_get_FLAC_stream_encoder_state().
	 */

	OggFLAC__SEEKABLE_STREAM_ENCODER_MEMORY_ALLOCATION_ERROR,
	/**< Memory allocation failed. */

	OggFLAC__SEEKABLE_STREAM_ENCODER_WRITE_ERROR,
	/**< The write callback returned an error. */

	OggFLAC__SEEKABLE_STREAM_ENCODER_READ_ERROR,
	/**< The read callback returned an error. */

	OggFLAC__SEEKABLE_STREAM_ENCODER_SEEK_ERROR,
	/**< The seek callback returned an error. */

	OggFLAC__SEEKABLE_STREAM_ENCODER_TELL_ERROR,
	/**< The tell callback returned an error. */

	OggFLAC__SEEKABLE_STREAM_ENCODER_ALREADY_INITIALIZED,
	/**< OggFLAC__seekable_stream_encoder_init() was called when the encoder was
	 * already initialized, usually because
	 * OggFLAC__seekable_stream_encoder_finish() was not called.
	 */

	OggFLAC__SEEKABLE_STREAM_ENCODER_INVALID_CALLBACK,
	/**< OggFLAC__seekable_stream_encoder_init() was called without all
	 * callbacks being set.
	 */

	OggFLAC__SEEKABLE_STREAM_ENCODER_INVALID_SEEKTABLE,
	/**< An invalid seek table was passed is the metadata to
	 * OggFLAC__seekable_stream_encoder_set_metadata().
	 */

	OggFLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED
	/**< The encoder is in the uninitialized state. */

} OggFLAC__SeekableStreamEncoderState;

/** Maps an OggFLAC__StreamEncoderState to a C string.
 *
 *  Using an OggFLAC__StreamEncoderState as the index to this array
 *  will give the string equivalent.  The contents should not be modified.
 */
extern OggFLAC_API const char * const OggFLAC__SeekableStreamEncoderStateString[];


/** Return values for the OggFLAC__SeekableStreamEncoder read callback.
 */
typedef enum {

	OggFLAC__SEEKABLE_STREAM_ENCODER_READ_STATUS_CONTINUE,
	/**< The read was OK and decoding can continue. */

	OggFLAC__SEEKABLE_STREAM_ENCODER_READ_STATUS_END_OF_STREAM,
	/**< The read was attempted at the end of the stream. */

	OggFLAC__SEEKABLE_STREAM_ENCODER_READ_STATUS_ABORT
	/**< An unrecoverable error occurred. */

} OggFLAC__SeekableStreamEncoderReadStatus;

/** Maps a OggFLAC__SeekableStreamEncoderReadStatus to a C string.
 *
 *  Using a OggFLAC__SeekableStreamEncoderReadStatus as the index to this array
 *  will give the string equivalent.  The contents should not be modified.
 */
extern OggFLAC_API const char * const OggFLAC__SeekableStreamEncoderReadStatusString[];


/***********************************************************************
 *
 * class OggFLAC__StreamEncoder
 *
 ***********************************************************************/

struct OggFLAC__SeekableStreamEncoderProtected;
struct OggFLAC__SeekableStreamEncoderPrivate;
/** The opaque structure definition for the seekable stream encoder type.
 *  See the \link oggflac_seekable_stream_encoder seekable stream encoder module \endlink
 *  for a detailed description.
 */
typedef struct {
	struct OggFLAC__SeekableStreamEncoderProtected *protected_; /* avoid the C++ keyword 'protected' */
	struct OggFLAC__SeekableStreamEncoderPrivate *private_; /* avoid the C++ keyword 'private' */
} OggFLAC__SeekableStreamEncoder;

/** Signature for the read callback.
 *  See OggFLAC__seekable_stream_encoder_set_read_callback() for more info.
 *
 * \param  encoder  The encoder instance calling the callback.
 * \param  buffer   A pointer to a location for the callee to store
 *                  data to be encoded.
 * \param  bytes    A pointer to the size of the buffer.  On entry
 *                  to the callback, it contains the maximum number
 *                  of bytes that may be stored in \a buffer.  The
 *                  callee must set it to the actual number of bytes
 *                  stored (0 in case of error or end-of-stream) before
 *                  returning.
 * \param  client_data  The callee's client data set through
 *                      OggFLAC__seekable_stream_encoder_set_client_data().
 * \retval OggFLAC__SeekableStreamEncoderReadStatus
 *    The callee's return status.
 */
typedef OggFLAC__SeekableStreamEncoderReadStatus (*OggFLAC__SeekableStreamEncoderReadCallback)(const OggFLAC__SeekableStreamEncoder *encoder, FLAC__byte buffer[], unsigned *bytes, void *client_data);

/** Signature for the seek callback.
 *  See OggFLAC__seekable_stream_encoder_set_seek_callback()
 *  and FLAC__SeekableStreamEncoderSeekCallback for more info.
 *
 * \param  encoder  The encoder instance calling the callback.
 * \param  absolute_byte_offset  The offset from the beginning of the stream
 *                               to seek to.
 * \param  client_data  The callee's client data set through
 *                      OggFLAC__seekable_stream_encoder_set_client_data().
 * \retval FLAC__SeekableStreamEncoderSeekStatus
 *    The callee's return status.
 */
typedef FLAC__SeekableStreamEncoderSeekStatus (*OggFLAC__SeekableStreamEncoderSeekCallback)(const OggFLAC__SeekableStreamEncoder *encoder, FLAC__uint64 absolute_byte_offset, void *client_data);

/** Signature for the tell callback.
 *  See OggFLAC__seekable_stream_encoder_set_tell_callback()
 *  and FLAC__SeekableStreamEncoderTellCallback for more info.
 *
 * \param  encoder  The encoder instance calling the callback.
 * \param  absolute_byte_offset  The address at which to store the current
 *                               position of the output.
 * \param  client_data  The callee's client data set through
 *                      OggFLAC__seekable_stream_encoder_set_client_data().
 * \retval FLAC__SeekableStreamEncoderTellStatus
 *    The callee's return status.
 */
typedef FLAC__SeekableStreamEncoderTellStatus (*OggFLAC__SeekableStreamEncoderTellCallback)(const OggFLAC__SeekableStreamEncoder *encoder, FLAC__uint64 *absolute_byte_offset, void *client_data);

/** Signature for the write callback.
 *  See OggFLAC__seekable_stream_encoder_set_write_callback()
 *  and FLAC__SeekableStreamEncoderWriteCallback for more info.
 *
 * \param  encoder  The encoder instance calling the callback.
 * \param  buffer   An array of encoded data of length \a bytes.
 * \param  bytes    The byte length of \a buffer.
 * \param  samples  The number of samples encoded by \a buffer.
 *                  \c 0 has a special meaning; see
 *                  OggFLAC__seekable_stream_encoder_set_write_callback().
 * \param  current_frame  The number of current frame being encoded.
 * \param  client_data  The callee's client data set through
 *                      OggFLAC__seekable_stream_encoder_set_client_data().
 * \retval FLAC__StreamEncoderWriteStatus
 *    The callee's return status.
 */
typedef FLAC__StreamEncoderWriteStatus (*OggFLAC__SeekableStreamEncoderWriteCallback)(const OggFLAC__SeekableStreamEncoder *encoder, const FLAC__byte buffer[], unsigned bytes, unsigned samples, unsigned current_frame, void *client_data);


/***********************************************************************
 *
 * Class constructor/destructor
 *
 ***********************************************************************/

/** Create a new seekable stream encoder instance.  The instance is created with
 *  default settings; see the individual OggFLAC__seekable_stream_encoder_set_*()
 *  functions for each setting's default.
 *
 * \retval OggFLAC__SeekableStreamEncoder*
 *    \c NULL if there was an error allocating memory, else the new instance.
 */
OggFLAC_API OggFLAC__SeekableStreamEncoder *OggFLAC__seekable_stream_encoder_new();

/** Free an encoder instance.  Deletes the object pointed to by \a encoder.
 *
 * \param encoder  A pointer to an existing encoder.
 * \assert
 *    \code encoder != NULL \endcode
 */
OggFLAC_API void OggFLAC__seekable_stream_encoder_delete(OggFLAC__SeekableStreamEncoder *encoder);


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
OggFLAC_API FLAC__bool OggFLAC__seekable_stream_encoder_set_serial_number(OggFLAC__SeekableStreamEncoder *encoder, long serial_number);

/** This is inherited from FLAC__StreamEncoder; see FLAC__stream_encoder_set_verify()
 *
 * \default \c false
 * \param  encoder  An encoder instance to set.
 * \param  value    Flag value (see above).
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
OggFLAC_API FLAC__bool OggFLAC__seekable_stream_encoder_set_verify(OggFLAC__SeekableStreamEncoder *encoder, FLAC__bool value);

/** This is inherited from FLAC__StreamEncoder; see FLAC__stream_encoder_set_streamable_subset()
 *
 * \default \c true
 * \param  encoder  An encoder instance to set.
 * \param  value    Flag value (see above).
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
OggFLAC_API FLAC__bool OggFLAC__seekable_stream_encoder_set_streamable_subset(OggFLAC__SeekableStreamEncoder *encoder, FLAC__bool value);

/** This is inherited from FLAC__StreamEncoder; see FLAC__stream_encoder_set_do_mid_side_stereo()
 *
 * \default \c false
 * \param  encoder  An encoder instance to set.
 * \param  value    Flag value (see above).
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
OggFLAC_API FLAC__bool OggFLAC__seekable_stream_encoder_set_do_mid_side_stereo(OggFLAC__SeekableStreamEncoder *encoder, FLAC__bool value);

/** This is inherited from FLAC__StreamEncoder; see FLAC__stream_encoder_set_loose_mid_side_stereo()
 *
 * \default \c false
 * \param  encoder  An encoder instance to set.
 * \param  value    Flag value (see above).
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
OggFLAC_API FLAC__bool OggFLAC__seekable_stream_encoder_set_loose_mid_side_stereo(OggFLAC__SeekableStreamEncoder *encoder, FLAC__bool value);

/** This is inherited from FLAC__StreamEncoder; see FLAC__stream_encoder_set_channels()
 *
 * \default \c 2
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
OggFLAC_API FLAC__bool OggFLAC__seekable_stream_encoder_set_channels(OggFLAC__SeekableStreamEncoder *encoder, unsigned value);

/** This is inherited from FLAC__StreamEncoder; see FLAC__stream_encoder_set_bits_per_sample()
 *
 * \default \c 16
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
OggFLAC_API FLAC__bool OggFLAC__seekable_stream_encoder_set_bits_per_sample(OggFLAC__SeekableStreamEncoder *encoder, unsigned value);

/** This is inherited from FLAC__StreamEncoder; see FLAC__stream_encoder_set_sample_rate()
 *
 * \default \c 44100
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
OggFLAC_API FLAC__bool OggFLAC__seekable_stream_encoder_set_sample_rate(OggFLAC__SeekableStreamEncoder *encoder, unsigned value);

/** This is inherited from FLAC__StreamEncoder; see FLAC__stream_encoder_set_blocksize()
 *
 * \default \c 1152
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
OggFLAC_API FLAC__bool OggFLAC__seekable_stream_encoder_set_blocksize(OggFLAC__SeekableStreamEncoder *encoder, unsigned value);

/** This is inherited from FLAC__StreamEncoder; see FLAC__stream_encoder_set_max_lpc_order()
 *
 * \default \c 0
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
OggFLAC_API FLAC__bool OggFLAC__seekable_stream_encoder_set_max_lpc_order(OggFLAC__SeekableStreamEncoder *encoder, unsigned value);

/** This is inherited from FLAC__StreamEncoder; see FLAC__stream_encoder_set_qlp_coeff_precision()
 *
 * \default \c 0
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
OggFLAC_API FLAC__bool OggFLAC__seekable_stream_encoder_set_qlp_coeff_precision(OggFLAC__SeekableStreamEncoder *encoder, unsigned value);

/** This is inherited from FLAC__StreamEncoder; see FLAC__stream_encoder_set_qlp_coeff_prec_search()
 *
 * \default \c false
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
OggFLAC_API FLAC__bool OggFLAC__seekable_stream_encoder_set_do_qlp_coeff_prec_search(OggFLAC__SeekableStreamEncoder *encoder, FLAC__bool value);

/** This is inherited from FLAC__StreamEncoder; see FLAC__stream_encoder_set_do_escape_coding()
 *
 * \default \c false
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
OggFLAC_API FLAC__bool OggFLAC__seekable_stream_encoder_set_do_escape_coding(OggFLAC__SeekableStreamEncoder *encoder, FLAC__bool value);

/** This is inherited from FLAC__StreamEncoder; see FLAC__stream_encoder_set_do_exhaustive_model_search()
 *
 * \default \c false
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
OggFLAC_API FLAC__bool OggFLAC__seekable_stream_encoder_set_do_exhaustive_model_search(OggFLAC__SeekableStreamEncoder *encoder, FLAC__bool value);

/** This is inherited from FLAC__StreamEncoder; see FLAC__stream_encoder_set_min_residual_partition_order()
 *
 * \default \c 0
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
OggFLAC_API FLAC__bool OggFLAC__seekable_stream_encoder_set_min_residual_partition_order(OggFLAC__SeekableStreamEncoder *encoder, unsigned value);

/** This is inherited from FLAC__StreamEncoder; see FLAC__stream_encoder_set_max_residual_partition_order()
 *
 * \default \c 0
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
OggFLAC_API FLAC__bool OggFLAC__seekable_stream_encoder_set_max_residual_partition_order(OggFLAC__SeekableStreamEncoder *encoder, unsigned value);

/** This is inherited from FLAC__StreamEncoder; see FLAC__stream_encoder_set_rice_parameter_search_dist()
 *
 * \default \c 0
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
OggFLAC_API FLAC__bool OggFLAC__seekable_stream_encoder_set_rice_parameter_search_dist(OggFLAC__SeekableStreamEncoder *encoder, unsigned value);

/** This is inherited from FLAC__StreamEncoder; see FLAC__stream_encoder_set_total_samples_estimate()
 *
 * \default \c 0
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
OggFLAC_API FLAC__bool OggFLAC__seekable_stream_encoder_set_total_samples_estimate(OggFLAC__SeekableStreamEncoder *encoder, FLAC__uint64 value);

/** This is inherited from FLAC__StreamEncoder; see FLAC__stream_encoder_set_metadata()
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
OggFLAC_API FLAC__bool OggFLAC__seekable_stream_encoder_set_metadata(OggFLAC__SeekableStreamEncoder *encoder, FLAC__StreamMetadata **metadata, unsigned num_blocks);

/** Set the read callback.
 *  The supplied function will be called when the encoder needs to read back
 *  encoded data.  This happens during the metadata callback, when the encoder
 *  has to read, modify, and rewrite the metadata (e.g. seekpoints) gathered
 *  while encoding.  The address of the buffer to be filled is supplied, along
 *  with the number of bytes the buffer can hold.  The callback may choose to
 *  supply less data and modify the byte count but must be careful not to
 *  overflow the buffer.  The callback then returns a status code chosen from
 *  OggFLAC__SeekableStreamEncoderReadStatus.
 *
 * \note
 * The callback is mandatory and must be set before initialization.
 *
 * \default \c NULL
 * \param  encoder  A encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 *    \code value != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
OggFLAC_API FLAC__bool OggFLAC__seekable_stream_encoder_set_read_callback(OggFLAC__SeekableStreamEncoder *encoder, OggFLAC__SeekableStreamEncoderReadCallback value);

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
OggFLAC_API FLAC__bool OggFLAC__seekable_stream_encoder_set_seek_callback(OggFLAC__SeekableStreamEncoder *encoder, OggFLAC__SeekableStreamEncoderSeekCallback value);

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
OggFLAC_API FLAC__bool OggFLAC__seekable_stream_encoder_set_tell_callback(OggFLAC__SeekableStreamEncoder *encoder, OggFLAC__SeekableStreamEncoderTellCallback value);

/** Set the write callback.
 *  This is inherited from FLAC__StreamEncoder; see
 *  FLAC__stream_encoder_set_write_callback().
 *
 * \note
 * Unlike the FLAC seekable stream encoder write callback, the Ogg
 * seekable stream encoder write callback will be called twice when
 * writing audio frames; once for the page header, and once for the page
 * body.  When writing the page header, the \a samples argument to the
 * write callback will be \c 0.
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
OggFLAC_API FLAC__bool OggFLAC__seekable_stream_encoder_set_write_callback(OggFLAC__SeekableStreamEncoder *encoder, OggFLAC__SeekableStreamEncoderWriteCallback value);

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
OggFLAC_API FLAC__bool OggFLAC__seekable_stream_encoder_set_client_data(OggFLAC__SeekableStreamEncoder *encoder, void *value);

/** Get the current encoder state.
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval OggFLAC__SeekableStreamEncoderState
 *    The current encoder state.
 */
OggFLAC_API OggFLAC__SeekableStreamEncoderState OggFLAC__seekable_stream_encoder_get_state(const OggFLAC__SeekableStreamEncoder *encoder);

/** Get the state of the underlying FLAC stream encoder.
 *  Useful when the seekable stream encoder state is
 *  \c OggFLAC__SEEKABLE_STREAM_ENCODER_FLAC_STREAM_ENCODER_ERROR.
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__StreamEncoderState
 *    The FLAC stream encoder state.
 */
OggFLAC_API FLAC__StreamEncoderState OggFLAC__seekable_stream_encoder_get_FLAC_stream_encoder_state(const OggFLAC__SeekableStreamEncoder *encoder);

/** Get the state of the underlying FLAC encoder's verify decoder.
 *  Useful when the seekable stream encoder state is
 *  \c OggFLAC__SEEKABLE_STREAM_ENCODER_FLAC_STREAM_ENCODER_ERROR
 *  and the FLAC stream encoder state is
 *  \c FLAC__STREAM_ENCODER_VERIFY_DECODER_ERROR.
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__StreamDecoderState
 *    The FLAC verify decoder state.
 */
OggFLAC_API FLAC__StreamDecoderState OggFLAC__seekable_stream_encoder_get_verify_decoder_state(const OggFLAC__SeekableStreamEncoder *encoder);

/** Get the current encoder state as a C string.
 *  This version automatically resolves
 *  \c OggFLAC__SEEKABLE_STREAM_ENCODER_FLAC_STREAM_ENCODER_ERROR
 *  by getting the FLAC stream encoder's resolved state.
 *
 * \param  encoder  A encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval const char *
 *    The encoder state as a C string.  Do not modify the contents.
 */
OggFLAC_API const char *OggFLAC__seekable_stream_encoder_get_resolved_state_string(const OggFLAC__SeekableStreamEncoder *encoder);

/** Get relevant values about the nature of a verify decoder error.
 *  Inherited from FLAC__stream_encoder_get_verify_decoder_error_stats().
 *  Useful when the seekable stream encoder state is
 *  \c OggFLAC__SEEKABLE_STREAM_ENCODER_FLAC_STREAM_ENCODER_ERROR
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
 *    \code absolute_sample != NULL \endcode
 *    \code frame_number != NULL \endcode
 *    \code channel != NULL \endcode
 *    \code sample != NULL \endcode
 *    \code expected != NULL \endcode
 */
OggFLAC_API void OggFLAC__seekable_stream_encoder_get_verify_decoder_error_stats(const OggFLAC__SeekableStreamEncoder *encoder, FLAC__uint64 *absolute_sample, unsigned *frame_number, unsigned *channel, unsigned *sample, FLAC__int32 *expected, FLAC__int32 *got);

/** This is inherited from FLAC__StreamEncoder; see FLAC__stream_encoder_get_verify()
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    See OggFLAC__seekable_stream_encoder_set_verify().
 */
OggFLAC_API FLAC__bool OggFLAC__seekable_stream_encoder_get_verify(const OggFLAC__SeekableStreamEncoder *encoder);

/** This is inherited from FLAC__StreamEncoder; see FLAC__stream_encoder_get_streamable_subset()
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    See OggFLAC__seekable_stream_encoder_set_streamable_subset().
 */
OggFLAC_API FLAC__bool OggFLAC__seekable_stream_encoder_get_streamable_subset(const OggFLAC__SeekableStreamEncoder *encoder);

/** This is inherited from FLAC__StreamEncoder; see FLAC__stream_encoder_get_do_mid_side_stereo()
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    See OggFLAC__seekable_stream_encoder_get_do_mid_side_stereo().
 */
OggFLAC_API FLAC__bool OggFLAC__seekable_stream_encoder_get_do_mid_side_stereo(const OggFLAC__SeekableStreamEncoder *encoder);

/** This is inherited from FLAC__StreamEncoder; see FLAC__stream_encoder_get_loose_mid_side_stereo()
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    See OggFLAC__seekable_stream_encoder_set_loose_mid_side_stereo().
 */
OggFLAC_API FLAC__bool OggFLAC__seekable_stream_encoder_get_loose_mid_side_stereo(const OggFLAC__SeekableStreamEncoder *encoder);

/** This is inherited from FLAC__StreamEncoder; see FLAC__stream_encoder_get_channels()
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval unsigned
 *    See OggFLAC__seekable_stream_encoder_set_channels().
 */
OggFLAC_API unsigned OggFLAC__seekable_stream_encoder_get_channels(const OggFLAC__SeekableStreamEncoder *encoder);

/** This is inherited from FLAC__StreamEncoder; see FLAC__stream_encoder_get_bits_per_sample()
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval unsigned
 *    See OggFLAC__seekable_stream_encoder_set_bits_per_sample().
 */
OggFLAC_API unsigned OggFLAC__seekable_stream_encoder_get_bits_per_sample(const OggFLAC__SeekableStreamEncoder *encoder);

/** This is inherited from FLAC__StreamEncoder; see FLAC__stream_encoder_get_sample_rate()
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval unsigned
 *    See OggFLAC__seekable_stream_encoder_set_sample_rate().
 */
OggFLAC_API unsigned OggFLAC__seekable_stream_encoder_get_sample_rate(const OggFLAC__SeekableStreamEncoder *encoder);

/** This is inherited from FLAC__StreamEncoder; see FLAC__stream_encoder_get_blocksize()
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval unsigned
 *    See OggFLAC__seekable_stream_encoder_set_blocksize().
 */
OggFLAC_API unsigned OggFLAC__seekable_stream_encoder_get_blocksize(const OggFLAC__SeekableStreamEncoder *encoder);

/** This is inherited from FLAC__StreamEncoder; see FLAC__stream_encoder_get_max_lpc_order()
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval unsigned
 *    See OggFLAC__seekable_stream_encoder_set_max_lpc_order().
 */
OggFLAC_API unsigned OggFLAC__seekable_stream_encoder_get_max_lpc_order(const OggFLAC__SeekableStreamEncoder *encoder);

/** This is inherited from FLAC__StreamEncoder; see FLAC__stream_encoder_get_qlp_coeff_precision()
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval unsigned
 *    See OggFLAC__seekable_stream_encoder_set_qlp_coeff_precision().
 */
OggFLAC_API unsigned OggFLAC__seekable_stream_encoder_get_qlp_coeff_precision(const OggFLAC__SeekableStreamEncoder *encoder);

/** This is inherited from FLAC__StreamEncoder; see FLAC__stream_encoder_get_do_qlp_coeff_prec_search()
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    See OggFLAC__seekable_stream_encoder_set_do_qlp_coeff_prec_search().
 */
OggFLAC_API FLAC__bool OggFLAC__seekable_stream_encoder_get_do_qlp_coeff_prec_search(const OggFLAC__SeekableStreamEncoder *encoder);

/** This is inherited from FLAC__StreamEncoder; see FLAC__stream_encoder_get_do_escape_coding()
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    See OggFLAC__seekable_stream_encoder_set_do_escape_coding().
 */
OggFLAC_API FLAC__bool OggFLAC__seekable_stream_encoder_get_do_escape_coding(const OggFLAC__SeekableStreamEncoder *encoder);

/** This is inherited from FLAC__StreamEncoder; see FLAC__stream_encoder_get_do_exhaustive_model_search()
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    See OggFLAC__seekable_stream_encoder_set_do_exhaustive_model_search().
 */
OggFLAC_API FLAC__bool OggFLAC__seekable_stream_encoder_get_do_exhaustive_model_search(const OggFLAC__SeekableStreamEncoder *encoder);

/** This is inherited from FLAC__StreamEncoder; see FLAC__stream_encoder_get_min_residual_partition_order()
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval unsigned
 *    See OggFLAC__seekable_stream_encoder_set_min_residual_partition_order().
 */
OggFLAC_API unsigned OggFLAC__seekable_stream_encoder_get_min_residual_partition_order(const OggFLAC__SeekableStreamEncoder *encoder);

/** This is inherited from FLAC__StreamEncoder; see FLAC__stream_encoder_get_man_residual_partition_order()
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval unsigned
 *    See OggFLAC__seekable_stream_encoder_set_max_residual_partition_order().
 */
OggFLAC_API unsigned OggFLAC__seekable_stream_encoder_get_max_residual_partition_order(const OggFLAC__SeekableStreamEncoder *encoder);

/** This is inherited from FLAC__StreamEncoder; see FLAC__stream_encoder_get_rice_parameter_search_dist()
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval unsigned
 *    See OggFLAC__seekable_stream_encoder_set_rice_parameter_search_dist().
 */
OggFLAC_API unsigned OggFLAC__seekable_stream_encoder_get_rice_parameter_search_dist(const OggFLAC__SeekableStreamEncoder *encoder);

/** This is inherited from FLAC__StreamEncoder; see FLAC__stream_encoder_get_total_samples_estimate()
 *
 * \param  encoder  An encoder instance to set.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__uint64
 *    See OggFLAC__seekable_stream_encoder_get_total_samples_estimate().
 */
OggFLAC_API FLAC__uint64 OggFLAC__seekable_stream_encoder_get_total_samples_estimate(const OggFLAC__SeekableStreamEncoder *encoder);

/** Initialize the encoder instance.
 *  Should be called after OggFLAC__seekable_stream_encoder_new() and
 *  OggFLAC__seekable_stream_encoder_set_*() but before OggFLAC__seekable_stream_encoder_process()
 *  or OggFLAC__seekable_stream_encoder_process_interleaved().  Will set and return
 *  the encoder state, which will be OggFLAC__SEEKABLE_STREAM_ENCODER_OK if
 *  initialization succeeded.
 *
 *  The call to OggFLAC__seekable_stream_encoder_init() currently will also immediately
 *  call the write callback several times, once with the \c fLaC signature,
 *  and once for each encoded metadata block.
 *
 * \param  encoder  An uninitialized encoder instance.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval OggFLAC__SeekableStreamEncoderState
 *    \c OggFLAC__SEEKABLE_STREAM_ENCODER_OK if initialization was successful; see
 *    OggFLAC__SeekableStreamEncoderState for the meanings of other return values.
 */
OggFLAC_API OggFLAC__SeekableStreamEncoderState OggFLAC__seekable_stream_encoder_init(OggFLAC__SeekableStreamEncoder *encoder);

/** Finish the encoding process.
 *  Flushes the encoding buffer, releases resources, resets the encoder
 *  settings to their defaults, and returns the encoder state to
 *  OggFLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED.  Note that this can generate
 *  one or more write callbacks before returning.
 *
 *  In the event of a prematurely-terminated encode, it is not strictly
 *  necessary to call this immediately before OggFLAC__seekable_stream_encoder_delete()
 *  but it is good practice to match every OggFLAC__seekable_stream_encoder_init()
 *  with an OggFLAC__seekable_stream_encoder_finish().
 *
 * \param  encoder  An uninitialized encoder instance.
 * \assert
 *    \code encoder != NULL \endcode
 */
OggFLAC_API void OggFLAC__seekable_stream_encoder_finish(OggFLAC__SeekableStreamEncoder *encoder);

/** Submit data for encoding.
 * This is inherited from FLAC__StreamEncoder; see FLAC__stream_encoder_process().
 *
 * \param  encoder  An initialized encoder instance in the OK state.
 * \param  buffer   An array of pointers to each channel's signal.
 * \param  samples  The number of samples in one channel.
 * \assert
 *    \code encoder != NULL \endcode
 *    \code OggFLAC__seekable_stream_encoder_get_state(encoder) == OggFLAC__SEEKABLE_STREAM_ENCODER_OK \endcode
 * \retval FLAC__bool
 *    \c true if successful, else \c false; in this case, check the
 *    encoder state with OggFLAC__seekable_stream_encoder_get_state() to see what
 *    went wrong.
 */
OggFLAC_API FLAC__bool OggFLAC__seekable_stream_encoder_process(OggFLAC__SeekableStreamEncoder *encoder, const FLAC__int32 * const buffer[], unsigned samples);

/** Submit data for encoding.
 * This is inherited from FLAC__StreamEncoder; see FLAC__stream_encoder_process_interleaved().
 *
 * \param  encoder  An initialized encoder instance in the OK state.
 * \param  buffer   An array of channel-interleaved data (see above).
 * \param  samples  The number of samples in one channel, the same as for
 *                  OggFLAC__seekable_stream_encoder_process().  For example, if
 *                  encoding two channels, \c 1000 \a samples corresponds
 *                  to a \a buffer of 2000 values.
 * \assert
 *    \code encoder != NULL \endcode
 *    \code OggFLAC__seekable_stream_encoder_get_state(encoder) == OggFLAC__SEEKABLE_STREAM_ENCODER_OK \endcode
 * \retval FLAC__bool
 *    \c true if successful, else \c false; in this case, check the
 *    encoder state with OggFLAC__seekable_stream_encoder_get_state() to see what
 *    went wrong.
 */
OggFLAC_API FLAC__bool OggFLAC__seekable_stream_encoder_process_interleaved(OggFLAC__SeekableStreamEncoder *encoder, const FLAC__int32 buffer[], unsigned samples);

/* \} */

#ifdef __cplusplus
}
#endif

#endif
