/* libFLAC - Free Lossless Audio Codec library
 * Copyright (C) 2000,2001,2002,2003,2004,2005  Josh Coalson
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

#ifndef FLAC__STREAM_ENCODER_H
#define FLAC__STREAM_ENCODER_H

#include "export.h"
#include "format.h"
#include "stream_decoder.h"

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

/** \defgroup flac_encoder FLAC/ *_encoder.h: encoder interfaces
 *  \ingroup flac
 *
 *  \brief
 *  This module describes the two encoder layers provided by libFLAC.
 *
 * For encoding FLAC streams, libFLAC provides three layers of access.  The
 * lowest layer is non-seekable stream-level encoding, the next is seekable
 * stream-level encoding, and the highest layer is file-level encoding.  The
 * interfaces are described in the \link flac_stream_encoder stream encoder
 * \endlink, \link flac_seekable_stream_encoder seekable stream encoder
 * \endlink, and \link flac_file_encoder file encoder \endlink modules
 * respectively.  Typically you will choose the highest layer that your input
 * source will support.
 * The stream encoder relies on callbacks for writing the data and
 * metadata. The file encoder provides these callbacks internally and you
 * need only supply the filename.
 *
 * The stream encoder relies on callbacks for writing the data and has no
 * provisions for seeking the output.  The seekable stream encoder wraps
 * the stream encoder and also automaticallay handles the writing back of
 * metadata discovered while encoding.  However, you must provide extra
 * callbacks for seek-related operations on your output, like seek and
 * tell.  The file encoder wraps the seekable stream encoder and supplies
 * all of the callbacks internally, simplifying the processing of standard
 * files.  The only callback exposed is for progress reporting, and that
 * is optional.
 */

