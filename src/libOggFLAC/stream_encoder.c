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

#include <stdio.h>
#include <stdlib.h> /* for malloc() */
#include <string.h> /* for memcpy() */
#include "ogg/ogg.h"
#include "FLAC/assert.h"
#include "OggFLAC/stream_decoder.h"
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
	void *client_data;
	FLAC__StreamEncoder *FLAC_stream_encoder;
	/* internal vars (all the above are class settings) */
	FLAC__bool is_first_packet;
	FLAC__uint64 samples_written;
	struct {
		ogg_stream_state stream_state;
		ogg_page page;
	} ogg;
} OggFLAC__StreamEncoderPrivate;


/***********************************************************************
 *
 * Public static class data
 *
 ***********************************************************************/

const char * const OggFLAC__StreamEncoderStateString[] = {
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
OggFLAC__StreamEncoder *OggFLAC__stream_encoder_new()
{
	OggFLAC__StreamEncoder *encoder;

	encoder = (OggFLAC__StreamEncoder*)malloc(sizeof(OggFLAC__StreamEncoder));
	if(encoder == 0) {
		return 0;
	}
	memset(encoder, 0, sizeof(OggFLAC__StreamEncoder));

	encoder->protected_ = (OggFLAC__StreamEncoderProtected*)malloc(sizeof(OggFLAC__StreamEncoderProtected));
	if(encoder->protected_ == 0) {
		free(encoder);
		return 0;
	}
	memset(encoder->protected_, 0, sizeof(OggFLAC__StreamEncoderProtected));

	encoder->private_ = (OggFLAC__StreamEncoderPrivate*)malloc(sizeof(OggFLAC__StreamEncoderPrivate));
	if(encoder->private_ == 0) {
		free(encoder->protected_);
		free(encoder);
		return 0;
	}
	memset(encoder->private_, 0, sizeof(OggFLAC__StreamEncoderPrivate));

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

void OggFLAC__stream_encoder_delete(OggFLAC__StreamEncoder *encoder)
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

OggFLAC__StreamEncoderState OggFLAC__stream_encoder_init(OggFLAC__StreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);

	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return encoder->protected_->state = OggFLAC__STREAM_ENCODER_ALREADY_INITIALIZED;

	if(0 == encoder->private_->write_callback)
		return encoder->protected_->state = OggFLAC__STREAM_ENCODER_INVALID_CALLBACK;

	if(ogg_stream_init(&encoder->private_->ogg.stream_state, encoder->protected_->serial_number) != 0)
		return encoder->protected_->state = OggFLAC__STREAM_ENCODER_OGG_ERROR;

	FLAC__stream_encoder_set_write_callback(encoder->private_->FLAC_stream_encoder, write_callback_);
	FLAC__stream_encoder_set_metadata_callback(encoder->private_->FLAC_stream_encoder, metadata_callback_);
	FLAC__stream_encoder_set_client_data(encoder->private_->FLAC_stream_encoder, encoder);

	if(FLAC__stream_encoder_init(encoder->private_->FLAC_stream_encoder) != FLAC__STREAM_ENCODER_OK)
		return encoder->protected_->state = OggFLAC__STREAM_ENCODER_FLAC_STREAM_ENCODER_ERROR;

	encoder->private_->is_first_packet = true;
	encoder->private_->samples_written = 0;

	return encoder->protected_->state = OggFLAC__STREAM_ENCODER_OK;
}

void OggFLAC__stream_encoder_finish(OggFLAC__StreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);

	if(encoder->protected_->state == OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return;

	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);

	FLAC__stream_encoder_finish(encoder->private_->FLAC_stream_encoder);

	(void)ogg_stream_clear(&encoder->private_->ogg.stream_state);

	set_defaults_(encoder);

	encoder->protected_->state = OggFLAC__STREAM_ENCODER_UNINITIALIZED;
}

FLAC__bool OggFLAC__stream_encoder_set_serial_number(OggFLAC__StreamEncoder *encoder, long value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return false;
	encoder->protected_->serial_number = value;
	return true;
}

FLAC__bool OggFLAC__stream_encoder_set_verify(OggFLAC__StreamEncoder *encoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_verify(encoder->private_->FLAC_stream_encoder, value);
}

