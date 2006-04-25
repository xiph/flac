/* libOggFLAC - Free Lossless Audio Codec + Ogg library
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
#include "OggFLAC/seekable_stream_encoder.h"
#include "protected/seekable_stream_encoder.h"
#include "private/ogg_helper.h"

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

static void set_defaults_(OggFLAC__SeekableStreamEncoder *encoder);
static FLAC__StreamEncoderWriteStatus write_callback_(const FLAC__StreamEncoder *encoder, const FLAC__byte buffer[], unsigned bytes, unsigned samples, unsigned current_frame, void *client_data);
static void metadata_callback_(const FLAC__StreamEncoder *encoder, const FLAC__StreamMetadata *metadata, void *client_data);


/***********************************************************************
 *
 * Private class data
 *
 ***********************************************************************/

typedef struct OggFLAC__SeekableStreamEncoderPrivate {
	OggFLAC__SeekableStreamEncoderReadCallback read_callback;
	OggFLAC__SeekableStreamEncoderSeekCallback seek_callback;
	OggFLAC__SeekableStreamEncoderTellCallback tell_callback;
	OggFLAC__SeekableStreamEncoderWriteCallback write_callback;
	void *client_data;
	FLAC__StreamEncoder *FLAC_stream_encoder;
	FLAC__StreamMetadata_SeekTable *seek_table;
	/* internal vars (all the above are class settings) */
	unsigned first_seekpoint_to_check;
	FLAC__uint64 samples_written;
} OggFLAC__SeekableStreamEncoderPrivate;


/***********************************************************************
 *
 * Public static class data
 *
 ***********************************************************************/

OggFLAC_API const char * const OggFLAC__SeekableStreamEncoderStateString[] = {
	"OggFLAC__SEEKABLE_STREAM_ENCODER_OK",
	"OggFLAC__SEEKABLE_STREAM_ENCODER_OGG_ERROR",
	"OggFLAC__SEEKABLE_STREAM_ENCODER_FLAC_STREAM_ENCODER_ERROR",
	"OggFLAC__SEEKABLE_STREAM_ENCODER_MEMORY_ALLOCATION_ERROR",
	"OggFLAC__SEEKABLE_STREAM_ENCODER_WRITE_ERROR",
	"OggFLAC__SEEKABLE_STREAM_ENCODER_READ_ERROR",
	"OggFLAC__SEEKABLE_STREAM_ENCODER_READ_ERROR",
	"OggFLAC__SEEKABLE_STREAM_ENCODER_SEEK_ERROR",
	"OggFLAC__SEEKABLE_STREAM_ENCODER_TELL_ERROR",
	"OggFLAC__SEEKABLE_STREAM_ENCODER_ALREADY_INITIALIZED",
	"OggFLAC__SEEKABLE_STREAM_ENCODER_INVALID_CALLBACK",
	"OggFLAC__SEEKABLE_STREAM_ENCODER_INVALID_SEEKTABLE",
	"OggFLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED"
};

OggFLAC_API const char * const OggFLAC__SeekableStreamEncoderReadStatusString[] = {
	"OggFLAC__SEEKABLE_STREAM_ENCODER_READ_STATUS_CONTINUE",
	"OggFLAC__SEEKABLE_STREAM_ENCODER_READ_STATUS_END_OF_STREAM",
	"OggFLAC__SEEKABLE_STREAM_ENCODER_READ_STATUS_ABORT"
};


/***********************************************************************
 *
 * Class constructor/destructor
 *
 */
OggFLAC_API OggFLAC__SeekableStreamEncoder *OggFLAC__seekable_stream_encoder_new()
{
	OggFLAC__SeekableStreamEncoder *encoder;

	encoder = (OggFLAC__SeekableStreamEncoder*)calloc(1, sizeof(OggFLAC__SeekableStreamEncoder));
	if(encoder == 0) {
		return 0;
	}

	encoder->protected_ = (OggFLAC__SeekableStreamEncoderProtected*)calloc(1, sizeof(OggFLAC__SeekableStreamEncoderProtected));
	if(encoder->protected_ == 0) {
		free(encoder);
		return 0;
	}

	encoder->private_ = (OggFLAC__SeekableStreamEncoderPrivate*)calloc(1, sizeof(OggFLAC__SeekableStreamEncoderPrivate));
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

	encoder->protected_->state = OggFLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED;

	return encoder;
}