/** \defgroup flac_stream_encoder FLAC/stream_encoder.h: stream encoder interface
 *  \ingroup flac_encoder
 *
 *  \brief
 *  This module contains the functions which implement the stream
 *  encoder.
 *
 * The basic usage of this encoder is as follows:
 * - The program creates an instance of an encoder using
 *   FLAC__stream_encoder_new().
 * - The program overrides the default settings and sets callbacks using
 *   FLAC__stream_encoder_set_*() functions.
 * - The program initializes the instance to validate the settings and
 *   prepare for encoding using FLAC__stream_encoder_init().
 * - The program calls FLAC__stream_encoder_process() or
 *   FLAC__stream_encoder_process_interleaved() to encode data, which
 *   subsequently calls the callbacks when there is encoder data ready
 *   to be written.
 * - The program finishes the encoding with FLAC__stream_encoder_finish(),
 *   which causes the encoder to encode any data still in its input pipe,
 *   call the metadata callback with the final encoding statistics, and
 *   finally reset the encoder to the uninitialized state.
 * - The instance may be used again or deleted with
 *   FLAC__stream_encoder_delete().
 *
 * In more detail, the stream encoder functions similarly to the
 * \link flac_stream_decoder stream decoder \endlink, but has fewer
 * callbacks and more options.  Typically the user will create a new
 * instance by calling FLAC__stream_encoder_new(), then set the necessary
 * parameters and callbacks with FLAC__stream_encoder_set_*(), and
 * initialize it by calling FLAC__stream_encoder_init().
 *
 * Unlike the decoders, the stream encoder has many options that can
 * affect the speed and compression ratio.  When setting these parameters
 * you should have some basic knowledge of the format (see the
 * <A HREF="../documentation.html#format">user-level documentation</A>
 * or the <A HREF="../format.html">formal description</A>).  The
 * FLAC__stream_encoder_set_*() functions themselves do not validate the
 * values as many are interdependent.  The FLAC__stream_encoder_init()
 * function will do this, so make sure to pay attention to the state
 * returned by FLAC__stream_encoder_init() to make sure that it is
 * FLAC__STREAM_ENCODER_OK.  Any parameters that are not set before
 * FLAC__stream_encoder_init() will take on the defaults from the
 * constructor.
 *
 * The user must provide function pointers for the following callbacks:
 *
 * - Write callback - This function is called by the encoder anytime there
 *   is raw encoded data to write.  It may include metadata mixed with
 *   encoded audio frames and the data is not guaranteed to be aligned on
 *   frame or metadata block boundaries.
 * - Metadata callback - This function is called once at the end of
 *   encoding with the populated STREAMINFO structure.  This is so file
 *   encoders can seek back to the beginning of the file and write the
 *   STREAMINFO block with the correct statistics after encoding (like
 *   minimum/maximum frame size).
 *
 * The call to FLAC__stream_encoder_init() currently will also immediately
 * call the write callback several times, once with the \c fLaC signature,
 * and once for each encoded metadata block.
 *
 * After initializing the instance, the user may feed audio data to the
 * encoder in one of two ways:
 *
 * - Channel separate, through FLAC__stream_encoder_process() - The user
 *   will pass an array of pointers to buffers, one for each channel, to
 *   the encoder, each of the same length.  The samples need not be
 *   block-aligned.
 * - Channel interleaved, through
 *   FLAC__stream_encoder_process_interleaved() - The user will pass a single
 *   pointer to data that is channel-interleaved (i.e. channel0_sample0,
 *   channel1_sample0, ... , channelN_sample0, channel0_sample1, ...).
 *   Again, the samples need not be block-aligned but they must be
 *   sample-aligned, i.e. the first value should be channel0_sample0 and
 *   the last value channelN_sampleM.
 *
 * When the user is finished encoding data, it calls
 * FLAC__stream_encoder_finish(), which causes the encoder to encode any
 * data still in its input pipe, and call the metadata callback with the
 * final encoding statistics.  Then the instance may be deleted with
 * FLAC__stream_encoder_delete() or initialized again to encode another
 * stream.
 *
 * For programs that write their own metadata, but that do not know the
 * actual metadata until after encoding, it is advantageous to instruct
 * the encoder to write a PADDING block of the correct size, so that
 * instead of rewriting the whole stream after encoding, the program can
 * just overwrite the PADDING block.  If only the maximum size of the
 * metadata is known, the program can write a slightly larger padding
 * block, then split it after encoding.
 *
 * Make sure you understand how lengths are calculated.  All FLAC metadata
 * blocks have a 4 byte header which contains the type and length.  This
 * length does not include the 4 bytes of the header.  See the format page
 * for the specification of metadata blocks and their lengths.
 *
 * \note
 * The "set" functions may only be called when the encoder is in the
 * state FLAC__STREAM_ENCODER_UNINITIALIZED, i.e. after
 * FLAC__stream_encoder_new() or FLAC__stream_encoder_finish(), but
 * before FLAC__stream_encoder_init().  If this is the case they will
 * return \c true, otherwise \c false.
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

	FLAC__STREAM_ENCODER_VERIFY_DECODER_ERROR,
	/**< An error occurred in the underlying verify stream decoder;
	 * check FLAC__stream_encoder_get_verify_decoder_state().
	 */

	FLAC__STREAM_ENCODER_VERIFY_MISMATCH_IN_AUDIO_DATA,
	/**< The verify decoder detected a mismatch between the original
	 * audio signal and the decoded audio signal.
	 */

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

	FLAC__STREAM_ENCODER_INVALID_MAX_LPC_ORDER,
	/**< The encoder has an invalid setting for the maximum LPC order. */

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
	/**< The metadata input to the encoder is invalid, in one of the following ways:
	 * - FLAC__stream_encoder_set_metadata() was called with a null pointer but a block count > 0
	 * - One of the metadata blocks contains an undefined type
	 * - It contains an illegal CUESHEET as checked by FLAC__format_cuesheet_is_legal()
	 * - It contains an illegal SEEKTABLE as checked by FLAC__format_seektable_is_legal()
	 * - It contains more than one SEEKTABLE block or more than one VORBIS_COMMENT block
	 */

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
extern FLAC_API const char * const FLAC__StreamEncoderStateString[];

/** Return values for the FLAC__StreamEncoder write callback.
 */
typedef enum {

	FLAC__STREAM_ENCODER_WRITE_STATUS_OK = 0,
	/**< The write was OK and encoding can continue. */

	FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR
	/**< An unrecoverable error occurred.  The encoder will return from the process call. */

} FLAC__StreamEncoderWriteStatus;

/** Maps a FLAC__StreamEncoderWriteStatus to a C string.
 *
 *  Using a FLAC__StreamEncoderWriteStatus as the index to this array
 *  will give the string equivalent.  The contents should not be modified.
 */
