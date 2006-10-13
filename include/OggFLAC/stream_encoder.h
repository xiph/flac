/* libOggFLAC - Free Lossless Audio Codec + Ogg library
 * Copyright (C) 2002,2003,2004,2005,2006 Josh Coalson
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

#ifndef OggFLAC__STREAM_ENCODER_H
#define OggFLAC__STREAM_ENCODER_H

#include "export.h"

#include "FLAC/stream_encoder.h"

#ifdef __cplusplus
extern "C" {
#endif


/** \file include/OggFLAC/stream_encoder.h
 *
 *  \brief
 *  This module contains the functions which implement the stream
 *  encoder.
 *
 *  See the detailed documentation in the
 *  \link oggflac_stream_encoder stream encoder \endlink module.
 */

/** \defgroup oggflac_encoder OggFLAC/ *_encoder.h: encoder interfaces
 *  \ingroup oggflac
 *
 *  \brief
 *  This module describes the encoder layers provided by libOggFLAC.
 *
 * libOggFLAC currently provides the same layers of access as libFLAC;
 * the interfaces are nearly identical, with the addition of a method for
 * specifying the Ogg serial number.  See the \link flac_encoder FLAC
 * encoder module \endlink for full documentation.
 */

/** \defgroup oggflac_stream_encoder OggFLAC/stream_encoder.h: stream encoder interface
 *  \ingroup oggflac_encoder
 *
 *  \brief
 *  This module contains the functions which implement the stream
 *  encoder.  The Ogg stream encoder is derived
 *  from the FLAC stream encoder.
 *
 * The interface here is nearly identical to FLAC's stream encoder,
 * including the callbacks, with the addition of
 * OggFLAC__stream_encoder_set_serial_number().  See the
 * \link flac_stream_encoder FLAC stream encoder module \endlink
 * for full documentation.
 *
 * \{
 */


/** State values for an OggFLAC__StreamEncoder
 *
 *  The encoder's state can be obtained by calling OggFLAC__stream_encoder_get_state().
 */
typedef enum {

	OggFLAC__STREAM_ENCODER_OK = 0,
	/**< The encoder is in the normal OK state and samples can be processed. */

	OggFLAC__STREAM_ENCODER_UNINITIALIZED,
	/**< The encoder is in the uninitialized state; one of the
	 * OggFLAC__stream_encoder_init_*() functions must be called before samples
	 * can be processed.
	 */

	OggFLAC__STREAM_ENCODER_OGG_ERROR,
	/**< An error occurred in the underlying Ogg layer.  */

	OggFLAC__STREAM_ENCODER_FLAC_STREAM_ENCODER_ERROR,
	/**< An error occurred in the underlying FLAC stream encoder;
	 * check OggFLAC__stream_encoder_get_FLAC_stream_encoder_state().
	 */

	OggFLAC__STREAM_ENCODER_CLIENT_ERROR,
	/**< One of the callbacks returned a fatal error. */

	OggFLAC__STREAM_ENCODER_IO_ERROR,
	/**< An I/O error occurred while opening/reading/writing a file.
	 * Check \c errno.
	 */

	OggFLAC__STREAM_ENCODER_MEMORY_ALLOCATION_ERROR
	/**< Memory allocation failed. */

} OggFLAC__StreamEncoderState;

/** Maps an OggFLAC__StreamEncoderState to a C string.
 *
 *  Using an OggFLAC__StreamEncoderState as the index to this array
 *  will give the string equivalent.  The contents should not be modified.
 */
extern OggFLAC_API const char * const OggFLAC__StreamEncoderStateString[];


/** Return values for the OggFLAC__StreamEncoder read callback.
 */