OggFLAC_API void OggFLAC__seekable_stream_encoder_delete(OggFLAC__SeekableStreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);

	(void)OggFLAC__seekable_stream_encoder_finish(encoder);

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

OggFLAC_API OggFLAC__SeekableStreamEncoderState OggFLAC__seekable_stream_encoder_init(OggFLAC__SeekableStreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);

	if(encoder->protected_->state != OggFLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return encoder->protected_->state = OggFLAC__SEEKABLE_STREAM_ENCODER_ALREADY_INITIALIZED;

	if(0 == encoder->private_->seek_callback || 0 == encoder->private_->tell_callback || 0 == encoder->private_->write_callback)
		return encoder->protected_->state = OggFLAC__SEEKABLE_STREAM_ENCODER_INVALID_CALLBACK;

	if(!OggFLAC__ogg_encoder_aspect_init(&encoder->protected_->ogg_encoder_aspect))
			return encoder->protected_->state = OggFLAC__SEEKABLE_STREAM_ENCODER_OGG_ERROR;

	if(0 != encoder->private_->seek_table && !FLAC__format_seektable_is_legal(encoder->private_->seek_table))
		return encoder->protected_->state = OggFLAC__SEEKABLE_STREAM_ENCODER_INVALID_SEEKTABLE;

	/*
	 * These must be done before we init the stream encoder because that
	 * calls the write_callback, which uses these values.
	 */
	encoder->private_->first_seekpoint_to_check = 0;
	encoder->private_->samples_written = 0;
	encoder->protected_->streaminfo_offset = 0;
	encoder->protected_->seektable_offset = 0;
	encoder->protected_->audio_offset = 0;

	FLAC__stream_encoder_set_write_callback(encoder->private_->FLAC_stream_encoder, write_callback_);
	FLAC__stream_encoder_set_metadata_callback(encoder->private_->FLAC_stream_encoder, metadata_callback_);
	FLAC__stream_encoder_set_client_data(encoder->private_->FLAC_stream_encoder, encoder);

	if(FLAC__stream_encoder_init(encoder->private_->FLAC_stream_encoder) != FLAC__STREAM_ENCODER_OK)
		return encoder->protected_->state = OggFLAC__SEEKABLE_STREAM_ENCODER_FLAC_STREAM_ENCODER_ERROR;

	/*
	 * Initializing the stream encoder writes all the metadata, so we
	 * save the stream offset now.
	 */
	if(encoder->private_->tell_callback(encoder, &encoder->protected_->audio_offset, encoder->private_->client_data) != FLAC__SEEKABLE_STREAM_ENCODER_TELL_STATUS_OK)
		return encoder->protected_->state = OggFLAC__SEEKABLE_STREAM_ENCODER_TELL_ERROR;

	return encoder->protected_->state = OggFLAC__SEEKABLE_STREAM_ENCODER_OK;
}

OggFLAC_API void OggFLAC__seekable_stream_encoder_finish(OggFLAC__SeekableStreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);

	if(encoder->protected_->state == OggFLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return;

	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);

	FLAC__stream_encoder_finish(encoder->private_->FLAC_stream_encoder);

	OggFLAC__ogg_encoder_aspect_finish(&encoder->protected_->ogg_encoder_aspect);

	set_defaults_(encoder);

	encoder->protected_->state = OggFLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED;
}

OggFLAC_API FLAC__bool OggFLAC__seekable_stream_encoder_set_serial_number(OggFLAC__SeekableStreamEncoder *encoder, long value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return false;
	OggFLAC__ogg_encoder_aspect_set_serial_number(&encoder->protected_->ogg_encoder_aspect, value);
	return true;
}