extern FLAC_API const char * const FLAC__StreamEncoderWriteStatusString[];


/***********************************************************************
 *
 * class FLAC__StreamEncoder
 *
 ***********************************************************************/

struct FLAC__StreamEncoderProtected;
struct FLAC__StreamEncoderPrivate;
/** The opaque structure definition for the stream encoder type.
 *  See the \link flac_stream_encoder stream encoder module \endlink
 *  for a detailed description.
 */
typedef struct {
	struct FLAC__StreamEncoderProtected *protected_; /* avoid the C++ keyword 'protected' */
	struct FLAC__StreamEncoderPrivate *private_; /* avoid the C++ keyword 'private' */
} FLAC__StreamEncoder;

/** Signature for the write callback.
 *  See FLAC__stream_encoder_set_write_callback() for more info.
 *
 * \param  encoder  The encoder instance calling the callback.
 * \param  buffer   An array of encoded data of length \a bytes.
 * \param  bytes    The byte length of \a buffer.
 * \param  samples  The number of samples encoded by \a buffer.
 *                  \c 0 has a special meaning; see
 *                  FLAC__stream_encoder_set_write_callback().
 * \param  current_frame  The number of the current frame being encoded.
 * \param  client_data  The callee's client data set through
 *                      FLAC__stream_encoder_set_client_data().
 * \retval FLAC__StreamEncoderWriteStatus
 *    The callee's return status.
 */
typedef FLAC__StreamEncoderWriteStatus (*FLAC__StreamEncoderWriteCallback)(const FLAC__StreamEncoder *encoder, const FLAC__byte buffer[], unsigned bytes, unsigned samples, unsigned current_frame, void *client_data);

/** Signature for the metadata callback.
 *  See FLAC__stream_encoder_set_metadata_callback() for more info.
 *
 * \param  encoder      The encoder instance calling the callback.
 * \param  metadata     The final populated STREAMINFO block.
 * \param  client_data  The callee's client data set through
 *                      FLAC__stream_encoder_set_client_data().
 */
typedef void (*FLAC__StreamEncoderMetadataCallback)(const FLAC__StreamEncoder *encoder, const FLAC__StreamMetadata *metadata, void *client_data);


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
FLAC_API FLAC__StreamEncoder *FLAC__stream_encoder_new();

/** Free an encoder instance.  Deletes the object pointed to by \a encoder.
 *
 * \param encoder  A pointer to an existing encoder.
 * \assert
 *    \code encoder != NULL \endcode
 */
FLAC_API void FLAC__stream_encoder_delete(FLAC__StreamEncoder *encoder);


/***********************************************************************
 *
 * Public class method prototypes
 *
 ***********************************************************************/

