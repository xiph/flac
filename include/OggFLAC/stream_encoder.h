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

#ifndef OggFLAC__STREAM_ENCODER_H
#define OggFLAC__STREAM_ENCODER_H

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
 *  This module describes the three encoder layers provided by libOggFLAC.
 *
 * libOggFLAC provides the same three layers of access as libFLAC and the
 * interface is identical.  See the \link flac_encoder FLAC encoder module
 * \endlink for full documentation.
 */

/** \defgroup oggflac_stream_encoder OggFLAC/stream_encoder.h: stream encoder interface
 *  \ingroup oggflac_encoder
 *
 *  \brief
 *  This module contains the functions which implement the stream
 *  encoder.
 *
 * The interface here is identical to FLAC's stream encoder.  See the
 * defaults, including the callbacks.  See the \link flac_stream_encoder
 * FLAC stream encoder module \endlink for full documentation.
 * \{
 */


/** State values for a OggFLAC__StreamEncoder
 *
 *  The encoder's state can be obtained by calling OggFLAC__stream_encoder_get_state().
 */
typedef enum {

	OggFLAC__STREAM_ENCODER_OK = 0,
	/**< The encoder is in the normal OK state. */

	OggFLAC__STREAM_ENCODER_FLAC_ENCODER_ERROR,
	/**< An error occurred in the underlying FLAC stream encoder;
	 * check OggFLAC__stream_encoder_get_FLAC_encoder_state().
	 */

	OggFLAC__STREAM_ENCODER_INVALID_CALLBACK,
	/**< The encoder was initialized before setting all the required callbacks. */

	OggFLAC__STREAM_ENCODER_MEMORY_ALLOCATION_ERROR,
	/**< Memory allocation failed. */

	OggFLAC__STREAM_ENCODER_ALREADY_INITIALIZED,
	/**< OggFLAC__stream_encoder_init() was called when the encoder was
	 * already initialized, usually because
	 * OggFLAC__stream_encoder_finish() was not called.
	 */

	OggFLAC__STREAM_ENCODER_UNINITIALIZED
	/**< The encoder is in the uninitialized state. */

} OggFLAC__StreamEncoderState;

/** Maps an OggFLAC__StreamEncoderState to a C string.
 *
 *  Using a OggFLAC__StreamEncoderState as the index to this array
 *  will give the string equivalent.  The contents should not be modified.
 */
extern const char * const OggFLAC__StreamEncoderStateString[];

/** Return values for the OggFLAC__StreamEncoder write callback.
 */
typedef enum {

	OggFLAC__STREAM_ENCODER_WRITE_STATUS_OK = 0,
	/**< The write was OK and encoding can continue. */

	OggFLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR
	/**< An unrecoverable error occurred.  The encoder will return from the process call. */

} OggFLAC__StreamEncoderWriteStatus;

/** Maps an OggFLAC__StreamEncoderWriteStatus to a C string.
 *
 *  Using a OggFLAC__StreamEncoderWriteStatus as the index to this array
 *  will give the string equivalent.  The contents should not be modified.
 */
extern const char * const OggFLAC__StreamEncoderWriteStatusString[];


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
	struct OggFLAC__StreamEncoderProtected *protected_; /* avoid the C++ keyword 'protected' */
	struct OggFLAC__StreamEncoderPrivate *private_; /* avoid the C++ keyword 'private' */
} OggFLAC__StreamEncoder;

/*@@@@ document: */
typedef OggFLAC__StreamEncoderWriteStatus (*OggFLAC__StreamEncoderWriteCallback)(const OggFLAC__StreamEncoder *encoder, const FLAC__byte buffer[], unsigned bytes, unsigned samples, unsigned current_frame, void *client_data);
typedef void (*OggFLAC__StreamEncoderMetadataCallback)(const OggFLAC__StreamEncoder *encoder, const OggFLAC__StreamMetadata *metadata, void *client_data);


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
OggFLAC__StreamEncoder *OggFLAC__stream_encoder_new();

/** Free an encoder instance.  Deletes the object pointed to by \a encoder.
 *
 * \param encoder  A pointer to an existing encoder.
 * \assert
 *    \code encoder != NULL \endcode
 */
void OggFLAC__stream_encoder_delete(OggFLAC__StreamEncoder *encoder);

/***********************************************************************
 *
 * Public class method prototypes
 *
 ***********************************************************************/

/** Inherited from FLAC__stream_encoder_set_verify()
 */
FLAC__bool OggFLAC__stream_encoder_set_verify(OggFLAC__StreamEncoder *encoder, FLAC__bool value);

/** Inherited from FLAC__stream_encoder_set_streamable_subset()
 */
FLAC__bool OggFLAC__stream_encoder_set_streamable_subset(OggFLAC__StreamEncoder *encoder, FLAC__bool value);

/** Inherited from FLAC__stream_encoder_set_do_mid_side_stereo()
 */
FLAC__bool OggFLAC__stream_encoder_set_do_mid_side_stereo(OggFLAC__StreamEncoder *encoder, FLAC__bool value);