typedef enum {

	OggFLAC__STREAM_ENCODER_READ_STATUS_CONTINUE,
	/**< The read was OK and decoding can continue. */

	OggFLAC__STREAM_ENCODER_READ_STATUS_END_OF_STREAM,
	/**< The read was attempted at the end of the stream. */

	OggFLAC__STREAM_ENCODER_READ_STATUS_ABORT,
	/**< An unrecoverable error occurred. */

	OggFLAC__STREAM_ENCODER_READ_STATUS_UNSUPPORTED
	/**< Client does not support reading back from the output. */

} OggFLAC__StreamEncoderReadStatus;

/** Maps a OggFLAC__StreamEncoderReadStatus to a C string.
 *
 *  Using a OggFLAC__StreamEncoderReadStatus as the index to this array
 *  will give the string equivalent.  The contents should not be modified.
 */
extern OggFLAC_API const char * const OggFLAC__StreamEncoderReadStatusString[];


/***********************************************************************
 *
 * class OggFLAC__StreamEncoder
 *
 ***********************************************************************/

struct OggFLAC__StreamEncoderProtected;
struct OggFLAC__StreamEncoderPrivate;
/** The opaque structure definition for the stream encoder type.
 *  See the \link oggflac_stream_encoder stream encoder module \endlink
 *  for a detailed description.
 */
typedef struct {
	FLAC__StreamEncoder super_;
	struct OggFLAC__StreamEncoderProtected *protected_; /* avoid the C++ keyword 'protected' */
	struct OggFLAC__StreamEncoderPrivate *private_; /* avoid the C++ keyword 'private' */
} OggFLAC__StreamEncoder;

/** Signature for the read callback.
 *
 *  A function pointer matching this signature must be passed to
 *  OggFLAC__stream_encoder_init_stream() if seeking is supported.
 *  The supplied function will be called when the encoder needs to read back
 *  encoded data.  This happens during the metadata callback, when the encoder
 *  has to read, modify, and rewrite the metadata (e.g. seekpoints) gathered
 *  while encoding.  The address of the buffer to be filled is supplied, along
 *  with the number of bytes the buffer can hold.  The callback may choose to
 *  supply less data and modify the byte count but must be careful not to
 *  overflow the buffer.  The callback then returns a status code chosen from
 *  OggFLAC__StreamEncoderReadStatus.
 *
 * \note In general, FLAC__StreamEncoder functions which change the
 * state should not be called on the \a encoder while in the callback.
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
 *                      OggFLAC__stream_encoder_set_client_data().
 * \retval OggFLAC__StreamEncoderReadStatus
 *    The callee's return status.
 */
typedef OggFLAC__StreamEncoderReadStatus (*OggFLAC__StreamEncoderReadCallback)(const OggFLAC__StreamEncoder *encoder, FLAC__byte buffer[], unsigned *bytes, void *client_data);


/***********************************************************************
 *
 * Class constructor/destructor
 *
 ***********************************************************************/

/** Create a new stream encoder instance.  The instance is created with
 *  default settings; see the individual OggFLAC__stream_encoder_set_*()
 *  functions for each setting's default.
 *
 * \retval OggFLAC__StreamEncoder*
 *    \c NULL if there was an error allocating memory, else the new instance.
 */
OggFLAC_API OggFLAC__StreamEncoder *OggFLAC__stream_encoder_new();

/** Free an encoder instance.  Deletes the object pointed to by \a encoder.
 *
 * \param encoder  A pointer to an existing encoder.
 * \assert
 *    \code encoder != NULL \endcode
 */
OggFLAC_API void OggFLAC__stream_encoder_delete(OggFLAC__StreamEncoder *encoder);


/***********************************************************************
 *
 * Public class method prototypes
 *
 ***********************************************************************/

