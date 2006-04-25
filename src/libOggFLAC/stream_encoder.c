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

#include <stdio.h>
#include <stdlib.h> /* for calloc() */
#include "FLAC/assert.h"
#include "OggFLAC/stream_encoder.h"
#include "protected/stream_encoder.h"

/***********************************************************************
 *
 * Private class method prototypes
 *
 ***********************************************************************/

/* unpublished debug routines */
extern FLAC__bool FLAC__stream_encoder_disable_constant_subframes(FLAC__StreamEncoder *encoder, FLAC__bool value);
extern FLAC__bool FLAC__stream_encoder_disable_fixed_subframes(FLAC__StreamEncoder *encoder, FLAC__bool value);
extern FLAC__bool FLAC__stream_encoder_disable_verbatim_subframes(FLAC__StreamEncoder *encoder, FLAC__bool value);

static void set_defaults_(OggFLAC__StreamEncoder *encoder);
static FLAC__StreamEncoderWriteStatus write_callback_(const FLAC__StreamEncoder *encoder, const FLAC__byte buffer[], unsigned bytes, unsigned samples, unsigned current_frame, void *client_data);
static void metadata_callback_(const FLAC__StreamEncoder *encoder, const FLAC__StreamMetadata *metadata, void *client_data);


/***********************************************************************
 *
 * Private class data
 *
 ***********************************************************************/

typedef struct OggFLAC__StreamEncoderPrivate {
	OggFLAC__StreamEncoderWriteCallback write_callback;
	OggFLAC__StreamEncoderMetadataCallback metadata_callback;
	void *client_data;
	FLAC__StreamEncoder *FLAC_stream_encoder;
} OggFLAC__StreamEncoderPrivate;


/***********************************************************************
 *
 * Public static class data
 *
 ***********************************************************************/

OggFLAC_API const char * const OggFLAC__StreamEncoderStateString[] = {
	"OggFLAC__STREAM_ENCODER_OK",
	"OggFLAC__STREAM_ENCODER_OGG_ERROR",
	"OggFLAC__STREAM_ENCODER_FLAC_STREAM_ENCODER_ERROR",
	"OggFLAC__STREAM_ENCODER_INVALID_CALLBACK",
	"OggFLAC__STREAM_ENCODER_MEMORY_ALLOCATION_ERROR",
	"OggFLAC__STREAM_ENCODER_ALREADY_INITIALIZED",
	"OggFLAC__STREAM_ENCODER_UNINITIALIZED"
};


/***********************************************************************
 *
 * Class constructor/destructor
 *
 */
OggFLAC_API OggFLAC__StreamEncoder *OggFLAC__stream_encoder_new()
{
	OggFLAC__StreamEncoder *encoder;

	encoder = (OggFLAC__StreamEncoder*)calloc(1, sizeof(OggFLAC__StreamEncoder));
	if(encoder == 0) {
		return 0;
	}

	encoder->protected_ = (OggFLAC__StreamEncoderProtected*)calloc(1, sizeof(OggFLAC__StreamEncoderProtected));
	if(encoder->protected_ == 0) {
		free(encoder);
		return 0;
	}

	encoder->private_ = (OggFLAC__StreamEncoderPrivate*)calloc(1, sizeof(OggFLAC__StreamEncoderPrivate));
	if(encoder->private_ == 0) {
		free(encoder->protected_);
		free(encoder);
		return 0;
	}

	encoder->private_->FLAC_stream_encoder = FLAC__stream_encoder_new();
	if(0 == encoder->private_->FLAC_stream_encoder) {
		free(encoder->private_);
		free(encoder->protected_);
		free(encoder);
		return 0;
	}

	set_defaults_(encoder);

	encoder->protected_->state = OggFLAC__STREAM_ENCODER_UNINITIALIZED;

	return encoder;
}

