/* libFLAC - Free Lossless Audio Codec library
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
#include "FLAC/assert.h"
#include "protected/seekable_stream_encoder.h"
#include "protected/stream_encoder.h"

#ifdef max
#undef max
#endif
#define max(a,b) ((a)>(b)?(a):(b))

/***********************************************************************
 *
 * Private class method prototypes
 *
 ***********************************************************************/

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
	FLAC__SeekableStreamEncoderWriteCallback write_callback;
	void *client_data;
	FLAC__StreamEncoder *stream_encoder;
	FLAC__StreamMetadata_SeekTable *seek_table;
	/* internal vars (all the above are class settings) */
	unsigned first_seekpoint_to_check;
	FLAC__uint64 stream_offset, seektable_offset;
	FLAC__uint64 bytes_written;
} FLAC__SeekableStreamEncoderPrivate;

/***********************************************************************
 *
 * Public static class data
 *
 ***********************************************************************/

const char * const FLAC__SeekableStreamEncoderStateString[] = {
	"FLAC__SEEKABLE_STREAM_ENCODER_OK",
	"FLAC__SEEKABLE_STREAM_ENCODER_STREAM_ENCODER_ERROR",
	"FLAC__SEEKABLE_STREAM_ENCODER_MEMORY_ALLOCATION_ERROR",
	"FLAC__SEEKABLE_STREAM_ENCODER_READ_ERROR",
	"FLAC__SEEKABLE_STREAM_ENCODER_WRITE_ERROR",
	"FLAC__SEEKABLE_STREAM_ENCODER_SEEK_ERROR",
	"FLAC__SEEKABLE_STREAM_ENCODER_ALREADY_INITIALIZED",
	"FLAC__SEEKABLE_STREAM_ENCODER_INVALID_CALLBACK",
	"FLAC__SEEKABLE_STREAM_ENCODER_INVALID_SEEKTABLE",
	"FLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED"
};

const char * const FLAC__SeekableStreamEncoderSeekStatusString[] = {
	"FLAC__SEEKABLE_STREAM_ENCODER_SEEK_STATUS_OK",
	"FLAC__SEEKABLE_STREAM_ENCODER_SEEK_STATUS_ERROR"
};


/***********************************************************************
 *
 * Class constructor/destructor
 *
 ***********************************************************************/