/** Set the serial number for the FLAC stream.
 *
 * \note
 * It is recommended to set a serial number explicitly as the default of '0'
 * may collide with other streams.
 *
 * \default \c 0
 * \param  encoder        An encoder instance to set.
 * \param  serial_number  See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_serial_number(OggFLAC__StreamEncoder *encoder, long serial_number);

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
OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_verify(OggFLAC__StreamEncoder *encoder, FLAC__bool value);

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
OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_streamable_subset(OggFLAC__StreamEncoder *encoder, FLAC__bool value);

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
OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_do_mid_side_stereo(OggFLAC__StreamEncoder *encoder, FLAC__bool value);

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
OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_loose_mid_side_stereo(OggFLAC__StreamEncoder *encoder, FLAC__bool value);

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
OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_channels(OggFLAC__StreamEncoder *encoder, unsigned value);

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
OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_bits_per_sample(OggFLAC__StreamEncoder *encoder, unsigned value);

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
OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_sample_rate(OggFLAC__StreamEncoder *encoder, unsigned value);

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
OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_blocksize(OggFLAC__StreamEncoder *encoder, unsigned value);

/** This is inherited from FLAC__StreamEncoder; see FLAC__stream_encoder_set_apodization()
 *
 * \default \c 0
 * \param  encoder        An encoder instance to set.
 * \param  specification  See above.
 * \assert
 *    \code encoder != NULL \endcode
 *    \code specification != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_apodization(OggFLAC__StreamEncoder *encoder, const char *specification);

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
OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_max_lpc_order(OggFLAC__StreamEncoder *encoder, unsigned value);

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
OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_qlp_coeff_precision(OggFLAC__StreamEncoder *encoder, unsigned value);

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
OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_do_qlp_coeff_prec_search(OggFLAC__StreamEncoder *encoder, FLAC__bool value);

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
OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_do_escape_coding(OggFLAC__StreamEncoder *encoder, FLAC__bool value);

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
OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_do_exhaustive_model_search(OggFLAC__StreamEncoder *encoder, FLAC__bool value);

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
OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_min_residual_partition_order(OggFLAC__StreamEncoder *encoder, unsigned value);

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
OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_max_residual_partition_order(OggFLAC__StreamEncoder *encoder, unsigned value);

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
OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_rice_parameter_search_dist(OggFLAC__StreamEncoder *encoder, unsigned value);

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
OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_total_samples_estimate(OggFLAC__StreamEncoder *encoder, FLAC__uint64 value);

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
OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_metadata(OggFLAC__StreamEncoder *encoder, FLAC__StreamMetadata **metadata, unsigned num_blocks);

/** Get the current encoder state.
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval OggFLAC__StreamEncoderState
 *    The current encoder state.
 */
OggFLAC_API OggFLAC__StreamEncoderState OggFLAC__stream_encoder_get_state(const OggFLAC__StreamEncoder *encoder);

/** Get the state of the underlying FLAC stream encoder.
 *  Useful when the stream encoder state is
 *  \c OggFLAC__STREAM_ENCODER_FLAC_STREAM_ENCODER_ERROR.
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__StreamEncoderState
 *    The FLAC stream encoder state.
 */
OggFLAC_API FLAC__StreamEncoderState OggFLAC__stream_encoder_get_FLAC_stream_encoder_state(const OggFLAC__StreamEncoder *encoder);

/** Get the state of the underlying FLAC stream encoder's verify decoder.
 *  Useful when the stream encoder state is
 *  \c OggFLAC__STREAM_ENCODER_FLAC_STREAM_ENCODER_ERROR and the
 *  FLAC encoder state is \c FLAC__STREAM_ENCODER_VERIFY_DECODER_ERROR.
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__StreamDecoderState
 *    The FLAC verify decoder state.
 */
OggFLAC_API FLAC__StreamDecoderState OggFLAC__stream_encoder_get_verify_decoder_state(const OggFLAC__StreamEncoder *encoder);

/** Get the current encoder state as a C string.
 *  This version automatically resolves
 *  \c OggFLAC__STREAM_ENCODER_FLAC_STREAM_ENCODER_ERROR by getting the
 *  FLAC stream encoder's state.
 *
 * \param  encoder  A encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval const char *
 *    The encoder state as a C string.  Do not modify the contents.
 */