/** Set the "verify" flag.  If \c true, the encoder will verify it's own
 *  encoded output by feeding it through an internal decoder and comparing
 *  the original signal against the decoded signal.  If a mismatch occurs,
 *  the process call will return \c false.  Note that this will slow the
 *  encoding process by the extra time required for decoding and comparison.
 *
 * \default \c false
 * \param  encoder  An encoder instance to set.
 * \param  value    Flag value (see above).
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC_API FLAC__bool FLAC__stream_encoder_set_verify(FLAC__StreamEncoder *encoder, FLAC__bool value);

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
FLAC_API FLAC__bool FLAC__stream_encoder_set_streamable_subset(FLAC__StreamEncoder *encoder, FLAC__bool value);

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
FLAC_API FLAC__bool FLAC__stream_encoder_set_do_mid_side_stereo(FLAC__StreamEncoder *encoder, FLAC__bool value);

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
FLAC_API FLAC__bool FLAC__stream_encoder_set_loose_mid_side_stereo(FLAC__StreamEncoder *encoder, FLAC__bool value);

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
FLAC_API FLAC__bool FLAC__stream_encoder_set_channels(FLAC__StreamEncoder *encoder, unsigned value);

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
FLAC_API FLAC__bool FLAC__stream_encoder_set_bits_per_sample(FLAC__StreamEncoder *encoder, unsigned value);

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
FLAC_API FLAC__bool FLAC__stream_encoder_set_sample_rate(FLAC__StreamEncoder *encoder, unsigned value);

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
FLAC_API FLAC__bool FLAC__stream_encoder_set_blocksize(FLAC__StreamEncoder *encoder, unsigned value);

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
FLAC_API FLAC__bool FLAC__stream_encoder_set_max_lpc_order(FLAC__StreamEncoder *encoder, unsigned value);

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
FLAC_API FLAC__bool FLAC__stream_encoder_set_qlp_coeff_precision(FLAC__StreamEncoder *encoder, unsigned value);

/** Set to \c false to use only the specified quantized linear predictor
 *  coefficient precision, or \c true to search neighboring precision
 *  values and use the best one.
 *
 * \default \c false
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC_API FLAC__bool FLAC__stream_encoder_set_do_qlp_coeff_prec_search(FLAC__StreamEncoder *encoder, FLAC__bool value);

/** Deprecated.  Setting this value has no effect.
 *
 * \default \c false
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC_API FLAC__bool FLAC__stream_encoder_set_do_escape_coding(FLAC__StreamEncoder *encoder, FLAC__bool value);

/** Set to \c false to let the encoder estimate the best model order
 *  based on the residual signal energy, or \c true to force the
 *  encoder to evaluate all order models and select the best.
 *
 * \default \c false
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC_API FLAC__bool FLAC__stream_encoder_set_do_exhaustive_model_search(FLAC__StreamEncoder *encoder, FLAC__bool value);

/** Set the minimum partition order to search when coding the residual.
 *  This is used in tandem with
 *  FLAC__stream_encoder_set_max_residual_partition_order().
 *
 *  The partition order determines the context size in the residual.
 *  The context size will be approximately <tt>blocksize / (2 ^ order)</tt>.
 *
 *  Set both min and max values to \c 0 to force a single context,
 *  whose Rice parameter is based on the residual signal variance.
 *  Otherwise, set a min and max order, and the encoder will search
 *  all orders, using the mean of each context for its Rice parameter,
 *  and use the best.
 *
 * \default \c 0
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC_API FLAC__bool FLAC__stream_encoder_set_min_residual_partition_order(FLAC__StreamEncoder *encoder, unsigned value);

/** Set the maximum partition order to search when coding the residual.
 *  This is used in tandem with
 *  FLAC__stream_encoder_set_min_residual_partition_order().
 *
 *  The partition order determines the context size in the residual.
 *  The context size will be approximately <tt>blocksize / (2 ^ order)</tt>.
 *
 *  Set both min and max values to \c 0 to force a single context,
 *  whose Rice parameter is based on the residual signal variance.
 *  Otherwise, set a min and max order, and the encoder will search
 *  all orders, using the mean of each context for its Rice parameter,
 *  and use the best.
 *
 * \default \c 0
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC_API FLAC__bool FLAC__stream_encoder_set_max_residual_partition_order(FLAC__StreamEncoder *encoder, unsigned value);

/** Deprecated.  Setting this value has no effect.
 *
 * \default \c 0
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC_API FLAC__bool FLAC__stream_encoder_set_rice_parameter_search_dist(FLAC__StreamEncoder *encoder, unsigned value);

/** Set an estimate of the total samples that will be encoded.
 *  This is merely an estimate and may be set to \c 0 if unknown.
 *  This value will be written to the STREAMINFO block before encoding,
 *  and can remove the need for the caller to rewrite the value later
 *  if the value is known before encoding.
 *
 * \default \c 0
 * \param  encoder  An encoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the encoder is already initialized, else \c true.
 */
FLAC_API FLAC__bool FLAC__stream_encoder_set_total_samples_estimate(FLAC__StreamEncoder *encoder, FLAC__uint64 value);