FLAC__SeekableStreamEncoder *FLAC__seekable_stream_encoder_new()
{
	FLAC__SeekableStreamEncoder *encoder;

	FLAC__ASSERT(sizeof(int) >= 4); /* we want to die right away if this is not true */

	encoder = (FLAC__SeekableStreamEncoder*)malloc(sizeof(FLAC__SeekableStreamEncoder));
	if(encoder == 0) {
		return 0;
	}
	encoder->protected_ = (FLAC__SeekableStreamEncoderProtected*)malloc(sizeof(FLAC__SeekableStreamEncoderProtected));
	if(encoder->protected_ == 0) {
		free(encoder);
		return 0;
	}
	encoder->private_ = (FLAC__SeekableStreamEncoderPrivate*)malloc(sizeof(FLAC__SeekableStreamEncoderPrivate));
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

void FLAC__seekable_stream_encoder_delete(FLAC__SeekableStreamEncoder *encoder)
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

FLAC__SeekableStreamEncoderState FLAC__seekable_stream_encoder_init(FLAC__SeekableStreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);

	if(encoder->protected_->state != FLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return encoder->protected_->state = FLAC__SEEKABLE_STREAM_ENCODER_ALREADY_INITIALIZED;

	if(0 == encoder->private_->seek_callback || 0 == encoder->private_->write_callback)
		return encoder->protected_->state = FLAC__SEEKABLE_STREAM_ENCODER_INVALID_CALLBACK;

	if(0 != encoder->private_->seek_table && !FLAC__format_seektable_is_legal(encoder->private_->seek_table))
		return encoder->protected_->state = FLAC__SEEKABLE_STREAM_ENCODER_INVALID_SEEKTABLE;

	/*
	 * This must be done before we init the stream encoder because that
	 * calls the write_callback, which uses these values.
	 */
	encoder->private_->first_seekpoint_to_check = 0;
	encoder->private_->stream_offset = 0;
	encoder->private_->seektable_offset = 0;
	encoder->private_->bytes_written = 0;

	FLAC__stream_encoder_set_write_callback(encoder->private_->stream_encoder, write_callback_);
	FLAC__stream_encoder_set_metadata_callback(encoder->private_->stream_encoder, metadata_callback_);
	FLAC__stream_encoder_set_client_data(encoder->private_->stream_encoder, encoder);

	if(FLAC__stream_encoder_init(encoder->private_->stream_encoder) != FLAC__STREAM_ENCODER_OK)
		return encoder->protected_->state = FLAC__SEEKABLE_STREAM_ENCODER_STREAM_ENCODER_ERROR;

	/*
	 * Initializing the stream encoder writes all the metadata, so we
	 * save the stream offset now.
	 */
	encoder->private_->stream_offset = encoder->private_->bytes_written; 

	return encoder->protected_->state = FLAC__SEEKABLE_STREAM_ENCODER_OK;
}

void FLAC__seekable_stream_encoder_finish(FLAC__SeekableStreamEncoder *encoder)
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

FLAC__bool FLAC__seekable_stream_encoder_set_streamable_subset(FLAC__SeekableStreamEncoder *encoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->stream_encoder);
	if(encoder->protected_->state != FLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_streamable_subset(encoder->private_->stream_encoder, value);
}

FLAC__bool FLAC__seekable_stream_encoder_set_do_mid_side_stereo(FLAC__SeekableStreamEncoder *encoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->stream_encoder);
	if(encoder->protected_->state != FLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_do_mid_side_stereo(encoder->private_->stream_encoder, value);
}

FLAC__bool FLAC__seekable_stream_encoder_set_loose_mid_side_stereo(FLAC__SeekableStreamEncoder *encoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->stream_encoder);
	if(encoder->protected_->state != FLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_loose_mid_side_stereo(encoder->private_->stream_encoder, value);
}

FLAC__bool FLAC__seekable_stream_encoder_set_channels(FLAC__SeekableStreamEncoder *encoder, unsigned value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->stream_encoder);
	if(encoder->protected_->state != FLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_channels(encoder->private_->stream_encoder, value);
}

FLAC__bool FLAC__seekable_stream_encoder_set_bits_per_sample(FLAC__SeekableStreamEncoder *encoder, unsigned value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->stream_encoder);
	if(encoder->protected_->state != FLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_bits_per_sample(encoder->private_->stream_encoder, value);
}

FLAC__bool FLAC__seekable_stream_encoder_set_sample_rate(FLAC__SeekableStreamEncoder *encoder, unsigned value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->stream_encoder);
	if(encoder->protected_->state != FLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_sample_rate(encoder->private_->stream_encoder, value);
}

FLAC__bool FLAC__seekable_stream_encoder_set_blocksize(FLAC__SeekableStreamEncoder *encoder, unsigned value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->stream_encoder);
	if(encoder->protected_->state != FLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_blocksize(encoder->private_->stream_encoder, value);
}

FLAC__bool FLAC__seekable_stream_encoder_set_max_lpc_order(FLAC__SeekableStreamEncoder *encoder, unsigned value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->stream_encoder);
	if(encoder->protected_->state != FLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_max_lpc_order(encoder->private_->stream_encoder, value);
}

FLAC__bool FLAC__seekable_stream_encoder_set_qlp_coeff_precision(FLAC__SeekableStreamEncoder *encoder, unsigned value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->stream_encoder);
	if(encoder->protected_->state != FLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_qlp_coeff_precision(encoder->private_->stream_encoder, value);
}

FLAC__bool FLAC__seekable_stream_encoder_set_do_qlp_coeff_prec_search(FLAC__SeekableStreamEncoder *encoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->stream_encoder);
	if(encoder->protected_->state != FLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_do_qlp_coeff_prec_search(encoder->private_->stream_encoder, value);
}

FLAC__bool FLAC__seekable_stream_encoder_set_do_escape_coding(FLAC__SeekableStreamEncoder *encoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->stream_encoder);
	if(encoder->protected_->state != FLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_do_escape_coding(encoder->private_->stream_encoder, value);
}

FLAC__bool FLAC__seekable_stream_encoder_set_do_exhaustive_model_search(FLAC__SeekableStreamEncoder *encoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->stream_encoder);
	if(encoder->protected_->state != FLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_do_exhaustive_model_search(encoder->private_->stream_encoder, value);
}

FLAC__bool FLAC__seekable_stream_encoder_set_min_residual_partition_order(FLAC__SeekableStreamEncoder *encoder, unsigned value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->stream_encoder);
	if(encoder->protected_->state != FLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_min_residual_partition_order(encoder->private_->stream_encoder, value);
}

FLAC__bool FLAC__seekable_stream_encoder_set_max_residual_partition_order(FLAC__SeekableStreamEncoder *encoder, unsigned value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->stream_encoder);
	if(encoder->protected_->state != FLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_max_residual_partition_order(encoder->private_->stream_encoder, value);
}

FLAC__bool FLAC__seekable_stream_encoder_set_rice_parameter_search_dist(FLAC__SeekableStreamEncoder *encoder, unsigned value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->stream_encoder);
	if(encoder->protected_->state != FLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_rice_parameter_search_dist(encoder->private_->stream_encoder, value);
}

FLAC__bool FLAC__seekable_stream_encoder_set_total_samples_estimate(FLAC__SeekableStreamEncoder *encoder, FLAC__uint64 value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->stream_encoder);
	if(encoder->protected_->state != FLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__stream_encoder_set_total_samples_estimate(encoder->private_->stream_encoder, value);
}

FLAC__bool FLAC__seekable_stream_encoder_set_metadata(FLAC__SeekableStreamEncoder *encoder, FLAC__StreamMetadata **metadata, unsigned num_blocks)
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

FLAC__bool FLAC__seekable_stream_encoder_set_seek_callback(FLAC__SeekableStreamEncoder *encoder, FLAC__SeekableStreamEncoderSeekCallback value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	if(encoder->protected_->state != FLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return false;
	encoder->private_->seek_callback = value;
	return true;
}

FLAC__bool FLAC__seekable_stream_encoder_set_write_callback(FLAC__SeekableStreamEncoder *encoder, FLAC__SeekableStreamEncoderWriteCallback value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	if(encoder->protected_->state != FLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return false;
	encoder->private_->write_callback = value;
	return true;
}

FLAC__bool FLAC__seekable_stream_encoder_set_client_data(FLAC__SeekableStreamEncoder *encoder, void *value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	if(encoder->protected_->state != FLAC__SEEKABLE_STREAM_ENCODER_UNINITIALIZED)
		return false;
	encoder->private_->client_data = value;
	return true;
}

FLAC__SeekableStreamEncoderState FLAC__seekable_stream_encoder_get_state(const FLAC__SeekableStreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->protected_);
	return encoder->protected_->state;
}

FLAC__SeekableStreamEncoderState FLAC__seekable_stream_encoder_get_stream_encoder_state(const FLAC__SeekableStreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return FLAC__stream_encoder_get_state(encoder->private_->stream_encoder);
}

FLAC__bool FLAC__seekable_stream_encoder_get_streamable_subset(const FLAC__SeekableStreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return FLAC__stream_encoder_get_streamable_subset(encoder->private_->stream_encoder);
}

FLAC__bool FLAC__seekable_stream_encoder_get_do_mid_side_stereo(const FLAC__SeekableStreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return FLAC__stream_encoder_get_do_mid_side_stereo(encoder->private_->stream_encoder);
}

FLAC__bool FLAC__seekable_stream_encoder_get_loose_mid_side_stereo(const FLAC__SeekableStreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return FLAC__stream_encoder_get_loose_mid_side_stereo(encoder->private_->stream_encoder);
}

unsigned FLAC__seekable_stream_encoder_get_channels(const FLAC__SeekableStreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return FLAC__stream_encoder_get_channels(encoder->private_->stream_encoder);
}

unsigned FLAC__seekable_stream_encoder_get_bits_per_sample(const FLAC__SeekableStreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return FLAC__stream_encoder_get_bits_per_sample(encoder->private_->stream_encoder);
}

unsigned FLAC__seekable_stream_encoder_get_sample_rate(const FLAC__SeekableStreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return FLAC__stream_encoder_get_sample_rate(encoder->private_->stream_encoder);
}

unsigned FLAC__seekable_stream_encoder_get_blocksize(const FLAC__SeekableStreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return FLAC__stream_encoder_get_blocksize(encoder->private_->stream_encoder);
}

unsigned FLAC__seekable_stream_encoder_get_max_lpc_order(const FLAC__SeekableStreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return FLAC__stream_encoder_get_max_lpc_order(encoder->private_->stream_encoder);
}

unsigned FLAC__seekable_stream_encoder_get_qlp_coeff_precision(const FLAC__SeekableStreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return FLAC__stream_encoder_get_qlp_coeff_precision(encoder->private_->stream_encoder);
}

FLAC__bool FLAC__seekable_stream_encoder_get_do_qlp_coeff_prec_search(const FLAC__SeekableStreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return FLAC__stream_encoder_get_do_qlp_coeff_prec_search(encoder->private_->stream_encoder);
}

FLAC__bool FLAC__seekable_stream_encoder_get_do_escape_coding(const FLAC__SeekableStreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return FLAC__stream_encoder_get_do_escape_coding(encoder->private_->stream_encoder);
}

FLAC__bool FLAC__seekable_stream_encoder_get_do_exhaustive_model_search(const FLAC__SeekableStreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return FLAC__stream_encoder_get_do_exhaustive_model_search(encoder->private_->stream_encoder);
}

unsigned FLAC__seekable_stream_encoder_get_min_residual_partition_order(const FLAC__SeekableStreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return FLAC__stream_encoder_get_min_residual_partition_order(encoder->private_->stream_encoder);
}

unsigned FLAC__seekable_stream_encoder_get_max_residual_partition_order(const FLAC__SeekableStreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return FLAC__stream_encoder_get_max_residual_partition_order(encoder->private_->stream_encoder);
}

unsigned FLAC__seekable_stream_encoder_get_rice_parameter_search_dist(const FLAC__SeekableStreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return FLAC__stream_encoder_get_rice_parameter_search_dist(encoder->private_->stream_encoder);
}

FLAC__uint64 FLAC__seekable_stream_encoder_get_total_samples_estimate(const FLAC__SeekableStreamEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return FLAC__stream_encoder_get_total_samples_estimate(encoder->private_->stream_encoder);
}

FLAC__bool FLAC__seekable_stream_encoder_process(FLAC__SeekableStreamEncoder *encoder, const FLAC__int32 * const buffer[], unsigned samples)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return FLAC__stream_encoder_process(encoder->private_->stream_encoder, buffer, samples);
}

/* 'samples' is channel-wide samples, e.g. for 1 second at 44100Hz, 'samples' = 44100 regardless of the number of channels */
FLAC__bool FLAC__seekable_stream_encoder_process_interleaved(FLAC__SeekableStreamEncoder *encoder, const FLAC__int32 buffer[], unsigned samples)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return FLAC__stream_encoder_process_interleaved(encoder->private_->stream_encoder, buffer, samples);
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
	encoder->private_->write_callback = 0;
	encoder->private_->client_data = 0;

	encoder->private_->seek_table = 0;
}