OggFLAC_API const char *OggFLAC__stream_encoder_get_resolved_state_string(const OggFLAC__StreamEncoder *encoder);

/** Get relevant values about the nature of a verify decoder error.
 *  Inherited from FLAC__stream_encoder_get_verify_decoder_error_stats().
 *  Useful when the stream encoder state is
 *  \c OggFLAC__STREAM_ENCODER_FLAC_STREAM_ENCODER_ERROR and the
 *  FLAC stream encoder state is
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
OggFLAC_API void OggFLAC__stream_encoder_get_verify_decoder_error_stats(const OggFLAC__StreamEncoder *encoder, FLAC__uint64 *absolute_sample, unsigned *frame_number, unsigned *channel, unsigned *sample, FLAC__int32 *expected, FLAC__int32 *got);

/** This is inherited from FLAC__StreamEncoder; see FLAC__stream_encoder_get_verify()
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    See OggFLAC__stream_encoder_set_verify().
 */
OggFLAC_API FLAC__bool OggFLAC__stream_encoder_get_verify(const OggFLAC__StreamEncoder *encoder);

/** This is inherited from FLAC__StreamEncoder; see FLAC__stream_encoder_get_streamable_subset()
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    See OggFLAC__stream_encoder_set_streamable_subset().
 */
OggFLAC_API FLAC__bool OggFLAC__stream_encoder_get_streamable_subset(const OggFLAC__StreamEncoder *encoder);

/** This is inherited from FLAC__StreamEncoder; see FLAC__stream_encoder_get_do_mid_side_stereo()
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    See OggFLAC__stream_encoder_get_do_mid_side_stereo().
 */
OggFLAC_API FLAC__bool OggFLAC__stream_encoder_get_do_mid_side_stereo(const OggFLAC__StreamEncoder *encoder);

/** This is inherited from FLAC__StreamEncoder; see FLAC__stream_encoder_get_loose_mid_side_stereo()
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    See OggFLAC__stream_encoder_set_loose_mid_side_stereo().
 */
OggFLAC_API FLAC__bool OggFLAC__stream_encoder_get_loose_mid_side_stereo(const OggFLAC__StreamEncoder *encoder);

/** This is inherited from FLAC__StreamEncoder; see FLAC__stream_encoder_get_channels()
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval unsigned
 *    See OggFLAC__stream_encoder_set_channels().
 */
OggFLAC_API unsigned OggFLAC__stream_encoder_get_channels(const OggFLAC__StreamEncoder *encoder);

/** This is inherited from FLAC__StreamEncoder; see FLAC__stream_encoder_get_bits_per_sample()
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval unsigned
 *    See OggFLAC__stream_encoder_set_bits_per_sample().
 */
OggFLAC_API unsigned OggFLAC__stream_encoder_get_bits_per_sample(const OggFLAC__StreamEncoder *encoder);

/** This is inherited from FLAC__StreamEncoder; see FLAC__stream_encoder_get_sample_rate()
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval unsigned
 *    See OggFLAC__stream_encoder_set_sample_rate().
 */
OggFLAC_API unsigned OggFLAC__stream_encoder_get_sample_rate(const OggFLAC__StreamEncoder *encoder);

/** This is inherited from FLAC__StreamEncoder; see FLAC__stream_encoder_get_blocksize()
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval unsigned
 *    See OggFLAC__stream_encoder_set_blocksize().
 */
OggFLAC_API unsigned OggFLAC__stream_encoder_get_blocksize(const OggFLAC__StreamEncoder *encoder);

/** This is inherited from FLAC__StreamEncoder; see FLAC__stream_encoder_get_max_lpc_order()
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval unsigned
 *    See OggFLAC__stream_encoder_set_max_lpc_order().
 */
OggFLAC_API unsigned OggFLAC__stream_encoder_get_max_lpc_order(const OggFLAC__StreamEncoder *encoder);

