/* libFLAC - Free Lossless Audio Codec library
 * Copyright (C) 2002,2003,2004,2005,2006  Josh Coalson
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
#include <string.h> /* for memcpy() */
#include "FLAC/assert.h"
#include "protected/seekable_stream_encoder.h"

#ifdef max
#undef max
#endif
#define max(a,b) ((a)>(b)?(a):(b))

/***********************************************************************
 *
 * Private class method prototypes
 *
 ***********************************************************************/

/* unpublished debug routines */
extern FLAC__bool FLAC__stream_encoder_disable_constant_subframes(FLAC__StreamEncoder *encoder, FLAC__bool value);
extern FLAC__bool FLAC__stream_encoder_disable_fixed_subframes(FLAC__StreamEncoder *encoder, FLAC__bool value);
extern FLAC__bool FLAC__stream_encoder_disable_verbatim_subframes(FLAC__StreamEncoder *encoder, FLAC__bool value);

static void set_defaults_(FLAC__SeekableStreamEncoder *encoder);
static FLAC__StreamEncoderWriteStatus write_callback_(const FLAC__StreamEncoder *encoder, const FLAC__byte buffer[], unsigned bytes, unsigned samples, unsigned current_frame, void *client_data);
static void metadata_callback_(const FLAC__StreamEncoder *encoder, const FLAC__StreamMetadata *metadata, void *client_data);

/***********************************************************************
 *
 * Private class data
 *
 ***********************************************************************/

typedef struct FLAC__SeekableStreamEncoderPrivate {
	FLAC__SeekableStreamEncoderSeekCallback seek_callback;
	FLAC__SeekableStreamEncoderTellCallback tell_callback;
	FLAC__SeekableStreamEncoderWriteCallback write_callback;
	void *client_data;
	FLAC__StreamEncoder *stream_encoder;
	FLAC__StreamMetadata_SeekTable *seek_table;
	/* internal vars (all the above are class settings) */
	unsigned first_seekpoint_to_check;
	FLAC__uint64 samples_written;
} FLAC__SeekableStreamEncoderPrivate;

/***********************************************************************
 *
 * Public static class data
 *
 ***********************************************************************/

FLAC_API const char * const FLAC__SeekableStreamEncoderStateString[] = {
	"FLAC__SEEKABLE_STREAM_ENCODER_OK",
	"FLAC__SEEKABLE_STREAM_ENCODER_STREAM_ENCODER_ERROR",
	"FLAC__SEEKABLE_STREAM_ENCODER_MEMORY_ALLOCATION_ERROR",
	"FLAC__SEEKABLE_STREAM_ENCODER_WRITE_ERROR",
	"FLAC__SEEKABLE_STREAM_ENCODER_READ_ERROR",
	"FLAC__SEEKABLE_STREAM_ENCODER_SEEK_ERROR",
	"FLAC__SEEKABLE_STREAM_ENCODER_TELL_ERROR",
	"FLAC__SEEKABLE_STREAM_ENCODER_ALREADY_INITIALIZED",
	"FLAC__SEEKABLE_STREAM_ENCODER_INVALID_CALLBACK",
	"FLAC__SEEKABLE_STREAM_ENCODER_INVALID_SEEKTABLE",
	"FLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED"
};

FLAC_API const char * const FLAC__SeekableStreamEncoderSeekStatusString[] = {
	"FLAC__SEEKABLE_STREAM_ENCODER_SEEK_STATUS_OK",
	"FLAC__SEEKABLE_STREAM_ENCODER_SEEK_STATUS_ERROR"
};

FLAC_API const char * const FLAC__SeekableStreamEncoderTellStatusString[] = {
	"FLAC__SEEKABLE_STREAM_ENCODER_TELL_STATUS_OK",
	"FLAC__SEEKABLE_STREAM_ENCODER_TELL_STATUS_ERROR"
};


/***********************************************************************
 *
 * Class constructor/destructor
 *
 ***********************************************************************/