FLAC__bool OggFLAC__stream_encoder_set_streamable_subset(OggFLAC__StreamEncoder *encoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_streamable_subset(encoder->private_->FLAC_stream_encoder, value);
}

FLAC__bool OggFLAC__stream_encoder_set_do_mid_side_stereo(OggFLAC__StreamEncoder *encoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_do_mid_side_stereo(encoder->private_->FLAC_stream_encoder, value);
}

FLAC__bool OggFLAC__stream_encoder_set_loose_mid_side_stereo(OggFLAC__StreamEncoder *encoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_loose_mid_side_stereo(encoder->private_->FLAC_stream_encoder, value);
}

FLAC__bool OggFLAC__stream_encoder_set_channels(OggFLAC__StreamEncoder *encoder, unsigned value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_channels(encoder->private_->FLAC_stream_encoder, value);
}

FLAC__bool OggFLAC__stream_encoder_set_bits_per_sample(OggFLAC__StreamEncoder *encoder, unsigned value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_bits_per_sample(encoder->private_->FLAC_stream_encoder, value);
}

FLAC__bool OggFLAC__stream_encoder_set_sample_rate(OggFLAC__StreamEncoder *encoder, unsigned value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_sample_rate(encoder->private_->FLAC_stream_encoder, value);
}

FLAC__bool OggFLAC__stream_encoder_set_blocksize(OggFLAC__StreamEncoder *encoder, unsigned value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_blocksize(encoder->private_->FLAC_stream_encoder, value);
}

FLAC__bool OggFLAC__stream_encoder_set_max_lpc_order(OggFLAC__StreamEncoder *encoder, unsigned value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_max_lpc_order(encoder->private_->FLAC_stream_encoder, value);
}

FLAC__bool OggFLAC__stream_encoder_set_qlp_coeff_precision(OggFLAC__StreamEncoder *encoder, unsigned value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_qlp_coeff_precision(encoder->private_->FLAC_stream_encoder, value);
}

FLAC__bool OggFLAC__stream_encoder_set_do_qlp_coeff_prec_search(OggFLAC__StreamEncoder *encoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_do_qlp_coeff_prec_search(encoder->private_->FLAC_stream_encoder, value);
}

FLAC__bool OggFLAC__stream_encoder_set_do_escape_coding(OggFLAC__StreamEncoder *encoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_do_escape_coding(encoder->private_->FLAC_stream_encoder, value);
}

FLAC__bool OggFLAC__stream_encoder_set_do_exhaustive_model_search(OggFLAC__StreamEncoder *encoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_do_exhaustive_model_search(encoder->private_->FLAC_stream_encoder, value);
}

FLAC__bool OggFLAC__stream_encoder_set_min_residual_partition_order(OggFLAC__StreamEncoder *encoder, unsigned value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_min_residual_partition_order(encoder->private_->FLAC_stream_encoder, value);
}

FLAC__bool OggFLAC__stream_encoder_set_max_residual_partition_order(OggFLAC__StreamEncoder *encoder, unsigned value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_max_residual_partition_order(encoder->private_->FLAC_stream_encoder, value);
}

FLAC__bool OggFLAC__stream_encoder_set_rice_parameter_search_dist(OggFLAC__StreamEncoder *encoder, unsigned value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_rice_parameter_search_dist(encoder->private_->FLAC_stream_encoder, value);
}

FLAC__bool OggFLAC__stream_encoder_set_total_samples_estimate(OggFLAC__StreamEncoder *encoder, FLAC__uint64 value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_total_samples_estimate(encoder->private_->FLAC_stream_encoder, value);
}

FLAC__bool OggFLAC__stream_encoder_set_metadata(OggFLAC__StreamEncoder *encoder, FLAC__StreamMetadata **metadata, unsigned num_blocks)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_metadata(encoder->private_->FLAC_stream_encoder, metadata, num_blocks);
}

FLAC__bool OggFLAC__stream_encoder_set_write_callback(OggFLAC__StreamEncoder *encoder, OggFLAC__StreamEncoderWriteCallback value)
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

FLAC__bool OggFLAC__stream_encoder_set_client_data(OggFLAC__StreamEncoder *encoder, void *value)
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
FLAC__bool OggFLAC__stream_encoder_disable_constant_subframes(OggFLAC__StreamEncoder *encoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_disable_constant_subframes(encoder->private_->FLAC_stream_encoder, value);
}