OggFLAC_API void OggFLAC__stream_encoder_delete(OggFLAC__StreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);

	(void)OggFLAC__stream_encoder_finish(encoder);

	FLAC__stream_encoder_delete(encoder->private_->FLAC_stream_encoder);

	free(encoder->private_);
	free(encoder->protected_);
	free(encoder);
}


/***********************************************************************
 *
 * Public class methods
 *
 ***********************************************************************/

OggFLAC_API OggFLAC__StreamEncoderState OggFLAC__stream_encoder_init(OggFLAC__StreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);

	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return encoder->protected_->state = OggFLAC__STREAM_ENCODER_ALREADY_INITIALIZED;

	if(0 == encoder->private_->write_callback || 0 == encoder->private_->metadata_callback)
		return encoder->protected_->state = OggFLAC__STREAM_ENCODER_INVALID_CALLBACK;

	if(!OggFLAC__ogg_encoder_aspect_init(&encoder->protected_->ogg_encoder_aspect))
			return encoder->protected_->state = OggFLAC__STREAM_ENCODER_OGG_ERROR;

	FLAC__stream_encoder_set_write_callback(encoder->private_->FLAC_stream_encoder, write_callback_);
	FLAC__stream_encoder_set_metadata_callback(encoder->private_->FLAC_stream_encoder, metadata_callback_);
	FLAC__stream_encoder_set_client_data(encoder->private_->FLAC_stream_encoder, encoder);

	if(FLAC__stream_encoder_init(encoder->private_->FLAC_stream_encoder) != FLAC__STREAM_ENCODER_OK)
		return encoder->protected_->state = OggFLAC__STREAM_ENCODER_FLAC_STREAM_ENCODER_ERROR;

	return encoder->protected_->state = OggFLAC__STREAM_ENCODER_OK;
}

OggFLAC_API void OggFLAC__stream_encoder_finish(OggFLAC__StreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);

	if(encoder->protected_->state == OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return;

	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);

	FLAC__stream_encoder_finish(encoder->private_->FLAC_stream_encoder);

	OggFLAC__ogg_encoder_aspect_finish(&encoder->protected_->ogg_encoder_aspect);

	set_defaults_(encoder);

	encoder->protected_->state = OggFLAC__STREAM_ENCODER_UNINITIALIZED;
}

OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_serial_number(OggFLAC__StreamEncoder *encoder, long value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return false;
	OggFLAC__ogg_encoder_aspect_set_serial_number(&encoder->protected_->ogg_encoder_aspect, value);
	return true;
}

OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_verify(OggFLAC__StreamEncoder *encoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_verify(encoder->private_->FLAC_stream_encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_streamable_subset(OggFLAC__StreamEncoder *encoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_streamable_subset(encoder->private_->FLAC_stream_encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_do_mid_side_stereo(OggFLAC__StreamEncoder *encoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_do_mid_side_stereo(encoder->private_->FLAC_stream_encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_loose_mid_side_stereo(OggFLAC__StreamEncoder *encoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_loose_mid_side_stereo(encoder->private_->FLAC_stream_encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_channels(OggFLAC__StreamEncoder *encoder, unsigned value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_channels(encoder->private_->FLAC_stream_encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_bits_per_sample(OggFLAC__StreamEncoder *encoder, unsigned value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_bits_per_sample(encoder->private_->FLAC_stream_encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_sample_rate(OggFLAC__StreamEncoder *encoder, unsigned value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_sample_rate(encoder->private_->FLAC_stream_encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_blocksize(OggFLAC__StreamEncoder *encoder, unsigned value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_blocksize(encoder->private_->FLAC_stream_encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_apodization(OggFLAC__StreamEncoder *encoder, const char *specification)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_apodization(encoder->private_->FLAC_stream_encoder, specification);
}

OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_max_lpc_order(OggFLAC__StreamEncoder *encoder, unsigned value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_max_lpc_order(encoder->private_->FLAC_stream_encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_qlp_coeff_precision(OggFLAC__StreamEncoder *encoder, unsigned value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_qlp_coeff_precision(encoder->private_->FLAC_stream_encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_do_qlp_coeff_prec_search(OggFLAC__StreamEncoder *encoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_do_qlp_coeff_prec_search(encoder->private_->FLAC_stream_encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_do_escape_coding(OggFLAC__StreamEncoder *encoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_do_escape_coding(encoder->private_->FLAC_stream_encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_do_exhaustive_model_search(OggFLAC__StreamEncoder *encoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_do_exhaustive_model_search(encoder->private_->FLAC_stream_encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_min_residual_partition_order(OggFLAC__StreamEncoder *encoder, unsigned value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_min_residual_partition_order(encoder->private_->FLAC_stream_encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_max_residual_partition_order(OggFLAC__StreamEncoder *encoder, unsigned value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_max_residual_partition_order(encoder->private_->FLAC_stream_encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_rice_parameter_search_dist(OggFLAC__StreamEncoder *encoder, unsigned value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_rice_parameter_search_dist(encoder->private_->FLAC_stream_encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_total_samples_estimate(OggFLAC__StreamEncoder *encoder, FLAC__uint64 value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_total_samples_estimate(encoder->private_->FLAC_stream_encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_metadata(OggFLAC__StreamEncoder *encoder, FLAC__StreamMetadata **metadata, unsigned num_blocks)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return false;
	/* reorder metadata if necessary to ensure that any VORBIS_COMMENT is the first, according to the mapping spec */
	if(0 != metadata && num_blocks > 1) {
		unsigned i;
		for(i = 1; i < num_blocks; i++) {
			if(0 != metadata[i] && metadata[i]->type == FLAC__METADATA_TYPE_VORBIS_COMMENT) {
				FLAC__StreamMetadata *vc = metadata[i];
				for( ; i > 0; i--)
					metadata[i] = metadata[i-1];
				metadata[0] = vc;
				break;
			}
		}
	}
	if(!OggFLAC__ogg_encoder_aspect_set_num_metadata(&encoder->protected_->ogg_encoder_aspect, num_blocks))
		return false;
	return FLAC__stream_encoder_set_metadata(encoder->private_->FLAC_stream_encoder, metadata, num_blocks);
}

OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_write_callback(OggFLAC__StreamEncoder *encoder, OggFLAC__StreamEncoderWriteCallback value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != value);
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return false;
	encoder->private_->write_callback = value;
	return true;
}

OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_metadata_callback(OggFLAC__StreamEncoder *encoder, OggFLAC__StreamEncoderMetadataCallback value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != value);
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return false;
	encoder->private_->metadata_callback = value;
	return true;
}

OggFLAC_API FLAC__bool OggFLAC__stream_encoder_set_client_data(OggFLAC__StreamEncoder *encoder, void *value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return false;
	encoder->private_->client_data = value;
	return true;
}

/*
 * These three functions are not static, but not publically exposed in
 * include/FLAC/ either.  They are used by the test suite.
 */
OggFLAC_API FLAC__bool OggFLAC__stream_encoder_disable_constant_subframes(OggFLAC__StreamEncoder *encoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_disable_constant_subframes(encoder->private_->FLAC_stream_encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__stream_encoder_disable_fixed_subframes(OggFLAC__StreamEncoder *encoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_disable_fixed_subframes(encoder->private_->FLAC_stream_encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__stream_encoder_disable_verbatim_subframes(OggFLAC__StreamEncoder *encoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_disable_verbatim_subframes(encoder->private_->FLAC_stream_encoder, value);
}

OggFLAC_API OggFLAC__StreamEncoderState OggFLAC__stream_encoder_get_state(const OggFLAC__StreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	return encoder->protected_->state;
}

OggFLAC_API FLAC__StreamEncoderState OggFLAC__stream_encoder_get_FLAC_stream_encoder_state(const OggFLAC__StreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	return FLAC__stream_encoder_get_state(encoder->private_->FLAC_stream_encoder);
}

OggFLAC_API FLAC__StreamDecoderState OggFLAC__stream_encoder_get_verify_decoder_state(const OggFLAC__StreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	return FLAC__stream_encoder_get_verify_decoder_state(encoder->private_->FLAC_stream_encoder);
}

OggFLAC_API const char *OggFLAC__stream_encoder_get_resolved_state_string(const OggFLAC__StreamEncoder *encoder)
{
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_FLAC_STREAM_ENCODER_ERROR)
		return OggFLAC__StreamEncoderStateString[encoder->protected_->state];
	else
		return FLAC__stream_encoder_get_resolved_state_string(encoder->private_->FLAC_stream_encoder);
}

OggFLAC_API void OggFLAC__stream_encoder_get_verify_decoder_error_stats(const OggFLAC__StreamEncoder *encoder, FLAC__uint64 *absolute_sample, unsigned *frame_number, unsigned *channel, unsigned *sample, FLAC__int32 *expected, FLAC__int32 *got)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__stream_encoder_get_verify_decoder_error_stats(encoder->private_->FLAC_stream_encoder, absolute_sample, frame_number, channel, sample, expected, got);
}

OggFLAC_API FLAC__bool OggFLAC__stream_encoder_get_verify(const OggFLAC__StreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	return FLAC__stream_encoder_get_verify(encoder->private_->FLAC_stream_encoder);
}

OggFLAC_API FLAC__bool OggFLAC__stream_encoder_get_streamable_subset(const OggFLAC__StreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	return FLAC__stream_encoder_get_streamable_subset(encoder->private_->FLAC_stream_encoder);
}

OggFLAC_API FLAC__bool OggFLAC__stream_encoder_get_do_mid_side_stereo(const OggFLAC__StreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	return FLAC__stream_encoder_get_do_mid_side_stereo(encoder->private_->FLAC_stream_encoder);
}

OggFLAC_API FLAC__bool OggFLAC__stream_encoder_get_loose_mid_side_stereo(const OggFLAC__StreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	return FLAC__stream_encoder_get_loose_mid_side_stereo(encoder->private_->FLAC_stream_encoder);
}

OggFLAC_API unsigned OggFLAC__stream_encoder_get_channels(const OggFLAC__StreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	return FLAC__stream_encoder_get_channels(encoder->private_->FLAC_stream_encoder);
}

OggFLAC_API unsigned OggFLAC__stream_encoder_get_bits_per_sample(const OggFLAC__StreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	return FLAC__stream_encoder_get_bits_per_sample(encoder->private_->FLAC_stream_encoder);
}

OggFLAC_API unsigned OggFLAC__stream_encoder_get_sample_rate(const OggFLAC__StreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	return FLAC__stream_encoder_get_sample_rate(encoder->private_->FLAC_stream_encoder);
}

OggFLAC_API unsigned OggFLAC__stream_encoder_get_blocksize(const OggFLAC__StreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	return FLAC__stream_encoder_get_blocksize(encoder->private_->FLAC_stream_encoder);
}

OggFLAC_API unsigned OggFLAC__stream_encoder_get_max_lpc_order(const OggFLAC__StreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	return FLAC__stream_encoder_get_max_lpc_order(encoder->private_->FLAC_stream_encoder);
}

OggFLAC_API unsigned OggFLAC__stream_encoder_get_qlp_coeff_precision(const OggFLAC__StreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	return FLAC__stream_encoder_get_qlp_coeff_precision(encoder->private_->FLAC_stream_encoder);
}

OggFLAC_API FLAC__bool OggFLAC__stream_encoder_get_do_qlp_coeff_prec_search(const OggFLAC__StreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	return FLAC__stream_encoder_get_do_qlp_coeff_prec_search(encoder->private_->FLAC_stream_encoder);
}

OggFLAC_API FLAC__bool OggFLAC__stream_encoder_get_do_escape_coding(const OggFLAC__StreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	return FLAC__stream_encoder_get_do_escape_coding(encoder->private_->FLAC_stream_encoder);
}

OggFLAC_API FLAC__bool OggFLAC__stream_encoder_get_do_exhaustive_model_search(const OggFLAC__StreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	return FLAC__stream_encoder_get_do_exhaustive_model_search(encoder->private_->FLAC_stream_encoder);
}

OggFLAC_API unsigned OggFLAC__stream_encoder_get_min_residual_partition_order(const OggFLAC__StreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	return FLAC__stream_encoder_get_min_residual_partition_order(encoder->private_->FLAC_stream_encoder);
}

OggFLAC_API unsigned OggFLAC__stream_encoder_get_max_residual_partition_order(const OggFLAC__StreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	return FLAC__stream_encoder_get_max_residual_partition_order(encoder->private_->FLAC_stream_encoder);
}

OggFLAC_API unsigned OggFLAC__stream_encoder_get_rice_parameter_search_dist(const OggFLAC__StreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	return FLAC__stream_encoder_get_rice_parameter_search_dist(encoder->private_->FLAC_stream_encoder);
}

OggFLAC_API FLAC__uint64 OggFLAC__stream_encoder_get_total_samples_estimate(const OggFLAC__StreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	return FLAC__stream_encoder_get_total_samples_estimate(encoder->private_->FLAC_stream_encoder);
}

OggFLAC_API FLAC__bool OggFLAC__stream_encoder_process(OggFLAC__StreamEncoder *encoder, const FLAC__int32 * const buffer[], unsigned samples)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	return FLAC__stream_encoder_process(encoder->private_->FLAC_stream_encoder, buffer, samples);
}

OggFLAC_API FLAC__bool OggFLAC__stream_encoder_process_interleaved(OggFLAC__StreamEncoder *encoder, const FLAC__int32 buffer[], unsigned samples)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	return FLAC__stream_encoder_process_interleaved(encoder->private_->FLAC_stream_encoder, buffer, samples);
}

/***********************************************************************
 *
 * Private class methods
 *
 ***********************************************************************/

void set_defaults_(OggFLAC__StreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);

	encoder->private_->write_callback = 0;
	encoder->private_->metadata_callback = 0;
	encoder->private_->client_data = 0;
	OggFLAC__ogg_encoder_aspect_set_defaults(&encoder->protected_->ogg_encoder_aspect);
}

FLAC__StreamEncoderWriteStatus write_callback_(const FLAC__StreamEncoder *unused, const FLAC__byte buffer[], unsigned bytes, unsigned samples, unsigned current_frame, void *client_data)
{
	OggFLAC__StreamEncoder *encoder = (OggFLAC__StreamEncoder*)client_data;
	const FLAC__uint64 total_samples_estimate = FLAC__stream_encoder_get_total_samples_estimate(encoder->private_->FLAC_stream_encoder);

	(void)unused;
	FLAC__ASSERT(encoder->private_->FLAC_stream_encoder == unused);

	return OggFLAC__ogg_encoder_aspect_write_callback_wrapper(&encoder->protected_->ogg_encoder_aspect, total_samples_estimate, buffer, bytes, samples, current_frame, (OggFLAC__OggEncoderAspectWriteCallbackProxy)encoder->private_->write_callback, encoder, encoder->private_->client_data);
}

void metadata_callback_(const FLAC__StreamEncoder *unused, const FLAC__StreamMetadata *metadata, void *client_data)
{
	OggFLAC__StreamEncoder *encoder = (OggFLAC__StreamEncoder*)client_data;

	(void)unused;
	FLAC__ASSERT(encoder->private_->FLAC_stream_encoder == unused);

	encoder->private_->metadata_callback(encoder, metadata, encoder->private_->client_data);
}