/** This is inherited from FLAC__StreamEncoder; see FLAC__stream_encoder_get_qlp_coeff_precision()
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval unsigned
 *    See OggFLAC__stream_encoder_set_qlp_coeff_precision().
 */
OggFLAC_API unsigned OggFLAC__stream_encoder_get_qlp_coeff_precision(const OggFLAC__StreamEncoder *encoder);

/** This is inherited from FLAC__StreamEncoder; see FLAC__stream_encoder_get_do_qlp_coeff_prec_search()
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    See OggFLAC__stream_encoder_set_do_qlp_coeff_prec_search().
 */
OggFLAC_API FLAC__bool OggFLAC__stream_encoder_get_do_qlp_coeff_prec_search(const OggFLAC__StreamEncoder *encoder);

/** This is inherited from FLAC__StreamEncoder; see FLAC__stream_encoder_get_do_escape_coding()
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    See OggFLAC__stream_encoder_set_do_escape_coding().
 */
OggFLAC_API FLAC__bool OggFLAC__stream_encoder_get_do_escape_coding(const OggFLAC__StreamEncoder *encoder);

/** This is inherited from FLAC__StreamEncoder; see FLAC__stream_encoder_get_do_exhaustive_model_search()
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    See OggFLAC__stream_encoder_set_do_exhaustive_model_search().
 */
OggFLAC_API FLAC__bool OggFLAC__stream_encoder_get_do_exhaustive_model_search(const OggFLAC__StreamEncoder *encoder);

/** This is inherited from FLAC__StreamEncoder; see FLAC__stream_encoder_get_min_residual_partition_order()
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval unsigned
 *    See OggFLAC__stream_encoder_set_min_residual_partition_order().
 */
OggFLAC_API unsigned OggFLAC__stream_encoder_get_min_residual_partition_order(const OggFLAC__StreamEncoder *encoder);

/** This is inherited from FLAC__StreamEncoder; see FLAC__stream_encoder_get_man_residual_partition_order()
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval unsigned
 *    See OggFLAC__stream_encoder_set_max_residual_partition_order().
 */
OggFLAC_API unsigned OggFLAC__stream_encoder_get_max_residual_partition_order(const OggFLAC__StreamEncoder *encoder);

/** This is inherited from FLAC__StreamEncoder; see FLAC__stream_encoder_get_rice_parameter_search_dist()
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval unsigned
 *    See OggFLAC__stream_encoder_set_rice_parameter_search_dist().
 */
OggFLAC_API unsigned OggFLAC__stream_encoder_get_rice_parameter_search_dist(const OggFLAC__StreamEncoder *encoder);

/** This is inherited from FLAC__StreamEncoder; see FLAC__stream_encoder_get_total_samples_estimate()
 *
 * \param  encoder  An encoder instance to set.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__uint64
 *    See OggFLAC__stream_encoder_get_total_samples_estimate().
 */
OggFLAC_API FLAC__uint64 OggFLAC__stream_encoder_get_total_samples_estimate(const OggFLAC__StreamEncoder *encoder);

/** Initialize the encoder instance.
 *  Should be called after OggFLAC__stream_encoder_new() and
 *  OggFLAC__stream_encoder_set_*() but before OggFLAC__stream_encoder_process()
 *  or OggFLAC__stream_encoder_process_interleaved().  Will set and return
 *  the encoder state, which will be OggFLAC__STREAM_ENCODER_OK if
 *  initialization succeeded.
 *
 *  The call to OggFLAC__stream_encoder_init() currently will also immediately
 *  call the write callback several times, once with the \c fLaC signature,
 *  and once for each encoded metadata block.
 *
 * \param  encoder  An uninitialized encoder instance.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval OggFLAC__StreamEncoderState
 *    \c OggFLAC__STREAM_ENCODER_OK if initialization was successful; see
 *    OggFLAC__StreamEncoderState for the meanings of other return values.
 */