/** Set the metadata blocks to be emitted to the stream before encoding.
 *  A value of \c NULL, \c 0 implies no metadata; otherwise, supply an
 *  array of pointers to metadata blocks.  The array is non-const since
 *  the encoder may need to change the \a is_last flag inside them.
 *  Otherwise, the encoder will not modify or free the blocks.  It is up
 *  to the caller to free the metadata blocks after encoding.
 *
 * \note
 * The encoder stores only the \a metadata pointer; the passed-in array
 * must survive at least until after FLAC__stream_encoder_init() returns.
 * Do not modify the array or free the blocks until then.
 *
 * \note
 * The STREAMINFO block is always written and no STREAMINFO block may
 * occur in the supplied array.
 *
 * \note
 * By default the encoder does not create a SEEKTABLE.  If one is supplied
 * in the \a metadata array it will be written verbatim.  However by itself
 * this is not very useful as the user will not know the stream offsets for
 * the seekpoints ahead of time.  You must use the seekable stream encoder
 * to generate a legal seektable
 * (see FLAC__seekable_stream_encoder_set_metadata())
 *
 * \note
 * A VORBIS_COMMENT block may be supplied.  The vendor string in it
 * will be ignored.  libFLAC will use it's own vendor string. libFLAC
 * will not modify the passed-in VORBIS_COMMENT's vendor string, it
 * will simply write it's own into the stream.  If no VORBIS_COMMENT
 * block is present in the \a metadata array, libFLAC will write an
 * empty one, containing only the vendor string.
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
FLAC_API FLAC__bool FLAC__stream_encoder_set_metadata(FLAC__StreamEncoder *encoder, FLAC__StreamMetadata **metadata, unsigned num_blocks);

/** Set the write callback.
 *  The supplied function will be called by the encoder anytime there is raw
 *  encoded data ready to write.  It may include metadata mixed with encoded
 *  audio frames and the data is not guaranteed to be aligned on frame or
 *  metadata block boundaries.
 *
 *  The only duty of the callback is to write out the \a bytes worth of data
 *  in \a buffer to the current position in the output stream.  The arguments
 *  \a samples and \a current_frame are purely informational.  If \a samples
 *  is greater than \c 0, then \a current_frame will hold the current frame
 *  number that is being written; otherwise, the write callback is being called
 *  to write metadata.
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
FLAC_API FLAC__bool FLAC__stream_encoder_set_write_callback(FLAC__StreamEncoder *encoder, FLAC__StreamEncoderWriteCallback value);

/** Set the metadata callback.
 *  The supplied function will be called once at the end of encoding with
 *  the populated STREAMINFO structure.  This is so file encoders can seek
 *  back to the beginning of the file and write the STREAMINFO block with
 *  the correct statistics after encoding (like minimum/maximum frame size
 *  and total samples).
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
FLAC_API FLAC__bool FLAC__stream_encoder_set_metadata_callback(FLAC__StreamEncoder *encoder, FLAC__StreamEncoderMetadataCallback value);

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
FLAC_API FLAC__bool FLAC__stream_encoder_set_client_data(FLAC__StreamEncoder *encoder, void *value);

/** Get the current encoder state.
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__StreamEncoderState
 *    The current encoder state.
 */
FLAC_API FLAC__StreamEncoderState FLAC__stream_encoder_get_state(const FLAC__StreamEncoder *encoder);

/** Get the state of the verify stream decoder.
 *  Useful when the stream encoder state is
 *  \c FLAC__STREAM_ENCODER_VERIFY_DECODER_ERROR.
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__StreamDecoderState
 *    The verify stream decoder state.
 */
FLAC_API FLAC__StreamDecoderState FLAC__stream_encoder_get_verify_decoder_state(const FLAC__StreamEncoder *encoder);

/** Get the current encoder state as a C string.
 *  This version automatically resolves
 *  \c FLAC__STREAM_ENCODER_VERIFY_DECODER_ERROR by getting the
 *  verify decoder's state.
 *
 * \param  encoder  A encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval const char *
 *    The encoder state as a C string.  Do not modify the contents.
 */
FLAC_API const char *FLAC__stream_encoder_get_resolved_state_string(const FLAC__StreamEncoder *encoder);

/** Get relevant values about the nature of a verify decoder error.
 *  Useful when the stream encoder state is
 *  \c FLAC__STREAM_ENCODER_VERIFY_DECODER_ERROR.  The arguments should
 *  be addresses in which the stats will be returned, or NULL if value
 *  is not desired.
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
FLAC_API void FLAC__stream_encoder_get_verify_decoder_error_stats(const FLAC__StreamEncoder *encoder, FLAC__uint64 *absolute_sample, unsigned *frame_number, unsigned *channel, unsigned *sample, FLAC__int32 *expected, FLAC__int32 *got);

/** Get the "verify" flag.
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    See FLAC__stream_encoder_set_verify().
 */
FLAC_API FLAC__bool FLAC__stream_encoder_get_verify(const FLAC__StreamEncoder *encoder);