FLAC__bool OggFLAC__stream_encoder_disable_fixed_subframes(OggFLAC__StreamEncoder *encoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_disable_fixed_subframes(encoder->private_->FLAC_stream_encoder, value);
}

FLAC__bool OggFLAC__stream_encoder_disable_verbatim_subframes(OggFLAC__StreamEncoder *encoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	if(encoder->protected_->state != OggFLAC__STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_disable_verbatim_subframes(encoder->private_->FLAC_stream_encoder, value);
}

OggFLAC__StreamEncoderState OggFLAC__stream_encoder_get_state(const OggFLAC__StreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	return encoder->protected_->state;
}

FLAC__StreamEncoderState OggFLAC__stream_encoder_get_FLAC_stream_encoder_state(const OggFLAC__StreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	return FLAC__stream_encoder_get_state(encoder->private_->FLAC_stream_encoder);
}

FLAC__StreamDecoderState OggFLAC__stream_encoder_get_verify_decoder_state(const OggFLAC__StreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	return FLAC__stream_encoder_get_verify_decoder_state(encoder->private_->FLAC_stream_encoder);
}

void OggFLAC__stream_encoder_get_verify_decoder_error_stats(const OggFLAC__StreamEncoder *encoder, FLAC__uint64 *absolute_sample, unsigned *frame_number, unsigned *channel, unsigned *sample, FLAC__int32 *expected, FLAC__int32 *got)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__stream_encoder_get_verify_decoder_error_stats(encoder->private_->FLAC_stream_encoder, absolute_sample, frame_number, channel, sample, expected, got);
}

FLAC__bool OggFLAC__stream_encoder_get_verify(const OggFLAC__StreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	return FLAC__stream_encoder_get_verify(encoder->private_->FLAC_stream_encoder);
}

FLAC__bool OggFLAC__stream_encoder_get_streamable_subset(const OggFLAC__StreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	return FLAC__stream_encoder_get_streamable_subset(encoder->private_->FLAC_stream_encoder);
}

FLAC__bool OggFLAC__stream_encoder_get_do_mid_side_stereo(const OggFLAC__StreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	return FLAC__stream_encoder_get_do_mid_side_stereo(encoder->private_->FLAC_stream_encoder);
}

FLAC__bool OggFLAC__stream_encoder_get_loose_mid_side_stereo(const OggFLAC__StreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	return FLAC__stream_encoder_get_loose_mid_side_stereo(encoder->private_->FLAC_stream_encoder);
}

unsigned OggFLAC__stream_encoder_get_channels(const OggFLAC__StreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	return FLAC__stream_encoder_get_channels(encoder->private_->FLAC_stream_encoder);
}

unsigned OggFLAC__stream_encoder_get_bits_per_sample(const OggFLAC__StreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	return FLAC__stream_encoder_get_bits_per_sample(encoder->private_->FLAC_stream_encoder);
}

unsigned OggFLAC__stream_encoder_get_sample_rate(const OggFLAC__StreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	return FLAC__stream_encoder_get_sample_rate(encoder->private_->FLAC_stream_encoder);
}

unsigned OggFLAC__stream_encoder_get_blocksize(const OggFLAC__StreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	return FLAC__stream_encoder_get_blocksize(encoder->private_->FLAC_stream_encoder);
}

unsigned OggFLAC__stream_encoder_get_max_lpc_order(const OggFLAC__StreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	return FLAC__stream_encoder_get_max_lpc_order(encoder->private_->FLAC_stream_encoder);
}

unsigned OggFLAC__stream_encoder_get_qlp_coeff_precision(const OggFLAC__StreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	return FLAC__stream_encoder_get_qlp_coeff_precision(encoder->private_->FLAC_stream_encoder);
}

FLAC__bool OggFLAC__stream_encoder_get_do_qlp_coeff_prec_search(const OggFLAC__StreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	return FLAC__stream_encoder_get_do_qlp_coeff_prec_search(encoder->private_->FLAC_stream_encoder);
}

FLAC__bool OggFLAC__stream_encoder_get_do_escape_coding(const OggFLAC__StreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	return FLAC__stream_encoder_get_do_escape_coding(encoder->private_->FLAC_stream_encoder);
}