OggFLAC_API OggFLAC__StreamEncoderState OggFLAC__stream_encoder_init(OggFLAC__StreamEncoder *encoder);

/** Initialize the encoder instance.
 *
 *  This flavor of initialization sets up the encoder to encode to a stream.
 *  I/O is performed via callbacks to the client.  For encoding to a plain file
 *  via filename or open \c FILE*, OggFLAC__stream_encoder_init_file() and
 *  OggFLAC__stream_encoder_init_FILE() provide a simpler interface.
 *
 *  This function should be called after OggFLAC__stream_encoder_new() and
 *  OggFLAC__stream_encoder_set_*() but before OggFLAC__stream_encoder_process()
 *  or OggFLAC__stream_encoder_process_interleaved().
 *  initialization succeeded.
 *
 *  The call to OggFLAC__stream_encoder_init_stream() currently will also immediately
 *  call the write callback several times, once with the \c fLaC signature,
 *  and once for each encoded metadata block.
 *
 * \note
 * Unlike the FLAC stream encoder write callback, the Ogg stream
 * encoder write callback will be called twice when writing each audio
 * frame; once for the page header, and once for the page body.
 * When writing the page header, the \a samples argument to the
 * write callback will be \c 0.
 *
 * \param  encoder            An uninitialized encoder instance.
 * \param  read_callback      See OggFLAC__StreamEncoderReadCallback.  This
 *                            pointer must not be \c NULL if \a seek_callback
 *                            is non-NULL since they are both needed to be
 *                            able to write data back to the Ogg FLAC stream
 *                            in the post-encode phase.
 * \param  write_callback     See FLAC__StreamEncoderWriteCallback.  This
 *                            pointer must not be \c NULL.
 * \param  seek_callback      See FLAC__StreamEncoderSeekCallback.  This
 *                            pointer may be \c NULL if seeking is not
 *                            supported.  The encoder uses seeking to go back
 *                            and write some some stream statistics to the
 *                            STREAMINFO block; this is recommended but not
 *                            necessary to create a valid FLAC stream.  If
 *                            \a seek_callback is not \c NULL then a
 *                            \a tell_callback must also be supplied.
 *                            Alternatively, a dummy seek callback that just
 *                            returns \c FLAC__STREAM_ENCODER_SEEK_STATUS_UNSUPPORTED
 *                            may also be supplied, all though this is slightly
 *                            less efficient for the decoder.
 * \param  tell_callback      See FLAC__StreamEncoderTellCallback.  This
 *                            pointer may be \c NULL if seeking is not
 *                            supported.  If \a seek_callback is \c NULL then
 *                            this argument will be ignored.  If
 *                            \a seek_callback is not \c NULL then a
 *                            \a tell_callback must also be supplied.
 *                            Alternatively, a dummy tell callback that just
 *                            returns \c FLAC__STREAM_ENCODER_TELL_STATUS_UNSUPPORTED
 *                            may also be supplied, all though this is slightly
 *                            less efficient for the decoder.
 * \param  metadata_callback  See FLAC__StreamEncoderMetadataCallback.  This
 *                            pointer may be \c NULL if the callback is not
 *                            desired.  If the client provides a seek callback,
 *                            this function is not necessary as the encoder
 *                            will automatically seek back and update the
 *                            STREAMINFO block.  It may also be \c NULL if the
 *                            client does not support seeking, since it will
 *                            have no way of going back to update the
 *                            STREAMINFO.  However the client can still supply
 *                            a callback if it would like to know the details
 *                            from the STREAMINFO.
 * \param  client_data        This value will be supplied to callbacks in their
 *                            \a client_data argument.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__StreamEncoderInitStatus
 *    \c FLAC__STREAM_ENCODER_INIT_STATUS_OK if initialization was successful;
 *    see FLAC__StreamEncoderInitStatus for the meanings of other return values.
 */