/** Inherited from FLAC__stream_encoder_set_loose_mid_side_stereo()
 */
FLAC__bool OggFLAC__stream_encoder_set_loose_mid_side_stereo(OggFLAC__StreamEncoder *encoder, FLAC__bool value);

/** Inherited from FLAC__stream_encoder_set_channels()
 */
FLAC__bool OggFLAC__stream_encoder_set_channels(OggFLAC__StreamEncoder *encoder, unsigned value);

/** Inherited from FLAC__stream_encoder_set_bits_per_sample()
 */
FLAC__bool OggFLAC__stream_encoder_set_bits_per_sample(OggFLAC__StreamEncoder *encoder, unsigned value);

/** Inherited from FLAC__stream_encoder_set_sample_rate()
 */
FLAC__bool OggFLAC__stream_encoder_set_sample_rate(OggFLAC__StreamEncoder *encoder, unsigned value);

/** Inherited from FLAC__stream_encoder_set_blocksize()
 */
FLAC__bool OggFLAC__stream_encoder_set_blocksize(OggFLAC__StreamEncoder *encoder, unsigned value);

/** Inherited from FLAC__stream_encoder_set_max_lpc_order()
 */
FLAC__bool OggFLAC__stream_encoder_set_max_lpc_order(OggFLAC__StreamEncoder *encoder, unsigned value);

/** Inherited from FLAC__stream_encoder_set_qlp_coeff_precision()
 */
FLAC__bool OggFLAC__stream_encoder_set_qlp_coeff_precision(OggFLAC__StreamEncoder *encoder, unsigned value);

/** Inherited from FLAC__stream_encoder_set_do_qlp_coeff_prec_search()
 */
FLAC__bool OggFLAC__stream_encoder_set_do_qlp_coeff_prec_search(OggFLAC__StreamEncoder *encoder, FLAC__bool value);

/** Inherited from FLAC__stream_encoder_set_do_escape_coding()
 */
FLAC__bool OggFLAC__stream_encoder_set_do_escape_coding(OggFLAC__StreamEncoder *encoder, FLAC__bool value);

/** Inherited from FLAC__stream_encoder_set_do_exhaustive_model_search()
 */
FLAC__bool OggFLAC__stream_encoder_set_do_exhaustive_model_search(OggFLAC__StreamEncoder *encoder, FLAC__bool value);

/** Inherited from FLAC__stream_encoder_set_min_residual_partition_order()
 */
FLAC__bool OggFLAC__stream_encoder_set_min_residual_partition_order(OggFLAC__StreamEncoder *encoder, unsigned value);

/** Inherited from FLAC__stream_encoder_set_max_residual_partition_order()
 */
FLAC__bool OggFLAC__stream_encoder_set_max_residual_partition_order(OggFLAC__StreamEncoder *encoder, unsigned value);

/** Inherited from FLAC__stream_encoder_set_rice_parameter_search_dist()
 */
FLAC__bool OggFLAC__stream_encoder_set_rice_parameter_search_dist(OggFLAC__StreamEncoder *encoder, unsigned value);

/** Inherited from FLAC__stream_encoder_set_total_samples_estimate()
 */
FLAC__bool OggFLAC__stream_encoder_set_total_samples_estimate(OggFLAC__StreamEncoder *encoder, FLAC__uint64 value);

/** Inherited from FLAC__stream_encoder_set_metadata()
 */
FLAC__bool OggFLAC__stream_encoder_set_metadata(OggFLAC__StreamEncoder *encoder, OggFLAC__StreamMetadata **metadata, unsigned num_blocks);

/** Inherited from FLAC__stream_encoder_set_write_callback()
 */
FLAC__bool OggFLAC__stream_encoder_set_write_callback(OggFLAC__StreamEncoder *encoder, OggFLAC__StreamEncoderWriteCallback value);

/** Inherited from FLAC__stream_encoder_set_metadata_callback()
 */
FLAC__bool OggFLAC__stream_encoder_set_metadata_callback(OggFLAC__StreamEncoder *encoder, OggFLAC__StreamEncoderMetadataCallback value);

/** Inherited from FLAC__stream_encoder_set_client_data()
 */
FLAC__bool OggFLAC__stream_encoder_set_client_data(OggFLAC__StreamEncoder *encoder, void *value);

/** Inherited from FLAC__stream_encoder_get_state()
 */
OggFLAC__StreamEncoderState OggFLAC__stream_encoder_get_state(const OggFLAC__StreamEncoder *encoder);

/** Get the state of the underlying FLAC encoder.
 *  Useful when the stream encoder state is
 *  \c OggFLAC__STREAM_ENCODER_FLAC_ENCODER_ERROR.
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__StreamEncoderState
 *    The stream encoder state.
 */
FLAC__StreamEncoderState OggFLAC__stream_encoder_get_FLAC_encoder_state(const OggFLAC__StreamEncoder *encoder);