FLAC__StreamEncoderWriteStatus write_callback_(const FLAC__StreamEncoder *encoder, const FLAC__byte buffer[], unsigned bytes, unsigned samples, unsigned current_frame, void *client_data)
{
	FLAC__SeekableStreamEncoder *seekable_stream_encoder = (FLAC__SeekableStreamEncoder*)client_data;
	FLAC__StreamEncoderWriteStatus status;

	/*
	 * Watch for the first SEEKTABLE block to go by and store its offset.
	 */
	if(samples == 0 && (buffer[0] & 0x7f) == FLAC__METADATA_TYPE_SEEKTABLE)
		seekable_stream_encoder->private_->seektable_offset = seekable_stream_encoder->private_->bytes_written;

	/*
	 * Mark the current seek point if hit (if stream_offset == 0 that
	 * means we're still writing metadata and haven't hit the first
	 * frame yet)
	 */
	if(seekable_stream_encoder->private_->stream_offset > 0 && seekable_stream_encoder->private_->seek_table->num_points > 0) {
		/*@@@ WATCHOUT: assumes the encoder is fixed-blocksize, which will be true indefinitely: */
		const unsigned blocksize = FLAC__stream_encoder_get_blocksize(encoder);
		const FLAC__uint64 frame_first_sample = (FLAC__uint64)current_frame * (FLAC__uint64)blocksize;
		const FLAC__uint64 frame_last_sample = frame_first_sample + (FLAC__uint64)blocksize - 1;
		FLAC__uint64 test_sample;
		unsigned i;
		for(i = seekable_stream_encoder->private_->first_seekpoint_to_check; i < seekable_stream_encoder->private_->seek_table->num_points; i++) {
			test_sample = seekable_stream_encoder->private_->seek_table->points[i].sample_number;
			if(test_sample > frame_last_sample) {
				break;
			}
			else if(test_sample >= frame_first_sample) {
				seekable_stream_encoder->private_->seek_table->points[i].sample_number = frame_first_sample;
				seekable_stream_encoder->private_->seek_table->points[i].stream_offset = seekable_stream_encoder->private_->bytes_written - seekable_stream_encoder->private_->stream_offset;
				seekable_stream_encoder->private_->seek_table->points[i].frame_samples = blocksize;
				seekable_stream_encoder->private_->first_seekpoint_to_check++;
				/* DO NOT: "break;" and here's why:
				 * The seektable template may contain more than one target
				 * sample for any given frame; we will keep looping, generating
				 * duplicate seekpoints for them, and we'll clean it up later,
				 * just before writing the seektable back to the metadata.
				 */
			}
			else {
				seekable_stream_encoder->private_->first_seekpoint_to_check++;
			}
		}
	}

	status = seekable_stream_encoder->private_->write_callback(seekable_stream_encoder, buffer, bytes, samples, current_frame, seekable_stream_encoder->private_->client_data);

	if(status == FLAC__STREAM_ENCODER_WRITE_STATUS_OK)
		seekable_stream_encoder->private_->bytes_written += bytes;
	else
		seekable_stream_encoder->protected_->state = FLAC__SEEKABLE_STREAM_ENCODER_WRITE_ERROR;

	return status;
}