FLAC__bool OggFLAC__stream_encoder_get_do_exhaustive_model_search(const OggFLAC__StreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	return FLAC__stream_encoder_get_do_exhaustive_model_search(encoder->private_->FLAC_stream_encoder);
}

unsigned OggFLAC__stream_encoder_get_min_residual_partition_order(const OggFLAC__StreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	return FLAC__stream_encoder_get_min_residual_partition_order(encoder->private_->FLAC_stream_encoder);
}

unsigned OggFLAC__stream_encoder_get_max_residual_partition_order(const OggFLAC__StreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	return FLAC__stream_encoder_get_max_residual_partition_order(encoder->private_->FLAC_stream_encoder);
}

unsigned OggFLAC__stream_encoder_get_rice_parameter_search_dist(const OggFLAC__StreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	return FLAC__stream_encoder_get_rice_parameter_search_dist(encoder->private_->FLAC_stream_encoder);
}

FLAC__uint64 OggFLAC__stream_encoder_get_total_samples_estimate(const OggFLAC__StreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	return FLAC__stream_encoder_get_total_samples_estimate(encoder->private_->FLAC_stream_encoder);
}

FLAC__bool OggFLAC__stream_encoder_process(OggFLAC__StreamEncoder *encoder, const FLAC__int32 * const buffer[], unsigned samples)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	return FLAC__stream_encoder_process(encoder->private_->FLAC_stream_encoder, buffer, samples);
}

FLAC__bool OggFLAC__stream_encoder_process_interleaved(OggFLAC__StreamEncoder *encoder, const FLAC__int32 buffer[], unsigned samples)
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
	encoder->private_->client_data = 0;
	encoder->protected_->serial_number = 0;
}

FLAC__StreamEncoderWriteStatus write_callback_(const FLAC__StreamEncoder *unused, const FLAC__byte buffer[], unsigned bytes, unsigned samples, unsigned current_frame, void *client_data)
{
	OggFLAC__StreamEncoder *encoder = (OggFLAC__StreamEncoder*)client_data;
	ogg_packet packet;
	const FLAC__uint64 total_samples_estimate = FLAC__stream_encoder_get_total_samples_estimate(encoder->private_->FLAC_stream_encoder);
	FLAC__ASSERT(encoder->private_->FLAC_stream_encoder == unused);

	encoder->private_->samples_written += samples;

	memset(&packet, 0, sizeof(packet));
	packet.packet = (unsigned char *)buffer;
	packet.granulepos = encoder->private_->samples_written;
	/* WATCHOUT:
	 * This depends on the behavior of FLAC__StreamEncoder that 'samples'
	 * will be 0 for metadata writes.
	 */
	packet.packetno = (samples == 0? -1 : (int)current_frame);
	packet.bytes = bytes;

	if(encoder->private_->is_first_packet) {
		packet.b_o_s = 1;
		encoder->private_->is_first_packet = false;
	}

	if(total_samples_estimate > 0 && total_samples_estimate == encoder->private_->samples_written)
		packet.e_o_s = 1;

	if(ogg_stream_packetin(&encoder->private_->ogg.stream_state, &packet) != 0)
		return FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR;

	while(ogg_stream_pageout(&encoder->private_->ogg.stream_state, &encoder->private_->ogg.page) != 0) {
		if(encoder->private_->write_callback(encoder, encoder->private_->ogg.page.header, encoder->private_->ogg.page.header_len, 0, current_frame, encoder->private_->client_data) != FLAC__STREAM_ENCODER_WRITE_STATUS_OK)
			return FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR;

		if(encoder->private_->write_callback(encoder, encoder->private_->ogg.page.body, encoder->private_->ogg.page.body_len, 0, current_frame, encoder->private_->client_data) != FLAC__STREAM_ENCODER_WRITE_STATUS_OK)
			return FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR;
	}

	return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
}

void metadata_callback_(const FLAC__StreamEncoder *encoder, const FLAC__StreamMetadata *metadata, void *client_data)
{
	/*
	 * We don't try to go back an update metadata blocks by mucking
	 * around inside the Ogg layer.  Maybe someday we will care to
	 * and an OggFLAC__SeekableStreamEncoder and OggFLAC__FileEncoder
	 * will be possible but it may just never be useful.
	 */
	(void)encoder, (void)metadata, (void)client_data;
}