/** Get the "streamable subset" flag.
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    See FLAC__stream_encoder_set_streamable_subset().
 */
FLAC_API FLAC__bool FLAC__stream_encoder_get_streamable_subset(const FLAC__StreamEncoder *encoder);

/** Get the "mid/side stereo coding" flag.
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    See FLAC__stream_encoder_get_do_mid_side_stereo().
 */
FLAC_API FLAC__bool FLAC__stream_encoder_get_do_mid_side_stereo(const FLAC__StreamEncoder *encoder);

/** Get the "adaptive mid/side switching" flag.
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    See FLAC__stream_encoder_set_loose_mid_side_stereo().
 */
FLAC_API FLAC__bool FLAC__stream_encoder_get_loose_mid_side_stereo(const FLAC__StreamEncoder *encoder);

/** Get the number of input channels being processed.
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval unsigned
 *    See FLAC__stream_encoder_set_channels().
 */
FLAC_API unsigned FLAC__stream_encoder_get_channels(const FLAC__StreamEncoder *encoder);

/** Get the input sample resolution setting.
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval unsigned
 *    See FLAC__stream_encoder_set_bits_per_sample().
 */
FLAC_API unsigned FLAC__stream_encoder_get_bits_per_sample(const FLAC__StreamEncoder *encoder);

/** Get the input sample rate setting.
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval unsigned
 *    See FLAC__stream_encoder_set_sample_rate().
 */
FLAC_API unsigned FLAC__stream_encoder_get_sample_rate(const FLAC__StreamEncoder *encoder);

/** Get the blocksize setting.
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval unsigned
 *    See FLAC__stream_encoder_set_blocksize().
 */
FLAC_API unsigned FLAC__stream_encoder_get_blocksize(const FLAC__StreamEncoder *encoder);

/** Get the maximum LPC order setting.
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval unsigned
 *    See FLAC__stream_encoder_set_max_lpc_order().
 */
FLAC_API unsigned FLAC__stream_encoder_get_max_lpc_order(const FLAC__StreamEncoder *encoder);

/** Get the quantized linear predictor coefficient precision setting.
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval unsigned
 *    See FLAC__stream_encoder_set_qlp_coeff_precision().
 */
FLAC_API unsigned FLAC__stream_encoder_get_qlp_coeff_precision(const FLAC__StreamEncoder *encoder);

/** Get the qlp coefficient precision search flag.
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    See FLAC__stream_encoder_set_do_qlp_coeff_prec_search().
 */
FLAC_API FLAC__bool FLAC__stream_encoder_get_do_qlp_coeff_prec_search(const FLAC__StreamEncoder *encoder);

/** Get the "escape coding" flag.
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    See FLAC__stream_encoder_set_do_escape_coding().
 */
FLAC_API FLAC__bool FLAC__stream_encoder_get_do_escape_coding(const FLAC__StreamEncoder *encoder);

/** Get the exhaustive model search flag.
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__bool
 *    See FLAC__stream_encoder_set_do_exhaustive_model_search().
 */
FLAC_API FLAC__bool FLAC__stream_encoder_get_do_exhaustive_model_search(const FLAC__StreamEncoder *encoder);

/** Get the minimum residual partition order setting.
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval unsigned
 *    See FLAC__stream_encoder_set_min_residual_partition_order().
 */
FLAC_API unsigned FLAC__stream_encoder_get_min_residual_partition_order(const FLAC__StreamEncoder *encoder);

/** Get maximum residual partition order setting.
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval unsigned
 *    See FLAC__stream_encoder_set_max_residual_partition_order().
 */
FLAC_API unsigned FLAC__stream_encoder_get_max_residual_partition_order(const FLAC__StreamEncoder *encoder);

/** Get the Rice parameter search distance setting.
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval unsigned
 *    See FLAC__stream_encoder_set_rice_parameter_search_dist().
 */
FLAC_API unsigned FLAC__stream_encoder_get_rice_parameter_search_dist(const FLAC__StreamEncoder *encoder);

/** Get the previously set estimate of the total samples to be encoded.
 *  The encoder merely mimics back the value given to
 *  FLAC__stream_encoder_set_total_samples_estimate() since it has no
 *  other way of knowing how many samples the user will encode.
 *
 * \param  encoder  An encoder instance to set.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__uint64
 *    See FLAC__stream_encoder_get_total_samples_estimate().
 */