void metadata_callback_(const FLAC__StreamEncoder *encoder, const FLAC__StreamMetadata *metadata, void *client_data)
{
	FLAC__SeekableStreamEncoder *seekable_stream_encoder = (FLAC__SeekableStreamEncoder*)client_data;
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

	(void)encoder; /* silence compiler warning about unused parameter */

	/*@@@ reopen callback here? */

	/* All this is based on intimate knowledge of the stream header
	 * layout, but a change to the header format that would break this
	 * would also break all streams encoded in the previous format.
	 */

	/*
	 * Write MD5 signature
	 */
	if(seekable_stream_encoder->private_->seek_callback(seekable_stream_encoder, 26, seekable_stream_encoder->private_->client_data) != FLAC__SEEKABLE_STREAM_ENCODER_SEEK_STATUS_OK) {
		seekable_stream_encoder->protected_->state = FLAC__SEEKABLE_STREAM_ENCODER_SEEK_ERROR;
		return;
	}
	if(seekable_stream_encoder->private_->write_callback(seekable_stream_encoder, metadata->data.stream_info.md5sum, 16, 0, 0, seekable_stream_encoder->private_->client_data) != FLAC__STREAM_ENCODER_WRITE_STATUS_OK) {
		seekable_stream_encoder->protected_->state = FLAC__SEEKABLE_STREAM_ENCODER_WRITE_ERROR;
		return;
	}

	/*
	 * Write total samples
	 */
	b[0] = ((FLAC__byte)(bps-1) << 4) | (FLAC__byte)((samples >> 32) & 0x0F);
	b[1] = (FLAC__byte)((samples >> 24) & 0xFF);
	b[2] = (FLAC__byte)((samples >> 16) & 0xFF);
	b[3] = (FLAC__byte)((samples >> 8) & 0xFF);
	b[4] = (FLAC__byte)(samples & 0xFF);
	if(seekable_stream_encoder->private_->seek_callback(seekable_stream_encoder, 21, seekable_stream_encoder->private_->client_data) != FLAC__SEEKABLE_STREAM_ENCODER_SEEK_STATUS_OK) {
		seekable_stream_encoder->protected_->state = FLAC__SEEKABLE_STREAM_ENCODER_SEEK_ERROR;
		return;
	}
	if(seekable_stream_encoder->private_->write_callback(seekable_stream_encoder, b, 5, 0, 0, seekable_stream_encoder->private_->client_data) != FLAC__STREAM_ENCODER_WRITE_STATUS_OK) {
		seekable_stream_encoder->protected_->state = FLAC__SEEKABLE_STREAM_ENCODER_WRITE_ERROR;
		return;
	}

	/*
	 * Write min/max framesize
	 */
	b[0] = (FLAC__byte)((min_framesize >> 16) & 0xFF);
	b[1] = (FLAC__byte)((min_framesize >> 8) & 0xFF);
	b[2] = (FLAC__byte)(min_framesize & 0xFF);
	b[3] = (FLAC__byte)((max_framesize >> 16) & 0xFF);
	b[4] = (FLAC__byte)((max_framesize >> 8) & 0xFF);
	b[5] = (FLAC__byte)(max_framesize & 0xFF);
	if(seekable_stream_encoder->private_->seek_callback(seekable_stream_encoder, 12, seekable_stream_encoder->private_->client_data) != FLAC__SEEKABLE_STREAM_ENCODER_SEEK_STATUS_OK) {
		seekable_stream_encoder->protected_->state = FLAC__SEEKABLE_STREAM_ENCODER_SEEK_ERROR;
		return;
	}
	if(seekable_stream_encoder->private_->write_callback(seekable_stream_encoder, b, 6, 0, 0, seekable_stream_encoder->private_->client_data) != FLAC__STREAM_ENCODER_WRITE_STATUS_OK) {
		seekable_stream_encoder->protected_->state = FLAC__SEEKABLE_STREAM_ENCODER_WRITE_ERROR;
		return;
	}

	/*
	 * Write seektable
	 */
	if(seekable_stream_encoder->private_->seek_table->num_points > 0 && seekable_stream_encoder->private_->seektable_offset > 0) {
		unsigned i;

		FLAC__format_seektable_sort(seekable_stream_encoder->private_->seek_table);

		FLAC__ASSERT(FLAC__format_seektable_is_legal(seekable_stream_encoder->private_->seek_table));

		if(seekable_stream_encoder->private_->seek_callback(seekable_stream_encoder, seekable_stream_encoder->private_->seektable_offset, seekable_stream_encoder->private_->client_data) != FLAC__SEEKABLE_STREAM_ENCODER_SEEK_STATUS_OK) {
			seekable_stream_encoder->protected_->state = FLAC__SEEKABLE_STREAM_ENCODER_SEEK_ERROR;
			return;
		}

		for(i = 0; i < seekable_stream_encoder->private_->seek_table->num_points; i++) {
			FLAC__uint64 xx;
			unsigned x;
			xx = seekable_stream_encoder->private_->seek_table->points[i].sample_number;
			b[7] = (FLAC__byte)xx; xx >>= 8;
			b[6] = (FLAC__byte)xx; xx >>= 8;
			b[5] = (FLAC__byte)xx; xx >>= 8;
			b[4] = (FLAC__byte)xx; xx >>= 8;
			b[3] = (FLAC__byte)xx; xx >>= 8;
			b[2] = (FLAC__byte)xx; xx >>= 8;
			b[1] = (FLAC__byte)xx; xx >>= 8;
			b[0] = (FLAC__byte)xx; xx >>= 8;
			xx = seekable_stream_encoder->private_->seek_table->points[i].stream_offset;
			b[15] = (FLAC__byte)xx; xx >>= 8;
			b[14] = (FLAC__byte)xx; xx >>= 8;
			b[13] = (FLAC__byte)xx; xx >>= 8;
			b[12] = (FLAC__byte)xx; xx >>= 8;
			b[11] = (FLAC__byte)xx; xx >>= 8;
			b[10] = (FLAC__byte)xx; xx >>= 8;
			b[9] = (FLAC__byte)xx; xx >>= 8;
			b[8] = (FLAC__byte)xx; xx >>= 8;
			x = seekable_stream_encoder->private_->seek_table->points[i].frame_samples;
			b[17] = (FLAC__byte)x; x >>= 8;
			b[16] = (FLAC__byte)x; x >>= 8;
			if(seekable_stream_encoder->private_->write_callback(seekable_stream_encoder, b, 18, 0, 0, seekable_stream_encoder->private_->client_data) != FLAC__STREAM_ENCODER_WRITE_STATUS_OK) {
				seekable_stream_encoder->protected_->state = FLAC__SEEKABLE_STREAM_ENCODER_WRITE_ERROR;
				return;
			}
		}
	}
}