OggFLAC_API FLAC__StreamEncoderInitStatus OggFLAC__stream_encoder_init_stream(OggFLAC__StreamEncoder *encoder, OggFLAC__StreamEncoderReadCallback read_callback, FLAC__StreamEncoderWriteCallback write_callback, FLAC__StreamEncoderSeekCallback seek_callback, FLAC__StreamEncoderTellCallback tell_callback, FLAC__StreamEncoderMetadataCallback metadata_callback, void *client_data);

/** Initialize the encoder instance.
 *
 *  This flavor of initialization sets up the encoder to encode to a plain
 *  file.  For non-stdio streams, you must use
 *  OggFLAC__stream_encoder_init_stream() and provide callbacks for the I/O.
 *
 *  This function should be called after OggFLAC__stream_encoder_new() and
 *  OggFLAC__stream_encoder_set_*() but before OggFLAC__stream_encoder_process()
 *  or OggFLAC__stream_encoder_process_interleaved().
 *  initialization succeeded.
 *
 *  The call to OggFLAC__stream_encoder_init_stream() currently will also immediately
 *  call the write callback several times, once with the \c fLaC signature,
 *  and once for each encoded metadata block.
 *
 * \note
 * Unlike the FLAC stream encoder write callback, the Ogg stream
 * encoder write callback will be called twice when writing each audio
 * frame; once for the page header, and once for the page body.
 * When writing the page header, the \a samples argument to the
 * write callback will be \c 0.
 *
 * \param  encoder            An uninitialized encoder instance.
 * \param  file               An open file.  The file should have been opened
 *                            with mode \c "w+b" and rewound.  The file
 *                            becomes owned by the encoder and should not be
 *                            manipulated by the client while encoding.
 *                            Unless \a file is \c stdout, it will be closed
 *                            when OggFLAC__stream_encoder_finish() is called.
 *                            Note however that a proper SEEKTABLE cannot be
 *                            created when encoding to \c stdout since it is
 *                            not seekable.
 * \param  progress_callback  See FLAC__StreamEncoderProgressCallback.  This
 *                            pointer may be \c NULL if the callback is not
 *                            desired.
 * \param  client_data        This value will be supplied to callbacks in their
 *                            \a client_data argument.
 * \assert
 *    \code encoder != NULL \endcode
 *    \code file != NULL \endcode
 * \retval FLAC__StreamEncoderInitStatus
 *    \c FLAC__STREAM_ENCODER_INIT_STATUS_OK if initialization was successful;
 *    see FLAC__StreamEncoderInitStatus for the meanings of other return values.
 */
OggFLAC_API FLAC__StreamEncoderInitStatus OggFLAC__stream_encoder_init_FILE(OggFLAC__StreamEncoder *encoder, FILE *file, FLAC__StreamEncoderProgressCallback progress_callback, void *client_data);

/** Initialize the encoder instance.
 *
 *  This flavor of initialization sets up the encoder to encode to a plain
 *  file.  If POSIX fopen() semantics are not sufficient (for example,
 *  with Unicode filenames on Windows), you must use
 *  OggFLAC__stream_encodeR_init_FILE(), or OggFLAC__stream_encoder_init_stream()
 *  and provide callbacks for the I/O.
 *
 *  This function should be called after OggFLAC__stream_encoder_new() and
 *  OggFLAC__stream_encoder_set_*() but before OggFLAC__stream_encoder_process()
 *  or OggFLAC__stream_encoder_process_interleaved().
 *  initialization succeeded.
 *
 *  The call to OggFLAC__stream_encoder_init_stream() currently will also immediately
 *  call the write callback several times, once with the \c fLaC signature,
 *  and once for each encoded metadata block.
 *
 * \note
 * Unlike the FLAC stream encoder write callback, the Ogg stream
 * encoder write callback will be called twice when writing each audio
 * frame; once for the page header, and once for the page body.
 * When writing the page header, the \a samples argument to the
 * write callback will be \c 0.
 *
 * \param  encoder            An uninitialized encoder instance.
 * \param  filename           The name of the file to encode to.  The file will
 *                            be opened with fopen().  Use \c NULL to encode to
 *                            \c stdout.  Note however that a proper SEEKTABLE
 *                            cannot be created when encoding to \c stdout since
 *                            it is not seekable.
 * \param  progress_callback  See FLAC__StreamEncoderProgressCallback.  This
 *                            pointer may be \c NULL if the callback is not
 *                            desired.
 * \param  client_data        This value will be supplied to callbacks in their
 *                            \a client_data argument.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__StreamEncoderInitStatus
 *    \c FLAC__STREAM_ENCODER_INIT_STATUS_OK if initialization was successful;
 *    see FLAC__StreamEncoderInitStatus for the meanings of other return values.
 */