FLAC_API FLAC__SeekableStreamEncoder *FLAC__seekable_stream_encoder_new()
{
	FLAC__SeekableStreamEncoder *encoder;

	FLAC__ASSERT(sizeof(int) >= 4); /* we want to die right away if this is not true */

	encoder = (FLAC__SeekableStreamEncoder*)calloc(1, sizeof(FLAC__SeekableStreamEncoder));
	if(encoder == 0) {
		return 0;
	}

	encoder->protected_ = (FLAC__SeekableStreamEncoderProtected*)calloc(1, sizeof(FLAC__SeekableStreamEncoderProtected));
	if(encoder->protected_ == 0) {
		free(encoder);
		return 0;
	}

	encoder->private_ = (FLAC__SeekableStreamEncoderPrivate*)calloc(1, sizeof(FLAC__SeekableStreamEncoderPrivate));
	if(encoder->private_ == 0) {
		free(encoder->protected_);
		free(encoder);
		return 0;
	}

	encoder->private_->stream_encoder = FLAC__stream_encoder_new();
	if(0 == encoder->private_->stream_encoder) {
		free(encoder->private_);
		free(encoder->protected_);
		free(encoder);
		return 0;
	}

	set_defaults_(encoder);

	encoder->protected_->state = FLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED;

	return encoder;
}

FLAC_API void FLAC__seekable_stream_encoder_delete(FLAC__SeekableStreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->private_->stream_encoder);

	(void)FLAC__seekable_stream_encoder_finish(encoder);

	FLAC__stream_encoder_delete(encoder->private_->stream_encoder);

	free(encoder->private_);
	free(encoder->protected_);
	free(encoder);
}


/***********************************************************************
 *
 * Public class methods
 *
 ***********************************************************************/

FLAC_API FLAC__SeekableStreamEncoderState FLAC__seekable_stream_encoder_init(FLAC__SeekableStreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);

	if(encoder->protected_->state != FLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return encoder->protected_->state = FLAC__SEEKABLE_STREAM_ENCODER_ALREADY_INITIALIZED;

	if(0 == encoder->private_->seek_callback || 0 == encoder->private_->tell_callback || 0 == encoder->private_->write_callback)
		return encoder->protected_->state = FLAC__SEEKABLE_STREAM_ENCODER_INVALID_CALLBACK;

	if(0 != encoder->private_->seek_table && !FLAC__format_seektable_is_legal(encoder->private_->seek_table))
		return encoder->protected_->state = FLAC__SEEKABLE_STREAM_ENCODER_INVALID_SEEKTABLE;

	/*
	 * These must be done before we init the stream encoder because that
	 * calls the write_callback, which uses these values.
	 */
	encoder->private_->first_seekpoint_to_check = 0;
	encoder->private_->samples_written = 0;
	encoder->protected_->streaminfo_offset = 0;
	encoder->protected_->seektable_offset = 0;
	encoder->protected_->audio_offset = 0;

	FLAC__stream_encoder_set_write_callback(encoder->private_->stream_encoder, write_callback_);
	FLAC__stream_encoder_set_metadata_callback(encoder->private_->stream_encoder, metadata_callback_);
	FLAC__stream_encoder_set_client_data(encoder->private_->stream_encoder, encoder);

	if(FLAC__stream_encoder_init(encoder->private_->stream_encoder) != FLAC__STREAM_ENCODER_OK)
		return encoder->protected_->state = FLAC__SEEKABLE_STREAM_ENCODER_STREAM_ENCODER_ERROR;

	/*
	 * Initializing the stream encoder writes all the metadata, so we
	 * save the stream offset now.
	 */
	if(encoder->private_->tell_callback(encoder, &encoder->protected_->audio_offset, encoder->private_->client_data) != FLAC__SEEKABLE_STREAM_ENCODER_TELL_STATUS_OK)
		return encoder->protected_->state = FLAC__SEEKABLE_STREAM_ENCODER_TELL_ERROR;

	return encoder->protected_->state = FLAC__SEEKABLE_STREAM_ENCODER_OK;
}

FLAC_API void FLAC__seekable_stream_encoder_finish(FLAC__SeekableStreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);

	if(encoder->protected_->state == FLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return;

	FLAC__ASSERT(0 != encoder->private_->stream_encoder);

	FLAC__stream_encoder_finish(encoder->private_->stream_encoder);

	set_defaults_(encoder);

	encoder->protected_->state = FLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED;
}

FLAC_API FLAC__bool FLAC__seekable_stream_encoder_set_verify(FLAC__SeekableStreamEncoder *encoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->stream_encoder);
	if(encoder->protected_->state != FLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_verify(encoder->private_->stream_encoder, value);
}

FLAC_API FLAC__bool FLAC__seekable_stream_encoder_set_streamable_subset(FLAC__SeekableStreamEncoder *encoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->stream_encoder);
	if(encoder->protected_->state != FLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_streamable_subset(encoder->private_->stream_encoder, value);
}

FLAC_API FLAC__bool FLAC__seekable_stream_encoder_set_do_mid_side_stereo(FLAC__SeekableStreamEncoder *encoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->stream_encoder);
	if(encoder->protected_->state != FLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_do_mid_side_stereo(encoder->private_->stream_encoder, value);
}