FLAC_API FLAC__uint64 FLAC__stream_encoder_get_total_samples_estimate(const FLAC__StreamEncoder *encoder);

/** Initialize the encoder instance.
 *  Should be called after FLAC__stream_encoder_new() and
 *  FLAC__stream_encoder_set_*() but before FLAC__stream_encoder_process()
 *  or FLAC__stream_encoder_process_interleaved().  Will set and return
 *  the encoder state, which will be FLAC__STREAM_ENCODER_OK if
 *  initialization succeeded.
 *
 *  The call to FLAC__stream_encoder_init() currently will also immediately
 *  call the write callback several times, once with the \c fLaC signature,
 *  and once for each encoded metadata block.
 *
 * \param  encoder  An uninitialized encoder instance.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__StreamEncoderState
 *    \c FLAC__STREAM_ENCODER_OK if initialization was successful; see
 *    FLAC__StreamEncoderState for the meanings of other return values.
 */
FLAC_API FLAC__StreamEncoderState FLAC__stream_encoder_init(FLAC__StreamEncoder *encoder);

/** Finish the encoding process.
 *  Flushes the encoding buffer, releases resources, resets the encoder
 *  settings to their defaults, and returns the encoder state to
 *  FLAC__STREAM_ENCODER_UNINITIALIZED.  Note that this can generate
 *  one or more write callbacks before returning, and will generate
 *  a metadata callback.
 *
 *  In the event of a prematurely-terminated encode, it is not strictly
 *  necessary to call this immediately before FLAC__stream_encoder_delete()
 *  but it is good practice to match every FLAC__stream_encoder_init()
 *  with a FLAC__stream_encoder_finish().
 *
 * \param  encoder  An uninitialized encoder instance.
 * \assert
 *    \code encoder != NULL \endcode
 */
FLAC_API void FLAC__stream_encoder_finish(FLAC__StreamEncoder *encoder);

/** Submit data for encoding.
 *  This version allows you to supply the input data via an array of
 *  pointers, each pointer pointing to an array of \a samples samples
 *  representing one channel.  The samples need not be block-aligned,
 *  but each channel should have the same number of samples.
 *
 * \param  encoder  An initialized encoder instance in the OK state.
 * \param  buffer   An array of pointers to each channel's signal.
 * \param  samples  The number of samples in one channel.
 * \assert
 *    \code encoder != NULL \endcode
 *    \code FLAC__stream_encoder_get_state(encoder) == FLAC__STREAM_ENCODER_OK \endcode
 * \retval FLAC__bool
 *    \c true if successful, else \c false; in this case, check the
 *    encoder state with FLAC__stream_encoder_get_state() to see what
 *    went wrong.
 */
FLAC_API FLAC__bool FLAC__stream_encoder_process(FLAC__StreamEncoder *encoder, const FLAC__int32 * const buffer[], unsigned samples);

/** Submit data for encoding.
 *  This version allows you to supply the input data where the channels
 *  are interleaved into a single array (i.e. channel0_sample0,
 *  channel1_sample0, ... , channelN_sample0, channel0_sample1, ...).
 *  The samples need not be block-aligned but they must be
 *  sample-aligned, i.e. the first value should be channel0_sample0
 *  and the last value channelN_sampleM.
 *
 * \param  encoder  An initialized encoder instance in the OK state.
 * \param  buffer   An array of channel-interleaved data (see above).
 * \param  samples  The number of samples in one channel, the same as for
 *                  FLAC__stream_encoder_process().  For example, if
 *                  encoding two channels, \c 1000 \a samples corresponds
 *                  to a \a buffer of 2000 values.
 * \assert
 *    \code encoder != NULL \endcode
 *    \code FLAC__stream_encoder_get_state(encoder) == FLAC__STREAM_ENCODER_OK \endcode
 * \retval FLAC__bool
 *    \c true if successful, else \c false; in this case, check the
 *    encoder state with FLAC__stream_encoder_get_state() to see what
 *    went wrong.
 */
FLAC_API FLAC__bool FLAC__stream_encoder_process_interleaved(FLAC__StreamEncoder *encoder, const FLAC__int32 buffer[], unsigned samples);

/* \} */

#ifdef __cplusplus
}
#endif

#endif