OggFLAC_API FLAC__StreamEncoderInitStatus OggFLAC__stream_encoder_init_file(OggFLAC__StreamEncoder *encoder, const char *filename, FLAC__StreamEncoderProgressCallback progress_callback, void *client_data);

/** Finish the encoding process.
 *  Flushes the encoding buffer, releases resources, resets the encoder
 *  settings to their defaults, and returns the encoder state to
 *  OggFLAC__STREAM_ENCODER_UNINITIALIZED.  Note that this can generate
 *  one or more write callbacks before returning.
 *
 *  In the event of a prematurely-terminated encode, it is not strictly
 *  necessary to call this immediately before OggFLAC__stream_encoder_delete()
 *  but it is good practice to match every OggFLAC__stream_encoder_init()
 *  with an OggFLAC__stream_encoder_finish().
 *
 * \param  encoder  An uninitialized encoder instance.
 * \assert
 *    \code encoder != NULL \endcode
 */
OggFLAC_API void OggFLAC__stream_encoder_finish(OggFLAC__StreamEncoder *encoder);

/** Submit data for encoding.
 * This is inherited from FLAC__StreamEncoder; see FLAC__stream_encoder_process().
 *
 * \param  encoder  An initialized encoder instance in the OK state.
 * \param  buffer   An array of pointers to each channel's signal.
 * \param  samples  The number of samples in one channel.
 * \assert
 *    \code encoder != NULL \endcode
 *    \code OggFLAC__stream_encoder_get_state(encoder) == OggFLAC__STREAM_ENCODER_OK \endcode
 * \retval FLAC__bool
 *    \c true if successful, else \c false; in this case, check the
 *    encoder state with OggFLAC__stream_encoder_get_state() to see what
 *    went wrong.
 */
OggFLAC_API FLAC__bool OggFLAC__stream_encoder_process(OggFLAC__StreamEncoder *encoder, const FLAC__int32 * const buffer[], unsigned samples);

/** Submit data for encoding.
 * This is inherited from FLAC__StreamEncoder; see FLAC__stream_encoder_process_interleaved().
 *
 * \param  encoder  An initialized encoder instance in the OK state.
 * \param  buffer   An array of channel-interleaved data (see above).
 * \param  samples  The number of samples in one channel, the same as for
 *                  OggFLAC__stream_encoder_process().  For example, if
 *                  encoding two channels, \c 1000 \a samples corresponds
 *                  to a \a buffer of 2000 values.
 * \assert
 *    \code encoder != NULL \endcode
 *    \code OggFLAC__stream_encoder_get_state(encoder) == OggFLAC__STREAM_ENCODER_OK \endcode
 * \retval FLAC__bool
 *    \c true if successful, else \c false; in this case, check the
 *    encoder state with OggFLAC__stream_encoder_get_state() to see what
 *    went wrong.
 */
OggFLAC_API FLAC__bool OggFLAC__stream_encoder_process_interleaved(OggFLAC__StreamEncoder *encoder, const FLAC__int32 buffer[], unsigned samples);

/* \} */

#ifdef __cplusplus
}
#endif

#endif