FLAC_API FLAC__bool FLAC__seekable_stream_encoder_set_loose_mid_side_stereo(FLAC__SeekableStreamEncoder *encoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->stream_encoder);
	if(encoder->protected_->state != FLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_loose_mid_side_stereo(encoder->private_->stream_encoder, value);
}

FLAC_API FLAC__bool FLAC__seekable_stream_encoder_set_channels(FLAC__SeekableStreamEncoder *encoder, unsigned value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->stream_encoder);
	if(encoder->protected_->state != FLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_channels(encoder->private_->stream_encoder, value);
}

FLAC_API FLAC__bool FLAC__seekable_stream_encoder_set_bits_per_sample(FLAC__SeekableStreamEncoder *encoder, unsigned value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->stream_encoder);
	if(encoder->protected_->state != FLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_bits_per_sample(encoder->private_->stream_encoder, value);
}

FLAC_API FLAC__bool FLAC__seekable_stream_encoder_set_sample_rate(FLAC__SeekableStreamEncoder *encoder, unsigned value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->stream_encoder);
	if(encoder->protected_->state != FLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_sample_rate(encoder->private_->stream_encoder, value);
}

FLAC_API FLAC__bool FLAC__seekable_stream_encoder_set_blocksize(FLAC__SeekableStreamEncoder *encoder, unsigned value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->stream_encoder);
	if(encoder->protected_->state != FLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_blocksize(encoder->private_->stream_encoder, value);
}

FLAC_API FLAC__bool FLAC__seekable_stream_encoder_set_apodization(FLAC__SeekableStreamEncoder *encoder, const char *specification)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->stream_encoder);
	if(encoder->protected_->state != FLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_apodization(encoder->private_->stream_encoder, specification);
}

FLAC_API FLAC__bool FLAC__seekable_stream_encoder_set_max_lpc_order(FLAC__SeekableStreamEncoder *encoder, unsigned value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->stream_encoder);
	if(encoder->protected_->state != FLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_max_lpc_order(encoder->private_->stream_encoder, value);
}

FLAC_API FLAC__bool FLAC__seekable_stream_encoder_set_qlp_coeff_precision(FLAC__SeekableStreamEncoder *encoder, unsigned value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->stream_encoder);
	if(encoder->protected_->state != FLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_qlp_coeff_precision(encoder->private_->stream_encoder, value);
}

FLAC_API FLAC__bool FLAC__seekable_stream_encoder_set_do_qlp_coeff_prec_search(FLAC__SeekableStreamEncoder *encoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->stream_encoder);
	if(encoder->protected_->state != FLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_do_qlp_coeff_prec_search(encoder->private_->stream_encoder, value);
}

FLAC_API FLAC__bool FLAC__seekable_stream_encoder_set_do_escape_coding(FLAC__SeekableStreamEncoder *encoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->stream_encoder);
	if(encoder->protected_->state != FLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_do_escape_coding(encoder->private_->stream_encoder, value);
}

FLAC_API FLAC__bool FLAC__seekable_stream_encoder_set_do_exhaustive_model_search(FLAC__SeekableStreamEncoder *encoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->stream_encoder);
	if(encoder->protected_->state != FLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_do_exhaustive_model_search(encoder->private_->stream_encoder, value);
}

FLAC_API FLAC__bool FLAC__seekable_stream_encoder_set_min_residual_partition_order(FLAC__SeekableStreamEncoder *encoder, unsigned value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->stream_encoder);
	if(encoder->protected_->state != FLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_min_residual_partition_order(encoder->private_->stream_encoder, value);
}

FLAC_API FLAC__bool FLAC__seekable_stream_encoder_set_max_residual_partition_order(FLAC__SeekableStreamEncoder *encoder, unsigned value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->stream_encoder);
	if(encoder->protected_->state != FLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_max_residual_partition_order(encoder->private_->stream_encoder, value);
}

FLAC_API FLAC__bool FLAC__seekable_stream_encoder_set_rice_parameter_search_dist(FLAC__SeekableStreamEncoder *encoder, unsigned value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->stream_encoder);
	if(encoder->protected_->state != FLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_rice_parameter_search_dist(encoder->private_->stream_encoder, value);
}

FLAC_API FLAC__bool FLAC__seekable_stream_encoder_set_total_samples_estimate(FLAC__SeekableStreamEncoder *encoder, FLAC__uint64 value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->stream_encoder);
	if(encoder->protected_->state != FLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_total_samples_estimate(encoder->private_->stream_encoder, value);
}