OggFLAC_API FLAC__bool OggFLAC__seekable_stream_encoder_set_verify(OggFLAC__SeekableStreamEncoder *encoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_verify(encoder->private_->FLAC_stream_encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__seekable_stream_encoder_set_streamable_subset(OggFLAC__SeekableStreamEncoder *encoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_streamable_subset(encoder->private_->FLAC_stream_encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__seekable_stream_encoder_set_do_mid_side_stereo(OggFLAC__SeekableStreamEncoder *encoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_do_mid_side_stereo(encoder->private_->FLAC_stream_encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__seekable_stream_encoder_set_loose_mid_side_stereo(OggFLAC__SeekableStreamEncoder *encoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_loose_mid_side_stereo(encoder->private_->FLAC_stream_encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__seekable_stream_encoder_set_channels(OggFLAC__SeekableStreamEncoder *encoder, unsigned value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_channels(encoder->private_->FLAC_stream_encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__seekable_stream_encoder_set_bits_per_sample(OggFLAC__SeekableStreamEncoder *encoder, unsigned value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_bits_per_sample(encoder->private_->FLAC_stream_encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__seekable_stream_encoder_set_sample_rate(OggFLAC__SeekableStreamEncoder *encoder, unsigned value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_sample_rate(encoder->private_->FLAC_stream_encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__seekable_stream_encoder_set_blocksize(OggFLAC__SeekableStreamEncoder *encoder, unsigned value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_blocksize(encoder->private_->FLAC_stream_encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__seekable_stream_encoder_set_apodization(OggFLAC__SeekableStreamEncoder *encoder, const char *specification)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_apodization(encoder->private_->FLAC_stream_encoder, specification);
}

OggFLAC_API FLAC__bool OggFLAC__seekable_stream_encoder_set_max_lpc_order(OggFLAC__SeekableStreamEncoder *encoder, unsigned value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_max_lpc_order(encoder->private_->FLAC_stream_encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__seekable_stream_encoder_set_qlp_coeff_precision(OggFLAC__SeekableStreamEncoder *encoder, unsigned value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_qlp_coeff_precision(encoder->private_->FLAC_stream_encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__seekable_stream_encoder_set_do_qlp_coeff_prec_search(OggFLAC__SeekableStreamEncoder *encoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_do_qlp_coeff_prec_search(encoder->private_->FLAC_stream_encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__seekable_stream_encoder_set_do_escape_coding(OggFLAC__SeekableStreamEncoder *encoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_do_escape_coding(encoder->private_->FLAC_stream_encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__seekable_stream_encoder_set_do_exhaustive_model_search(OggFLAC__SeekableStreamEncoder *encoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_do_exhaustive_model_search(encoder->private_->FLAC_stream_encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__seekable_stream_encoder_set_min_residual_partition_order(OggFLAC__SeekableStreamEncoder *encoder, unsigned value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_min_residual_partition_order(encoder->private_->FLAC_stream_encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__seekable_stream_encoder_set_max_residual_partition_order(OggFLAC__SeekableStreamEncoder *encoder, unsigned value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_max_residual_partition_order(encoder->private_->FLAC_stream_encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__seekable_stream_encoder_set_rice_parameter_search_dist(OggFLAC__SeekableStreamEncoder *encoder, unsigned value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_rice_parameter_search_dist(encoder->private_->FLAC_stream_encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__seekable_stream_encoder_set_total_samples_estimate(OggFLAC__SeekableStreamEncoder *encoder, FLAC__uint64 value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_total_samples_estimate(encoder->private_->FLAC_stream_encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__seekable_stream_encoder_set_metadata(OggFLAC__SeekableStreamEncoder *encoder, FLAC__StreamMetadata **metadata, unsigned num_blocks)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
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
	if(0 != metadata && num_blocks > 0) {
		unsigned i;
		for(i = 0; i < num_blocks; i++) {
			/* keep track of any SEEKTABLE block */
			if(0 != metadata[i] && metadata[i]->type == FLAC__METADATA_TYPE_SEEKTABLE) {
				encoder->private_->seek_table = &metadata[i]->data.seek_table;
				break; /* take only the first one */
			}
		}
	}
	if(!OggFLAC__ogg_encoder_aspect_set_num_metadata(&encoder->protected_->ogg_encoder_aspect, num_blocks))
		return false;
	return FLAC__stream_encoder_set_metadata(encoder->private_->FLAC_stream_encoder, metadata, num_blocks);
}

OggFLAC_API FLAC__bool OggFLAC__seekable_stream_encoder_set_read_callback(OggFLAC__SeekableStreamEncoder *encoder, OggFLAC__SeekableStreamEncoderReadCallback value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != value);
	if(encoder->protected_->state != OggFLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return false;
	encoder->private_->read_callback = value;
	return true;
}

OggFLAC_API FLAC__bool OggFLAC__seekable_stream_encoder_set_seek_callback(OggFLAC__SeekableStreamEncoder *encoder, OggFLAC__SeekableStreamEncoderSeekCallback value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != value);
	if(encoder->protected_->state != OggFLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return false;
	encoder->private_->seek_callback = value;
	return true;
}

OggFLAC_API FLAC__bool OggFLAC__seekable_stream_encoder_set_tell_callback(OggFLAC__SeekableStreamEncoder *encoder, OggFLAC__SeekableStreamEncoderTellCallback value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != value);
	if(encoder->protected_->state != OggFLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return false;
	encoder->private_->tell_callback = value;
	return true;
}

OggFLAC_API FLAC__bool OggFLAC__seekable_stream_encoder_set_write_callback(OggFLAC__SeekableStreamEncoder *encoder, OggFLAC__SeekableStreamEncoderWriteCallback value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != value);
	if(encoder->protected_->state != OggFLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return false;
	encoder->private_->write_callback = value;
	return true;
}

OggFLAC_API FLAC__bool OggFLAC__seekable_stream_encoder_set_client_data(OggFLAC__SeekableStreamEncoder *encoder, void *value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	if(encoder->protected_->state != OggFLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return false;
	encoder->private_->client_data = value;
	return true;
}

/*
 * These three functions are not static, but not publically exposed in
 * include/FLAC/ either.  They are used by the test suite.
 */
OggFLAC_API FLAC__bool OggFLAC__seekable_stream_encoder_disable_constant_subframes(OggFLAC__SeekableStreamEncoder *encoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	if(encoder->protected_->state != OggFLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_disable_constant_subframes(encoder->private_->FLAC_stream_encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__seekable_stream_encoder_disable_fixed_subframes(OggFLAC__SeekableStreamEncoder *encoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	if(encoder->protected_->state != OggFLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_disable_fixed_subframes(encoder->private_->FLAC_stream_encoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__seekable_stream_encoder_disable_verbatim_subframes(OggFLAC__SeekableStreamEncoder *encoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	if(encoder->protected_->state != OggFLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_disable_verbatim_subframes(encoder->private_->FLAC_stream_encoder, value);
}

OggFLAC_API OggFLAC__SeekableStreamEncoderState OggFLAC__seekable_stream_encoder_get_state(const OggFLAC__SeekableStreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	return encoder->protected_->state;
}

OggFLAC_API FLAC__StreamEncoderState OggFLAC__seekable_stream_encoder_get_FLAC_stream_encoder_state(const OggFLAC__SeekableStreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	return FLAC__stream_encoder_get_state(encoder->private_->FLAC_stream_encoder);
}

OggFLAC_API FLAC__StreamDecoderState OggFLAC__seekable_stream_encoder_get_verify_decoder_state(const OggFLAC__SeekableStreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	return FLAC__stream_encoder_get_verify_decoder_state(encoder->private_->FLAC_stream_encoder);
}

OggFLAC_API const char *OggFLAC__seekable_stream_encoder_get_resolved_state_string(const OggFLAC__SeekableStreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(encoder->protected_->state != OggFLAC__SEEKABLE_STREAM_ENCODER_FLAC_STREAM_ENCODER_ERROR)
		return OggFLAC__SeekableStreamEncoderStateString[encoder->protected_->state];
	else
		return FLAC__stream_encoder_get_resolved_state_string(encoder->private_->FLAC_stream_encoder);
}

OggFLAC_API void OggFLAC__seekable_stream_encoder_get_verify_decoder_error_stats(const OggFLAC__SeekableStreamEncoder *encoder, FLAC__uint64 *absolute_sample, unsigned *frame_number, unsigned *channel, unsigned *sample, FLAC__int32 *expected, FLAC__int32 *got)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	FLAC__stream_encoder_get_verify_decoder_error_stats(encoder->private_->FLAC_stream_encoder, absolute_sample, frame_number, channel, sample, expected, got);
}

OggFLAC_API FLAC__bool OggFLAC__seekable_stream_encoder_get_verify(const OggFLAC__SeekableStreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	return FLAC__stream_encoder_get_verify(encoder->private_->FLAC_stream_encoder);
}

OggFLAC_API FLAC__bool OggFLAC__seekable_stream_encoder_get_streamable_subset(const OggFLAC__SeekableStreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	return FLAC__stream_encoder_get_streamable_subset(encoder->private_->FLAC_stream_encoder);
}

OggFLAC_API FLAC__bool OggFLAC__seekable_stream_encoder_get_do_mid_side_stereo(const OggFLAC__SeekableStreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	return FLAC__stream_encoder_get_do_mid_side_stereo(encoder->private_->FLAC_stream_encoder);
}

OggFLAC_API FLAC__bool OggFLAC__seekable_stream_encoder_get_loose_mid_side_stereo(const OggFLAC__SeekableStreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	return FLAC__stream_encoder_get_loose_mid_side_stereo(encoder->private_->FLAC_stream_encoder);
}

OggFLAC_API unsigned OggFLAC__seekable_stream_encoder_get_channels(const OggFLAC__SeekableStreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	return FLAC__stream_encoder_get_channels(encoder->private_->FLAC_stream_encoder);
}

OggFLAC_API unsigned OggFLAC__seekable_stream_encoder_get_bits_per_sample(const OggFLAC__SeekableStreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	return FLAC__stream_encoder_get_bits_per_sample(encoder->private_->FLAC_stream_encoder);
}

OggFLAC_API unsigned OggFLAC__seekable_stream_encoder_get_sample_rate(const OggFLAC__SeekableStreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	return FLAC__stream_encoder_get_sample_rate(encoder->private_->FLAC_stream_encoder);
}

OggFLAC_API unsigned OggFLAC__seekable_stream_encoder_get_blocksize(const OggFLAC__SeekableStreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	return FLAC__stream_encoder_get_blocksize(encoder->private_->FLAC_stream_encoder);
}

OggFLAC_API unsigned OggFLAC__seekable_stream_encoder_get_max_lpc_order(const OggFLAC__SeekableStreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	return FLAC__stream_encoder_get_max_lpc_order(encoder->private_->FLAC_stream_encoder);
}

OggFLAC_API unsigned OggFLAC__seekable_stream_encoder_get_qlp_coeff_precision(const OggFLAC__SeekableStreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	return FLAC__stream_encoder_get_qlp_coeff_precision(encoder->private_->FLAC_stream_encoder);
}

OggFLAC_API FLAC__bool OggFLAC__seekable_stream_encoder_get_do_qlp_coeff_prec_search(const OggFLAC__SeekableStreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	return FLAC__stream_encoder_get_do_qlp_coeff_prec_search(encoder->private_->FLAC_stream_encoder);
}

OggFLAC_API FLAC__bool OggFLAC__seekable_stream_encoder_get_do_escape_coding(const OggFLAC__SeekableStreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	return FLAC__stream_encoder_get_do_escape_coding(encoder->private_->FLAC_stream_encoder);
}

OggFLAC_API FLAC__bool OggFLAC__seekable_stream_encoder_get_do_exhaustive_model_search(const OggFLAC__SeekableStreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	return FLAC__stream_encoder_get_do_exhaustive_model_search(encoder->private_->FLAC_stream_encoder);
}

OggFLAC_API unsigned OggFLAC__seekable_stream_encoder_get_min_residual_partition_order(const OggFLAC__SeekableStreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	return FLAC__stream_encoder_get_min_residual_partition_order(encoder->private_->FLAC_stream_encoder);
}

OggFLAC_API unsigned OggFLAC__seekable_stream_encoder_get_max_residual_partition_order(const OggFLAC__SeekableStreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	return FLAC__stream_encoder_get_max_residual_partition_order(encoder->private_->FLAC_stream_encoder);
}

OggFLAC_API unsigned OggFLAC__seekable_stream_encoder_get_rice_parameter_search_dist(const OggFLAC__SeekableStreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	return FLAC__stream_encoder_get_rice_parameter_search_dist(encoder->private_->FLAC_stream_encoder);
}

OggFLAC_API FLAC__uint64 OggFLAC__seekable_stream_encoder_get_total_samples_estimate(const OggFLAC__SeekableStreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	return FLAC__stream_encoder_get_total_samples_estimate(encoder->private_->FLAC_stream_encoder);
}

OggFLAC_API FLAC__bool OggFLAC__seekable_stream_encoder_process(OggFLAC__SeekableStreamEncoder *encoder, const FLAC__int32 * const buffer[], unsigned samples)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(!FLAC__stream_encoder_process(encoder->private_->FLAC_stream_encoder, buffer, samples)) {
		encoder->protected_->state = OggFLAC__SEEKABLE_STREAM_ENCODER_FLAC_STREAM_ENCODER_ERROR;
		return false;
	}
	else
		return true;
}

OggFLAC_API FLAC__bool OggFLAC__seekable_stream_encoder_process_interleaved(OggFLAC__SeekableStreamEncoder *encoder, const FLAC__int32 buffer[], unsigned samples)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->FLAC_stream_encoder);
	if(!FLAC__stream_encoder_process_interleaved(encoder->private_->FLAC_stream_encoder, buffer, samples)) {
		encoder->protected_->state = OggFLAC__SEEKABLE_STREAM_ENCODER_FLAC_STREAM_ENCODER_ERROR;
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

void set_defaults_(OggFLAC__SeekableStreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);

	encoder->private_->seek_callback = 0;
	encoder->private_->tell_callback = 0;
	encoder->private_->write_callback = 0;
	encoder->private_->client_data = 0;

	encoder->private_->seek_table = 0;

	OggFLAC__ogg_encoder_aspect_set_defaults(&encoder->protected_->ogg_encoder_aspect);
}

FLAC__StreamEncoderWriteStatus write_callback_(const FLAC__StreamEncoder *unused, const FLAC__byte buffer[], unsigned bytes, unsigned samples, unsigned current_frame, void *client_data)
{
	OggFLAC__SeekableStreamEncoder *encoder = (OggFLAC__SeekableStreamEncoder*)client_data;
	FLAC__StreamEncoderWriteStatus status;
	FLAC__uint64 output_position;

	(void)unused; /* silence compiler warning about unused parameter */
	FLAC__ASSERT(encoder->private_->FLAC_stream_encoder == unused);

	if(encoder->private_->tell_callback(encoder, &output_position, encoder->private_->client_data) != FLAC__SEEKABLE_STREAM_ENCODER_TELL_STATUS_OK)
		return encoder->protected_->state = OggFLAC__SEEKABLE_STREAM_ENCODER_TELL_ERROR;

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
		const unsigned blocksize = FLAC__stream_encoder_get_blocksize(encoder->private_->FLAC_stream_encoder);
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

	status = OggFLAC__ogg_encoder_aspect_write_callback_wrapper(&encoder->protected_->ogg_encoder_aspect, FLAC__stream_encoder_get_total_samples_estimate(encoder->private_->FLAC_stream_encoder), buffer, bytes, samples, current_frame, (OggFLAC__OggEncoderAspectWriteCallbackProxy)encoder->private_->write_callback, encoder, encoder->private_->client_data);

	if(status == FLAC__STREAM_ENCODER_WRITE_STATUS_OK) {
		encoder->private_->samples_written += samples;
	}
	else
		encoder->protected_->state = OggFLAC__SEEKABLE_STREAM_ENCODER_WRITE_ERROR;

	return status;
}

void metadata_callback_(const FLAC__StreamEncoder *unused, const FLAC__StreamMetadata *metadata, void *client_data)
{
	OggFLAC__SeekableStreamEncoder *encoder = (OggFLAC__SeekableStreamEncoder*)client_data;
	FLAC__byte b[max(6, FLAC__STREAM_METADATA_SEEKPOINT_LENGTH)];
	const FLAC__uint64 samples = metadata->data.stream_info.total_samples;
	const unsigned min_framesize = metadata->data.stream_info.min_framesize;
	const unsigned max_framesize = metadata->data.stream_info.max_framesize;
	ogg_page page;

	FLAC__ASSERT(metadata->type == FLAC__METADATA_TYPE_STREAMINFO);

	/* We get called by the stream encoder when the encoding process
	 * has finished so that we can update the STREAMINFO and SEEKTABLE
	 * blocks.
	 */

	(void)unused; /* silence compiler warning about unused parameter */
	FLAC__ASSERT(encoder->private_->FLAC_stream_encoder == unused);

	/*@@@ reopen callback here?  The docs currently require user to open files in update mode from the start */

	/* All this is based on intimate knowledge of the stream header
	 * layout, but a change to the header format that would break this
	 * would also break all streams encoded in the previous format.
	 */

	/*
	 * Write STREAMINFO stats
	 */
	simple_ogg_page__init(&page);
	if(!simple_ogg_page__get_at(encoder, encoder->protected_->streaminfo_offset, &page, encoder->private_->seek_callback, encoder->private_->read_callback, encoder->private_->client_data)) {
		simple_ogg_page__clear(&page);
		return; /* state already set */
	}
	/*
	 * MD5 signature
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

		if(md5_offset + 16 > (unsigned)page.body_len) {
			encoder->protected_->state = OggFLAC__SEEKABLE_STREAM_ENCODER_OGG_ERROR;
			simple_ogg_page__clear(&page);
			return;
		}
		memcpy(page.body + md5_offset, metadata->data.stream_info.md5sum, 16);
	}
	/*
	 * total samples
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

		if(total_samples_byte_offset + 5 > (unsigned)page.body_len) {
			encoder->protected_->state = OggFLAC__SEEKABLE_STREAM_ENCODER_OGG_ERROR;
			simple_ogg_page__clear(&page);
			return;
		}
		b[0] = (FLAC__byte)page.body[total_samples_byte_offset] & 0xF0;
		b[0] |= (FLAC__byte)((samples >> 32) & 0x0F);
		b[1] = (FLAC__byte)((samples >> 24) & 0xFF);
		b[2] = (FLAC__byte)((samples >> 16) & 0xFF);
		b[3] = (FLAC__byte)((samples >> 8) & 0xFF);
		b[4] = (FLAC__byte)(samples & 0xFF);
		memcpy(page.body + total_samples_byte_offset, b, 5);
	}
	/*
	 * min/max framesize
	 */
	{
		const unsigned min_framesize_offset =
			FLAC__STREAM_METADATA_HEADER_LENGTH +
			(
				FLAC__STREAM_METADATA_STREAMINFO_MIN_BLOCK_SIZE_LEN +
				FLAC__STREAM_METADATA_STREAMINFO_MAX_BLOCK_SIZE_LEN
			) / 8;

		if(min_framesize_offset + 6 > (unsigned)page.body_len) {
			encoder->protected_->state = OggFLAC__SEEKABLE_STREAM_ENCODER_OGG_ERROR;
			simple_ogg_page__clear(&page);
			return;
		}
		b[0] = (FLAC__byte)((min_framesize >> 16) & 0xFF);
		b[1] = (FLAC__byte)((min_framesize >> 8) & 0xFF);
		b[2] = (FLAC__byte)(min_framesize & 0xFF);
		b[3] = (FLAC__byte)((max_framesize >> 16) & 0xFF);
		b[4] = (FLAC__byte)((max_framesize >> 8) & 0xFF);
		b[5] = (FLAC__byte)(max_framesize & 0xFF);
		memcpy(page.body + min_framesize_offset, b, 6);
	}
	if(!simple_ogg_page__set_at(encoder, encoder->protected_->streaminfo_offset, &page, encoder->private_->seek_callback, encoder->private_->write_callback, encoder->private_->client_data)) {
		simple_ogg_page__clear(&page);
		return; /* state already set */
	}
	simple_ogg_page__clear(&page);

	/*
	 * Write seektable
	 */
	if(0 != encoder->private_->seek_table && encoder->private_->seek_table->num_points > 0 && encoder->protected_->seektable_offset > 0) {
		unsigned i;
		FLAC__byte *p;

		FLAC__format_seektable_sort(encoder->private_->seek_table);

		FLAC__ASSERT(FLAC__format_seektable_is_legal(encoder->private_->seek_table));

		simple_ogg_page__init(&page);
		if(!simple_ogg_page__get_at(encoder, encoder->protected_->seektable_offset, &page, encoder->private_->seek_callback, encoder->private_->read_callback, encoder->private_->client_data)) {
			simple_ogg_page__clear(&page);
			return; /* state already set */
		}

		if(FLAC__STREAM_METADATA_HEADER_LENGTH + (18*encoder->private_->seek_table->num_points) > (unsigned)page.body_len) {
			encoder->protected_->state = OggFLAC__SEEKABLE_STREAM_ENCODER_OGG_ERROR;
			simple_ogg_page__clear(&page);
			return;
		}

		for(i = 0, p = page.body + FLAC__STREAM_METADATA_HEADER_LENGTH; i < encoder->private_->seek_table->num_points; i++, p += 18) {
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
				encoder->protected_->state = OggFLAC__SEEKABLE_STREAM_ENCODER_WRITE_ERROR;
				simple_ogg_page__clear(&page);
				return;
			}
			memcpy(p, b, 18);
		}

		if(!simple_ogg_page__set_at(encoder, encoder->protected_->seektable_offset, &page, encoder->private_->seek_callback, encoder->private_->write_callback, encoder->private_->client_data)) {
			simple_ogg_page__clear(&page);
			return; /* state already set */
		}
		simple_ogg_page__clear(&page);
	}
}