/** Get the state of the FLAC encoder's verify decoder.
 *  Useful when the stream encoder state is
 *  \c OggFLAC__STREAM_ENCODER_FLAC_ENCODER_ERROR and the
 *  FLAC encoder state is \c OggFLAC__STREAM_ENCODER_VERIFY_DECODER_ERROR.
 *
 * \param  encoder  An encoder instance to query.
 * \assert
 *    \code encoder != NULL \endcode
 * \retval FLAC__StreamDecoderState
 *    The stream encoder state.
 */
FLAC__StreamDecoderState OggFLAC__stream_encoder_get_verify_decoder_state(const OggFLAC__StreamEncoder *encoder);

/** Inherited from FLAC__stream_encoder_get_verify()
 */
FLAC__bool OggFLAC__stream_encoder_get_verify(const OggFLAC__StreamEncoder *encoder);

/** Inherited from FLAC__stream_encoder_get_streamable_subset()
 */
FLAC__bool OggFLAC__stream_encoder_get_streamable_subset(const OggFLAC__StreamEncoder *encoder);

/** Inherited from FLAC__stream_encoder_get_do_mid_side_stereo()
 */
FLAC__bool OggFLAC__stream_encoder_get_do_mid_side_stereo(const OggFLAC__StreamEncoder *encoder);

/** Inherited from FLAC__stream_encoder_get_loose_mid_side_stereo()
 */
FLAC__bool OggFLAC__stream_encoder_get_loose_mid_side_stereo(const OggFLAC__StreamEncoder *encoder);

/** Inherited from FLAC__stream_encoder_get_channels()
 */
unsigned OggFLAC__stream_encoder_get_channels(const OggFLAC__StreamEncoder *encoder);

/** Inherited from FLAC__stream_encoder_get_bits_per_sample()
 */
unsigned OggFLAC__stream_encoder_get_bits_per_sample(const OggFLAC__StreamEncoder *encoder);

/** Inherited from FLAC__stream_encoder_get_sample_rate()
 */
unsigned OggFLAC__stream_encoder_get_sample_rate(const OggFLAC__StreamEncoder *encoder);

/** Inherited from FLAC__stream_encoder_get_blocksize()
 */
unsigned OggFLAC__stream_encoder_get_blocksize(const OggFLAC__StreamEncoder *encoder);

/** Inherited from FLAC__stream_encoder_get_max_lpc_order()
 */
unsigned OggFLAC__stream_encoder_get_max_lpc_order(const OggFLAC__StreamEncoder *encoder);

/** Inherited from FLAC__stream_encoder_get_qlp_coeff_precision()
 */
unsigned OggFLAC__stream_encoder_get_qlp_coeff_precision(const OggFLAC__StreamEncoder *encoder);

/** Inherited from FLAC__stream_encoder_get_do_qlp_coeff_prec_search()
 */
FLAC__bool OggFLAC__stream_encoder_get_do_qlp_coeff_prec_search(const OggFLAC__StreamEncoder *encoder);

/** Inherited from FLAC__stream_encoder_get_do_escape_coding()
 */
FLAC__bool OggFLAC__stream_encoder_get_do_escape_coding(const OggFLAC__StreamEncoder *encoder);

/** Inherited from FLAC__stream_encoder_get_do_exhaustive_model_search()
 */
FLAC__bool OggFLAC__stream_encoder_get_do_exhaustive_model_search(const OggFLAC__StreamEncoder *encoder);

/** Inherited from FLAC__stream_encoder_get_min_residual_partition_order()
 */
unsigned OggFLAC__stream_encoder_get_min_residual_partition_order(const OggFLAC__StreamEncoder *encoder);

/** Inherited from FLAC__stream_encoder_get_max_residual_partition_order()
 */
unsigned OggFLAC__stream_encoder_get_max_residual_partition_order(const OggFLAC__StreamEncoder *encoder);

/** Inherited from FLAC__stream_encoder_get_rice_parameter_search_dist()
 */
unsigned OggFLAC__stream_encoder_get_rice_parameter_search_dist(const OggFLAC__StreamEncoder *encoder);

/** Inherited from FLAC__stream_encoder_get_total_samples_estimate()
 */
FLAC__uint64 OggFLAC__stream_encoder_get_total_samples_estimate(const OggFLAC__StreamEncoder *encoder);

/** Inherited from FLAC__stream_encoder_init()
 */
OggFLAC__StreamEncoderState OggFLAC__stream_encoder_init(OggFLAC__StreamEncoder *encoder);

/** Inherited from FLAC__stream_encoder_finish()
 */
void OggFLAC__stream_encoder_finish(OggFLAC__StreamEncoder *encoder);

/** Inherited from FLAC__stream_encoder_process()
 */
FLAC__bool OggFLAC__stream_encoder_process(OggFLAC__StreamEncoder *encoder, const FLAC__int32 * const buffer[], unsigned samples);

/** Inherited from FLAC__stream_encoder_process_interleaved()
 */
FLAC__bool OggFLAC__stream_encoder_process_interleaved(OggFLAC__StreamEncoder *encoder, const FLAC__int32 buffer[], unsigned samples);

/* \} */

#ifdef __cplusplus
}
#endif

#endif