FLAC_API FLAC__bool FLAC__seekable_stream_encoder_set_metadata(FLAC__SeekableStreamEncoder *encoder, FLAC__StreamMetadata **metadata, unsigned num_blocks)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->stream_encoder);
	if(encoder->protected_->state != FLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return false;
	if(0 != metadata && num_blocks > 0) {
		unsigned i;
		for(i = 0; i < num_blocks; i++) {
			if(0 != metadata[i] && metadata[i]->type == FLAC__METADATA_TYPE_SEEKTABLE) {
				encoder->private_->seek_table = &metadata[i]->data.seek_table;
				break; /* take only the first one */
			}
		}
	}
	return FLAC__stream_encoder_set_metadata(encoder->private_->stream_encoder, metadata, num_blocks);
}

FLAC_API FLAC__bool FLAC__seekable_stream_encoder_set_seek_callback(FLAC__SeekableStreamEncoder *encoder, FLAC__SeekableStreamEncoderSeekCallback value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	if(encoder->protected_->state != FLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return false;
	encoder->private_->seek_callback = value;
	return true;
}

FLAC_API FLAC__bool FLAC__seekable_stream_encoder_set_tell_callback(FLAC__SeekableStreamEncoder *encoder, FLAC__SeekableStreamEncoderTellCallback value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	if(encoder->protected_->state != FLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return false;
	encoder->private_->tell_callback = value;
	return true;
}

FLAC_API FLAC__bool FLAC__seekable_stream_encoder_set_write_callback(FLAC__SeekableStreamEncoder *encoder, FLAC__SeekableStreamEncoderWriteCallback value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	if(encoder->protected_->state != FLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return false;
	encoder->private_->write_callback = value;
	return true;
}

FLAC_API FLAC__bool FLAC__seekable_stream_encoder_set_client_data(FLAC__SeekableStreamEncoder *encoder, void *value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	if(encoder->protected_->state != FLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return false;
	encoder->private_->client_data = value;
	return true;
}

/*
 * These three functions are not static, but not publically exposed in
 * include/FLAC/ either.  They are used by the test suite.
 */
FLAC_API FLAC__bool FLAC__seekable_stream_encoder_disable_constant_subframes(FLAC__SeekableStreamEncoder *encoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	if(encoder->protected_->state != FLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_disable_constant_subframes(encoder->private_->stream_encoder, value);
}

FLAC_API FLAC__bool FLAC__seekable_stream_encoder_disable_fixed_subframes(FLAC__SeekableStreamEncoder *encoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	if(encoder->protected_->state != FLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_disable_fixed_subframes(encoder->private_->stream_encoder, value);
}

FLAC_API FLAC__bool FLAC__seekable_stream_encoder_disable_verbatim_subframes(FLAC__SeekableStreamEncoder *encoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	if(encoder->protected_->state != FLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_disable_verbatim_subframes(encoder->private_->stream_encoder, value);
}

FLAC_API FLAC__SeekableStreamEncoderState FLAC__seekable_stream_encoder_get_state(const FLAC__SeekableStreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->protected_);
	return encoder->protected_->state;
}

FLAC_API FLAC__StreamEncoderState FLAC__seekable_stream_encoder_get_stream_encoder_state(const FLAC__SeekableStreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return FLAC__stream_encoder_get_state(encoder->private_->stream_encoder);
}

FLAC_API FLAC__StreamDecoderState FLAC__seekable_stream_encoder_get_verify_decoder_state(const FLAC__SeekableStreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return FLAC__stream_encoder_get_verify_decoder_state(encoder->private_->stream_encoder);
}

FLAC_API const char *FLAC__seekable_stream_encoder_get_resolved_state_string(const FLAC__SeekableStreamEncoder *encoder)
{
	if(encoder->protected_->state != FLAC__SEEKABLE_STREAM_ENCODER_STREAM_ENCODER_ERROR)
		return FLAC__SeekableStreamEncoderStateString[encoder->protected_->state];
	else
		return FLAC__stream_encoder_get_resolved_state_string(encoder->private_->stream_encoder);
}

FLAC_API void FLAC__seekable_stream_encoder_get_verify_decoder_error_stats(const FLAC__SeekableStreamEncoder *encoder, FLAC__uint64 *absolute_sample, unsigned *frame_number, unsigned *channel, unsigned *sample, FLAC__int32 *expected, FLAC__int32 *got)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__stream_encoder_get_verify_decoder_error_stats(encoder->private_->stream_encoder, absolute_sample, frame_number, channel, sample, expected, got);
}

FLAC_API FLAC__bool FLAC__seekable_stream_encoder_get_verify(const FLAC__SeekableStreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return FLAC__stream_encoder_get_verify(encoder->private_->stream_encoder);
}

FLAC_API FLAC__bool FLAC__seekable_stream_encoder_get_streamable_subset(const FLAC__SeekableStreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return FLAC__stream_encoder_get_streamable_subset(encoder->private_->stream_encoder);
}

FLAC_API FLAC__bool FLAC__seekable_stream_encoder_get_do_mid_side_stereo(const FLAC__SeekableStreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return FLAC__stream_encoder_get_do_mid_side_stereo(encoder->private_->stream_encoder);
}

FLAC_API FLAC__bool FLAC__seekable_stream_encoder_get_loose_mid_side_stereo(const FLAC__SeekableStreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return FLAC__stream_encoder_get_loose_mid_side_stereo(encoder->private_->stream_encoder);
}

FLAC_API unsigned FLAC__seekable_stream_encoder_get_channels(const FLAC__SeekableStreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return FLAC__stream_encoder_get_channels(encoder->private_->stream_encoder);
}

FLAC_API unsigned FLAC__seekable_stream_encoder_get_bits_per_sample(const FLAC__SeekableStreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return FLAC__stream_encoder_get_bits_per_sample(encoder->private_->stream_encoder);
}

FLAC_API unsigned FLAC__seekable_stream_encoder_get_sample_rate(const FLAC__SeekableStreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return FLAC__stream_encoder_get_sample_rate(encoder->private_->stream_encoder);
}

FLAC_API unsigned FLAC__seekable_stream_encoder_get_blocksize(const FLAC__SeekableStreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return FLAC__stream_encoder_get_blocksize(encoder->private_->stream_encoder);
}

FLAC_API unsigned FLAC__seekable_stream_encoder_get_max_lpc_order(const FLAC__SeekableStreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return FLAC__stream_encoder_get_max_lpc_order(encoder->private_->stream_encoder);
}

FLAC_API unsigned FLAC__seekable_stream_encoder_get_qlp_coeff_precision(const FLAC__SeekableStreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return FLAC__stream_encoder_get_qlp_coeff_precision(encoder->private_->stream_encoder);
}

FLAC_API FLAC__bool FLAC__seekable_stream_encoder_get_do_qlp_coeff_prec_search(const FLAC__SeekableStreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return FLAC__stream_encoder_get_do_qlp_coeff_prec_search(encoder->private_->stream_encoder);
}

FLAC_API FLAC__bool FLAC__seekable_stream_encoder_get_do_escape_coding(const FLAC__SeekableStreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return FLAC__stream_encoder_get_do_escape_coding(encoder->private_->stream_encoder);
}

FLAC_API FLAC__bool FLAC__seekable_stream_encoder_get_do_exhaustive_model_search(const FLAC__SeekableStreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return FLAC__stream_encoder_get_do_exhaustive_model_search(encoder->private_->stream_encoder);
}

FLAC_API unsigned FLAC__seekable_stream_encoder_get_min_residual_partition_order(const FLAC__SeekableStreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return FLAC__stream_encoder_get_min_residual_partition_order(encoder->private_->stream_encoder);
}

FLAC_API unsigned FLAC__seekable_stream_encoder_get_max_residual_partition_order(const FLAC__SeekableStreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return FLAC__stream_encoder_get_max_residual_partition_order(encoder->private_->stream_encoder);
}

FLAC_API unsigned FLAC__seekable_stream_encoder_get_rice_parameter_search_dist(const FLAC__SeekableStreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return FLAC__stream_encoder_get_rice_parameter_search_dist(encoder->private_->stream_encoder);
}

FLAC_API FLAC__uint64 FLAC__seekable_stream_encoder_get_total_samples_estimate(const FLAC__SeekableStreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return FLAC__stream_encoder_get_total_samples_estimate(encoder->private_->stream_encoder);
}

FLAC_API FLAC__bool FLAC__seekable_stream_encoder_process(FLAC__SeekableStreamEncoder *encoder, const FLAC__int32 * const buffer[], unsigned samples)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	if(!FLAC__stream_encoder_process(encoder->private_->stream_encoder, buffer, samples)) {
		encoder->protected_->state = FLAC__SEEKABLE_STREAM_ENCODER_STREAM_ENCODER_ERROR;
		return false;
	}
	else
		return true;
}

/* 'samples' is channel-wide samples, e.g. for 1 second at 44100Hz, 'samples' = 44100 regardless of the number of channels */
FLAC_API FLAC__bool FLAC__seekable_stream_encoder_process_interleaved(FLAC__SeekableStreamEncoder *encoder, const FLAC__int32 buffer[], unsigned samples)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	if(!FLAC__stream_encoder_process_interleaved(encoder->private_->stream_encoder, buffer, samples)) {
		encoder->protected_->state = FLAC__SEEKABLE_STREAM_ENCODER_STREAM_ENCODER_ERROR;
		return false;
	}
	else
		return true;
}

/***********************************************************************
 *
 * Private class methods
 *
 ***********************************************************************/

void set_defaults_(FLAC__SeekableStreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);

	encoder->private_->seek_callback = 0;
	encoder->private_->tell_callback = 0;
	encoder->private_->write_callback = 0;
	encoder->private_->client_data = 0;

	encoder->private_->seek_table = 0;
}

FLAC__StreamEncoderWriteStatus write_callback_(const FLAC__StreamEncoder *unused, const FLAC__byte buffer[], unsigned bytes, unsigned samples, unsigned current_frame, void *client_data)
{
	FLAC__SeekableStreamEncoder *encoder = (FLAC__SeekableStreamEncoder*)client_data;
	FLAC__StreamEncoderWriteStatus status;
	FLAC__uint64 output_position;

	(void)unused; /* silence compiler warning about unused parameter */
	FLAC__ASSERT(encoder->private_->stream_encoder == unused);

	if(encoder->private_->tell_callback(encoder, &output_position, encoder->private_->client_data) != FLAC__SEEKABLE_STREAM_ENCODER_TELL_STATUS_OK)
		return encoder->protected_->state = FLAC__SEEKABLE_STREAM_ENCODER_TELL_ERROR;

	/*
	 * Watch for the STREAMINFO block and first SEEKTABLE block to go by and store their offsets.
	 */
	if(samples == 0) {
		FLAC__MetadataType type = (buffer[0] & 0x7f);
		if(type == FLAC__METADATA_TYPE_STREAMINFO)
			encoder->protected_->streaminfo_offset = output_position;
		else if(type == FLAC__METADATA_TYPE_SEEKTABLE && encoder->protected_->seektable_offset == 0)
			encoder->protected_->seektable_offset = output_position;
	}

	/*
	 * Mark the current seek point if hit (if audio_offset == 0 that
	 * means we're still writing metadata and haven't hit the first
	 * frame yet)
	 */
	if(0 != encoder->private_->seek_table && encoder->protected_->audio_offset > 0 && encoder->private_->seek_table->num_points > 0) {
		const unsigned blocksize = FLAC__stream_encoder_get_blocksize(encoder->private_->stream_encoder);
		const FLAC__uint64 frame_first_sample = encoder->private_->samples_written;
		const FLAC__uint64 frame_last_sample = frame_first_sample + (FLAC__uint64)blocksize - 1;
		FLAC__uint64 test_sample;
		unsigned i;
		for(i = encoder->private_->first_seekpoint_to_check; i < encoder->private_->seek_table->num_points; i++) {
			test_sample = encoder->private_->seek_table->points[i].sample_number;
			if(test_sample > frame_last_sample) {
				break;
			}
			else if(test_sample >= frame_first_sample) {
				encoder->private_->seek_table->points[i].sample_number = frame_first_sample;
				encoder->private_->seek_table->points[i].stream_offset = output_position - encoder->protected_->audio_offset;
				encoder->private_->seek_table->points[i].frame_samples = blocksize;
				encoder->private_->first_seekpoint_to_check++;
				/* DO NOT: "break;" and here's why:
				 * The seektable template may contain more than one target
				 * sample for any given frame; we will keep looping, generating
				 * duplicate seekpoints for them, and we'll clean it up later,
				 * just before writing the seektable back to the metadata.
				 */
			}
			else {
				encoder->private_->first_seekpoint_to_check++;
			}
		}
	}

	status = encoder->private_->write_callback(encoder, buffer, bytes, samples, current_frame, encoder->private_->client_data);

	if(status == FLAC__STREAM_ENCODER_WRITE_STATUS_OK) {
		encoder->private_->samples_written += samples;
	}
	else
		encoder->protected_->state = FLAC__SEEKABLE_STREAM_ENCODER_WRITE_ERROR;

	return status;
}

void metadata_callback_(const FLAC__StreamEncoder *unused, const FLAC__StreamMetadata *metadata, void *client_data)
{
	FLAC__SeekableStreamEncoder *encoder = (FLAC__SeekableStreamEncoder*)client_data;
	FLAC__byte b[max(6, FLAC__STREAM_METADATA_SEEKPOINT_LENGTH)];
	const FLAC__uint64 samples = metadata->data.stream_info.total_samples;
	const unsigned min_framesize = metadata->data.stream_info.min_framesize;
	const unsigned max_framesize = metadata->data.stream_info.max_framesize;
	const unsigned bps = metadata->data.stream_info.bits_per_sample;

	FLAC__ASSERT(metadata->type == FLAC__METADATA_TYPE_STREAMINFO);

	/* We get called by the stream encoder when the encoding process
	 * has finished so that we can update the STREAMINFO and SEEKTABLE
	 * blocks.
	 */

	(void)unused; /* silence compiler warning about unused parameter */
	FLAC__ASSERT(encoder->private_->stream_encoder == unused);

	/*@@@ reopen callback here?  The docs currently require user to open files in update mode from the start */

	/* All this is based on intimate knowledge of the stream header
	 * layout, but a change to the header format that would break this
	 * would also break all streams encoded in the previous format.
	 */

	/*
	 * Write MD5 signature
	 */
	{
		const unsigned md5_offset =
		FLAC__STREAM_METADATA_HEADER_LENGTH +
		(
			FLAC__STREAM_METADATA_STREAMINFO_MIN_BLOCK_SIZE_LEN +
			FLAC__STREAM_METADATA_STREAMINFO_MAX_BLOCK_SIZE_LEN +
			FLAC__STREAM_METADATA_STREAMINFO_MIN_FRAME_SIZE_LEN +
			FLAC__STREAM_METADATA_STREAMINFO_MAX_FRAME_SIZE_LEN +
			FLAC__STREAM_METADATA_STREAMINFO_SAMPLE_RATE_LEN +
			FLAC__STREAM_METADATA_STREAMINFO_CHANNELS_LEN +
			FLAC__STREAM_METADATA_STREAMINFO_BITS_PER_SAMPLE_LEN +
			FLAC__STREAM_METADATA_STREAMINFO_TOTAL_SAMPLES_LEN
		) / 8;

		if(encoder->private_->seek_callback(encoder, encoder->protected_->streaminfo_offset + md5_offset, encoder->private_->client_data) != FLAC__SEEKABLE_STREAM_ENCODER_SEEK_STATUS_OK) {
			encoder->protected_->state = FLAC__SEEKABLE_STREAM_ENCODER_SEEK_ERROR;
			return;
		}
		if(encoder->private_->write_callback(encoder, metadata->data.stream_info.md5sum, 16, 0, 0, encoder->private_->client_data) != FLAC__STREAM_ENCODER_WRITE_STATUS_OK) {
			encoder->protected_->state = FLAC__SEEKABLE_STREAM_ENCODER_WRITE_ERROR;
			return;
		}
	}

	/*
	 * Write total samples
	 */
	{
		const unsigned total_samples_byte_offset =
		FLAC__STREAM_METADATA_HEADER_LENGTH +
		(
			FLAC__STREAM_METADATA_STREAMINFO_MIN_BLOCK_SIZE_LEN +
			FLAC__STREAM_METADATA_STREAMINFO_MAX_BLOCK_SIZE_LEN +
			FLAC__STREAM_METADATA_STREAMINFO_MIN_FRAME_SIZE_LEN +
			FLAC__STREAM_METADATA_STREAMINFO_MAX_FRAME_SIZE_LEN +
			FLAC__STREAM_METADATA_STREAMINFO_SAMPLE_RATE_LEN +
			FLAC__STREAM_METADATA_STREAMINFO_CHANNELS_LEN +
			FLAC__STREAM_METADATA_STREAMINFO_BITS_PER_SAMPLE_LEN
			- 4
		) / 8;

		b[0] = ((FLAC__byte)(bps-1) << 4) | (FLAC__byte)((samples >> 32) & 0x0F);
		b[1] = (FLAC__byte)((samples >> 24) & 0xFF);
		b[2] = (FLAC__byte)((samples >> 16) & 0xFF);
		b[3] = (FLAC__byte)((samples >> 8) & 0xFF);
		b[4] = (FLAC__byte)(samples & 0xFF);
		if(encoder->private_->seek_callback(encoder, encoder->protected_->streaminfo_offset + total_samples_byte_offset, encoder->private_->client_data) != FLAC__SEEKABLE_STREAM_ENCODER_SEEK_STATUS_OK) {
			encoder->protected_->state = FLAC__SEEKABLE_STREAM_ENCODER_SEEK_ERROR;
			return;
		}
		if(encoder->private_->write_callback(encoder, b, 5, 0, 0, encoder->private_->client_data) != FLAC__STREAM_ENCODER_WRITE_STATUS_OK) {
			encoder->protected_->state = FLAC__SEEKABLE_STREAM_ENCODER_WRITE_ERROR;
			return;
		}
	}

	/*
	 * Write min/max framesize
	 */
	{
		const unsigned min_framesize_offset =
		FLAC__STREAM_METADATA_HEADER_LENGTH +
		(
			FLAC__STREAM_METADATA_STREAMINFO_MIN_BLOCK_SIZE_LEN +
			FLAC__STREAM_METADATA_STREAMINFO_MAX_BLOCK_SIZE_LEN
		) / 8;

		b[0] = (FLAC__byte)((min_framesize >> 16) & 0xFF);
		b[1] = (FLAC__byte)((min_framesize >> 8) & 0xFF);
		b[2] = (FLAC__byte)(min_framesize & 0xFF);
		b[3] = (FLAC__byte)((max_framesize >> 16) & 0xFF);
		b[4] = (FLAC__byte)((max_framesize >> 8) & 0xFF);
		b[5] = (FLAC__byte)(max_framesize & 0xFF);
		if(encoder->private_->seek_callback(encoder, encoder->protected_->streaminfo_offset + min_framesize_offset, encoder->private_->client_data) != FLAC__SEEKABLE_STREAM_ENCODER_SEEK_STATUS_OK) {
			encoder->protected_->state = FLAC__SEEKABLE_STREAM_ENCODER_SEEK_ERROR;
			return;
		}
		if(encoder->private_->write_callback(encoder, b, 6, 0, 0, encoder->private_->client_data) != FLAC__STREAM_ENCODER_WRITE_STATUS_OK) {
			encoder->protected_->state = FLAC__SEEKABLE_STREAM_ENCODER_WRITE_ERROR;
			return;
		}
	}

	/*
	 * Write seektable
	 */
	if(0 != encoder->private_->seek_table && encoder->private_->seek_table->num_points > 0 && encoder->protected_->seektable_offset > 0) {
		unsigned i;

		FLAC__format_seektable_sort(encoder->private_->seek_table);

		FLAC__ASSERT(FLAC__format_seektable_is_legal(encoder->private_->seek_table));

		if(encoder->private_->seek_callback(encoder, encoder->protected_->seektable_offset + FLAC__STREAM_METADATA_HEADER_LENGTH, encoder->private_->client_data) != FLAC__SEEKABLE_STREAM_ENCODER_SEEK_STATUS_OK) {
			encoder->protected_->state = FLAC__SEEKABLE_STREAM_ENCODER_SEEK_ERROR;
			return;
		}

		for(i = 0; i < encoder->private_->seek_table->num_points; i++) {
			FLAC__uint64 xx;
			unsigned x;
			xx = encoder->private_->seek_table->points[i].sample_number;
			b[7] = (FLAC__byte)xx; xx >>= 8;
			b[6] = (FLAC__byte)xx; xx >>= 8;
			b[5] = (FLAC__byte)xx; xx >>= 8;
			b[4] = (FLAC__byte)xx; xx >>= 8;
			b[3] = (FLAC__byte)xx; xx >>= 8;
			b[2] = (FLAC__byte)xx; xx >>= 8;
			b[1] = (FLAC__byte)xx; xx >>= 8;
			b[0] = (FLAC__byte)xx; xx >>= 8;
			xx = encoder->private_->seek_table->points[i].stream_offset;
			b[15] = (FLAC__byte)xx; xx >>= 8;
			b[14] = (FLAC__byte)xx; xx >>= 8;
			b[13] = (FLAC__byte)xx; xx >>= 8;
			b[12] = (FLAC__byte)xx; xx >>= 8;
			b[11] = (FLAC__byte)xx; xx >>= 8;
			b[10] = (FLAC__byte)xx; xx >>= 8;
			b[9] = (FLAC__byte)xx; xx >>= 8;
			b[8] = (FLAC__byte)xx; xx >>= 8;
			x = encoder->private_->seek_table->points[i].frame_samples;
			b[17] = (FLAC__byte)x; x >>= 8;
			b[16] = (FLAC__byte)x; x >>= 8;
			if(encoder->private_->write_callback(encoder, b, 18, 0, 0, encoder->private_->client_data) != FLAC__STREAM_ENCODER_WRITE_STATUS_OK) {
				encoder->protected_->state = FLAC__SEEKABLE_STREAM_ENCODER_WRITE_ERROR;
				return;
			}
		}
	}
}
