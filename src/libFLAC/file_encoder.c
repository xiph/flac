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
#include "protected/file_encoder.h"
#include "protected/seekable_stream_encoder.h"

/***********************************************************************
 *
 * Private class method prototypes
 *
 ***********************************************************************/

static void set_defaults_(FLAC__FileEncoder *encoder);
static FLAC__StreamEncoderWriteStatus file_encoder_write_callback_(const FLAC__StreamEncoder *encoder, const FLAC__byte buffer[], unsigned bytes, unsigned samples, unsigned current_frame, void *client_data);
static void file_encoder_metadata_callback_(const FLAC__StreamEncoder *encoder, const FLAC__StreamMetadata *metadata, void *client_data);

/***********************************************************************
 *
 * Private class data
 *
 ***********************************************************************/

typedef struct FLAC__FileEncoderPrivate {
	FILE *file;
	char *filename;
	FLAC__SeekableStreamEncoder *seekable_stream_encoder;
} FLAC__FileEncoderPrivate;

/***********************************************************************
 *
 * Public static class data
 *
 ***********************************************************************/

const char * const FLAC__FileEncoderStateString[] = {
	"FLAC__FILE_ENCODER_OK",
	"FLAC__FILE_ENCODER_NO_FILENAME",
	"FLAC__FILE_ENCODER_SEEKABLE_STREAM_ENCODER_ERROR",
	"FLAC__FILE_ENCODER_FATAL_ERROR_WHILE_WRITING",
	"FLAC__FILE_ENCODER_ERROR_OPENING_FILE",
	"FLAC__FILE_ENCODER_MEMORY_ALLOCATION_ERROR",
	"FLAC__FILE_ENCODER_ALREADY_INITIALIZED",
	"FLAC__FILE_ENCODER_UNINITIALIZED"
};


/***********************************************************************
 *
 * Class constructor/destructor
 *
 ***********************************************************************/
FLAC__FileEncoder *FLAC__file_encoder_new()
{
	FLAC__FileEncoder *encoder;

	FLAC__ASSERT(sizeof(int) >= 4); /* we want to die right away if this is not true */

	encoder = (FLAC__FileEncoder*)malloc(sizeof(FLAC__FileEncoder));
	if(encoder == 0) {
		return 0;
	}
	encoder->protected_ = (FLAC__FileEncoderProtected*)malloc(sizeof(FLAC__FileEncoderProtected));
	if(encoder->protected_ == 0) {
		free(encoder);
		return 0;
	}
	encoder->private_ = (FLAC__FileEncoderPrivate*)malloc(sizeof(FLAC__FileEncoderPrivate));
	if(encoder->private_ == 0) {
		free(encoder->protected_);
		free(encoder);
		return 0;
	}

	encoder->private_->seekable_stream_encoder = FLAC__seekable_stream_encoder_new();

	if(0 == encoder->private_->seekable_stream_encoder) {
		free(encoder->private_);
		free(encoder->protected_);
		free(encoder);
		return 0;
	}

	encoder->private_->file = 0;

	set_defaults_(encoder);

	encoder->protected_->state = FLAC__FILE_ENCODER_UNINITIALIZED;

	return encoder;
}

void FLAC__file_encoder_delete(FLAC__FileEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->private_->seekable_stream_encoder);

	(void)FLAC__file_encoder_finish(encoder);

	FLAC__seekable_stream_encoder_delete(encoder->private_->seekable_stream_encoder);

	free(encoder->private_);
	free(encoder->protected_);
	free(encoder);
}

/***********************************************************************
 *
 * Public class methods
 *
 ***********************************************************************/

FLAC__FileEncoderState FLAC__file_encoder_init(FLAC__FileEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);

	if(encoder->protected_->state != FLAC__FILE_ENCODER_UNINITIALIZED)
		return encoder->protected_->state = FLAC__FILE_ENCODER_ALREADY_INITIALIZED;

	if(0 == encoder->private_->filename)
		return encoder->protected_->state = FLAC__FILE_ENCODER_NO_FILENAME;

	encoder->private_->file = fopen(encoder->private_->filename, "wb");

	if(encoder->private_->file == 0)
		return encoder->protected_->state = FLAC__FILE_ENCODER_ERROR_OPENING_FILE;

	FLAC__seekable_stream_encoder_set_write_callback(encoder->private_->seekable_stream_encoder, write_callback_);
	FLAC__seekable_stream_encoder_set_metadata_callback(encoder->private_->seekable_stream_encoder, metadata_callback_);
	FLAC__seekable_stream_encoder_set_client_data(encoder->private_->seekable_stream_encoder, encoder);

	if(FLAC__seekable_seekable_stream_encoder_init(encoder->private_->seekable_stream_encoder) != FLAC__SEEKABLE_STREAM_ENCODER_OK)
		return encoder->protected_->state = FLAC__FILE_ENCODER_SEEKABLE_STREAM_ENCODER_ERROR;

	return decoder->protected_->state = FLAC__FILE_ENCODER_OK;
}

void FLAC__file_encoder_finish(FLAC__FileEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);

	if(encoder->protected_->state == FLAC__FILE_ENCODER_UNINITIALIZED)
		return;

	FLAC__ASSERT(0 != encoder->private_->seekable_stream_encoder);

	if(0 != encoder->private_->file) {
		fclose(encoder->private_->file);
		encoder->private_->file = 0;
	}

	if(0 != encoder->private_->filename) {
		free(encoder->private_->filename);
		encoder->private_->filename = 0;
	}

	set_defaults_(encoder);

	encoder->protected_->state = FLAC__FILE_ENCODER_UNINITIALIZED;

	return FLAC__seekable_stream_encoder_finish(encoder->private_->seekable_stream_encoder);
}

FLAC__bool FLAC__file_encoder_set_streamable_subset(FLAC__FileEncoder *encoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->seekable_stream_encoder);
	if(encoder->protected_->state != FLAC__FILE_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__seekable_stream_encoder_set_streamable_subset(encoder->private_->seekable_stream_encoder, value);
}

FLAC__bool FLAC__file_encoder_set_do_mid_side_stereo(FLAC__FileEncoder *encoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->seekable_stream_encoder);
	if(encoder->protected_->state != FLAC__FILE_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__seekable_stream_encoder_set_do_mid_side_stereo(encoder->private_->seekable_stream_encoder, value);
}

FLAC__bool FLAC__file_encoder_set_loose_mid_side_stereo(FLAC__FileEncoder *encoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->seekable_stream_encoder);
	if(encoder->protected_->state != FLAC__FILE_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__seekable_stream_encoder_set_loose_mid_side_stereo(encoder->private_->seekable_stream_encoder, value);
}

FLAC__bool FLAC__file_encoder_set_channels(FLAC__FileEncoder *encoder, unsigned value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->seekable_stream_encoder);
	if(encoder->protected_->state != FLAC__FILE_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__seekable_stream_encoder_set_channels(encoder->private_->seekable_stream_encoder, value);
}

FLAC__bool FLAC__file_encoder_set_bits_per_sample(FLAC__FileEncoder *encoder, unsigned value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->seekable_stream_encoder);
	if(encoder->protected_->state != FLAC__FILE_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__seekable_stream_encoder_set_bits_per_sample(encoder->private_->seekable_stream_encoder, value);
}

FLAC__bool FLAC__file_encoder_set_sample_rate(FLAC__FileEncoder *encoder, unsigned value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->seekable_stream_encoder);
	if(encoder->protected_->state != FLAC__FILE_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__seekable_stream_encoder_set_sample_rate(encoder->private_->seekable_stream_encoder, value);
}

FLAC__bool FLAC__file_encoder_set_blocksize(FLAC__FileEncoder *encoder, unsigned value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->seekable_stream_encoder);
	if(encoder->protected_->state != FLAC__FILE_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__seekable_stream_encoder_set_blocksize(encoder->private_->seekable_stream_encoder, value);
}

FLAC__bool FLAC__file_encoder_set_max_lpc_order(FLAC__FileEncoder *encoder, unsigned value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->seekable_stream_encoder);
	if(encoder->protected_->state != FLAC__FILE_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__seekable_stream_encoder_set_max_lpc_order(encoder->private_->seekable_stream_encoder, value);
}

FLAC__bool FLAC__file_encoder_set_qlp_coeff_precision(FLAC__FileEncoder *encoder, unsigned value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->seekable_stream_encoder);
	if(encoder->protected_->state != FLAC__FILE_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__seekable_stream_encoder_set_qlp_coeff_precision(encoder->private_->seekable_stream_encoder, value);
}

FLAC__bool FLAC__file_encoder_set_do_qlp_coeff_prec_search(FLAC__FileEncoder *encoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->seekable_stream_encoder);
	if(encoder->protected_->state != FLAC__FILE_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__seekable_stream_encoder_set_do_qlp_coeff_prec_search(encoder->private_->seekable_stream_encoder, value);
}

FLAC__bool FLAC__file_encoder_set_do_escape_coding(FLAC__FileEncoder *encoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->seekable_stream_encoder);
	if(encoder->protected_->state != FLAC__FILE_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__seekable_stream_encoder_set_do_escape_coding(encoder->private_->seekable_stream_encoder, value);
}

FLAC__bool FLAC__file_encoder_set_do_exhaustive_model_search(FLAC__FileEncoder *encoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->seekable_stream_encoder);
	if(encoder->protected_->state != FLAC__FILE_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__seekable_stream_encoder_set_do_exhaustive_model_search(encoder->private_->seekable_stream_encoder, value);
}

FLAC__bool FLAC__file_encoder_set_min_residual_partition_order(FLAC__FileEncoder *encoder, unsigned value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->seekable_stream_encoder);
	if(encoder->protected_->state != FLAC__FILE_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__seekable_stream_encoder_set_min_residual_partition_order(encoder->private_->seekable_stream_encoder, value);
}

FLAC__bool FLAC__file_encoder_set_max_residual_partition_order(FLAC__FileEncoder *encoder, unsigned value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->seekable_stream_encoder);
	if(encoder->protected_->state != FLAC__FILE_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__seekable_stream_encoder_set_max_residual_partition_order(encoder->private_->seekable_stream_encoder, value);
}

FLAC__bool FLAC__file_encoder_set_rice_parameter_search_dist(FLAC__FileEncoder *encoder, unsigned value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->seekable_stream_encoder);
	if(encoder->protected_->state != FLAC__FILE_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__seekable_stream_encoder_set_rice_parameter_search_dist(encoder->private_->seekable_stream_encoder, value);
}

FLAC__bool FLAC__file_encoder_set_total_samples_estimate(FLAC__FileEncoder *encoder, FLAC__uint64 value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->seekable_stream_encoder);
	if(encoder->protected_->state != FLAC__FILE_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__seekable_stream_encoder_set_total_samples_estimate(encoder->private_->seekable_stream_encoder, value);
}

FLAC__bool FLAC__file_encoder_set_metadata(FLAC__FileEncoder *encoder, FLAC__StreamMetadata **metadata, unsigned num_blocks)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != encoder->private_->seekable_stream_encoder);
	if(encoder->protected_->state != FLAC__FILE_ENCODER_UNINITIALIZED)
		return false;
	return FLAC__seekable_stream_encoder_set_metadata(encoder->private_->stream_encoder, value);
}

FLAC__bool FLAC__file_encoder_set_filename(FLAC__FileEncoder *encoder, const char *value)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);
	FLAC__ASSERT(0 != value);
	if(encoder->protected_->state != FLAC__FILE_ENCODER_UNINITIALIZED)
		return false;
	if(0 != encoder->private_->filename) {
		free(encoder->private_->filename);
		encoder->private_->filename = 0;
	}
	if(0 == (encoder->private_->filename = (char*)malloc(strlen(value)+1))) {
		encoder->protected_->state = FLAC__FILE_ENCODER_MEMORY_ALLOCATION_ERROR;
		return false;
	}
	strcpy(encoder->private_->filename, value);
	return true;
}

FLAC__FileEncoderState FLAC__file_encoder_get_state(const FLAC__FileEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->protected_);
	return encoder->protected_->state;
}

FLAC__FileEncoderState FLAC__file_encoder_get_seekable_stream_encoder_state(const FLAC__FileEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return FLAC__seekable_stream_encoder_get_state(encoder->private_->seekable_stream_encoder);
}

FLAC__bool FLAC__file_encoder_get_streamable_subset(const FLAC__FileEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return FLAC__seekable_stream_encoder_get_streamable_subset(encoder->private_->seekable_stream_encoder);
}

FLAC__bool FLAC__file_encoder_get_do_mid_side_stereo(const FLAC__FileEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return FLAC__seekable_stream_encoder_get_do_mid_side_stereo(encoder->private_->seekable_stream_encoder);
}

FLAC__bool FLAC__file_encoder_get_loose_mid_side_stereo(const FLAC__FileEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return FLAC__seekable_stream_encoder_get_loose_mid_side_stereo(encoder->private_->seekable_stream_encoder);
}

unsigned FLAC__file_encoder_get_channels(const FLAC__FileEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return FLAC__seekable_stream_encoder_get_channels(encoder->private_->seekable_stream_encoder);
}

unsigned FLAC__file_encoder_get_bits_per_sample(const FLAC__FileEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return FLAC__seekable_stream_encoder_get_bits_per_sample(encoder->private_->seekable_stream_encoder);
}

unsigned FLAC__file_encoder_get_sample_rate(const FLAC__FileEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return FLAC__seekable_stream_encoder_get_sample_rate(encoder->private_->seekable_stream_encoder);
}

unsigned FLAC__file_encoder_get_blocksize(const FLAC__FileEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return FLAC__seekable_stream_encoder_get_blocksize(encoder->private_->seekable_stream_encoder);
}

unsigned FLAC__file_encoder_get_max_lpc_order(const FLAC__FileEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return FLAC__seekable_stream_encoder_get_max_lpc_order(encoder->private_->seekable_stream_encoder);
}

unsigned FLAC__file_encoder_get_qlp_coeff_precision(const FLAC__FileEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return FLAC__seekable_stream_encoder_get_qlp_coeff_precision(encoder->private_->seekable_stream_encoder);
}

FLAC__bool FLAC__file_encoder_get_do_qlp_coeff_prec_search(const FLAC__FileEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return FLAC__seekable_stream_encoder_get_do_qlp_coeff_prec_search(encoder->private_->seekable_stream_encoder);
}

FLAC__bool FLAC__file_encoder_get_do_escape_coding(const FLAC__FileEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return FLAC__seekable_stream_encoder_get_do_escape_coding(encoder->private_->seekable_stream_encoder);
}

FLAC__bool FLAC__file_encoder_get_do_exhaustive_model_search(const FLAC__FileEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return FLAC__seekable_stream_encoder_get_do_exhaustive_model_search(encoder->private_->seekable_stream_encoder);
}

unsigned FLAC__file_encoder_get_min_residual_partition_order(const FLAC__FileEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return FLAC__seekable_stream_encoder_get_min_residual_partition_order(encoder->private_->seekable_stream_encoder);
}

unsigned FLAC__file_encoder_get_max_residual_partition_order(const FLAC__FileEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return FLAC__seekable_stream_encoder_get_max_residual_partition_order(encoder->private_->seekable_stream_encoder);
}

unsigned FLAC__file_encoder_get_rice_parameter_search_dist(const FLAC__FileEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return FLAC__seekable_stream_encoder_get_rice_parameter_search_dist(encoder->private_->seekable_stream_encoder);
}

FLAC__bool FLAC__file_encoder_process(FLAC__FileEncoder *encoder, const FLAC__int32 * const buffer[], unsigned samples)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return FLAC__seekable_stream_encoder_process(encoder->private_->seekable_stream_encoder, buffer, samples);
}

/* 'samples' is channel-wide samples, e.g. for 1 second at 44100Hz, 'samples' = 44100 regardless of the number of channels */
FLAC__bool FLAC__file_encoder_process_interleaved(FLAC__FileEncoder *encoder, const FLAC__int32 buffer[], unsigned samples)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	return FLAC__seekable_stream_encoder_process_interleaved(encoder->private_->seekable_stream_encoder, buffer, samples);
}

/***********************************************************************
 *
 * Private class methods
 *
 ***********************************************************************/

void set_defaults_(FLAC__FileEncoder *encoder)
{
	FLAC__ASSERT(0 != encoder);
	FLAC__ASSERT(0 != encoder->private_);
	FLAC__ASSERT(0 != encoder->protected_);

	encoder->protected_->filename = 0;
}

FLAC__StreamEncoderWriteStatus file_encoder_write_callback_(const FLAC__StreamEncoder *encoder, const FLAC__byte buffer[], unsigned bytes, unsigned samples, unsigned current_frame, void *client_data)
{
	FLAC__FileEncoder *file_encoder = (FLAC__FileEncoder*)client_data;

	/* mark the current seek point if hit (if stream_offset == 0 that means we're still writing metadata and haven't hit the first frame yet) */
	if(file_encoder->private_->stream_offset > 0 && file_encoder->private_->seek_table.num_points > 0) {
		FLAC__uint64 current_sample = (FLAC__uint64)current_frame * (FLAC__uint64)FLAC__stream_encoder_get_blocksize(encoder), test_sample;
		unsigned i;
		for(i = file_encoder->private_->first_seek_point_to_check; i < file_encoder->private_->seek_table.num_points; i++) {
			test_sample = file_encoder->private_->seek_table.points[i].sample_number;
			if(test_sample > current_sample) {
				break;
			}
			else if(test_sample == current_sample) {
				file_encoder->private_->seek_table.points[i].stream_offset = file_encoder->private_->bytes_written - file_encoder->private_->stream_offset;
				file_encoder->private_->seek_table.points[i].frame_samples = FLAC__stream_encoder_get_blocksize(encoder);
				file_encoder->private_->first_seek_point_to_check++;
				break;
			}
			else {
				file_encoder->private_->first_seek_point_to_check++;
			}
		}
	}

	file_encoder->private_->bytes_written += bytes;
	file_encoder->private_->samples_written += samples;
	file_encoder->private_->current_frame = current_frame;

	if(samples && file_encoder->private_->verbose && file_encoder->private_->total_samples_to_encode > 0 && !(current_frame & mask))
		print_stats(file_encoder->private_);

	if(file_encoder->private_->verify) {
		file_encoder->private_->verify_fifo.encoded_signal = buffer;
		file_encoder->private_->verify_fifo.encoded_bytes = bytes;
		if(file_encoder->private_->verify_fifo.into_frames) {
			if(!FLAC__stream_decoder_process_one_frame(file_encoder->private_->verify_fifo.decoder)) {
				file_encoder->private_->verify_fifo.result = FLAC__VERIFY_FAILED_IN_FRAME;
				return FLAC__STREAM_ENCODER_WRITE_FATAL_ERROR;
			}
		}
		else {
			if(!FLAC__stream_decoder_process_metadata(file_encoder->private_->verify_fifo.decoder)) {
				file_encoder->private_->verify_fifo.result = FLAC__VERIFY_FAILED_IN_METADATA;
				return FLAC__STREAM_ENCODER_WRITE_FATAL_ERROR;
			}
		}
	}

#ifdef FLAC__HAS_OGG
	if(file_encoder->private_->use_ogg) {
		ogg_packet op;

		memset(&op, 0, sizeof(op));
		op.packet = (unsigned char *)buffer;
		op.granulepos = file_encoder->private_->samples_written - 1;
		/*@@@ WATCHOUT:
		 * this depends on the behavior of libFLAC that we will get one
		 * write_callback first with all the metadata (and 'samples'
		 * will be 0), then one write_callback for each frame.
		 */
		op.packetno = (samples == 0? -1 : (int)file_encoder->private_->current_frame);
		op.bytes = bytes;

		if (file_encoder->private_->bytes_written == bytes)
			op.b_o_s = 1;

		if (file_encoder->private_->total_samples_to_encode == file_encoder->private_->samples_written)
			op.e_o_s = 1;

		ogg_stream_packetin(&file_encoder->private_->ogg.os, &op);

		while(ogg_stream_pageout(&file_encoder->private_->ogg.os, &file_encoder->private_->ogg.og) != 0) {
			int written;
			written = fwrite(file_encoder->private_->ogg.og.header, 1, file_encoder->private_->ogg.og.header_len, file_encoder->private_->fout);
			if (written != file_encoder->private_->ogg.og.header_len)
				return FLAC__STREAM_ENCODER_WRITE_FATAL_ERROR;

			written = fwrite(file_encoder->private_->ogg.og.body, 1, file_encoder->private_->ogg.og.body_len, file_encoder->private_->fout);
			if (written != file_encoder->private_->ogg.og.body_len)
				return FLAC__STREAM_ENCODER_WRITE_FATAL_ERROR;
		}

		return FLAC__STREAM_ENCODER_WRITE_OK;
	}
	else
#endif
	{
		if(fwrite(buffer, sizeof(FLAC__byte), bytes, file_encoder->private_->fout) == bytes)
			return FLAC__STREAM_ENCODER_WRITE_OK;
		else
			return FLAC__STREAM_ENCODER_WRITE_FATAL_ERROR;
	}
}

void file_encoder_metadata_callback_(const FLAC__StreamEncoder *encoder, const FLAC__StreamMetadata *metadata, void *client_data)
{
	file_encoder->private_ *file_encoder->private_ = (file_encoder->private_ *)client_data;
	FLAC__byte b;
	FILE *f = file_encoder->private_->fout;
	const FLAC__uint64 samples = metadata->data.stream_info.total_samples;
	const unsigned min_framesize = metadata->data.stream_info.min_framesize;
	const unsigned max_framesize = metadata->data.stream_info.max_framesize;

	FLAC__ASSERT(metadata->type == FLAC__METADATA_TYPE_STREAMINFO);

	/*
	 * If we are writing to an ogg stream, there is no need to go back
	 * and update the STREAMINFO or SEEKTABLE blocks; the values we would
	 * update are not necessary with Ogg as the transport.  We can't do
	 * it reliably anyway without knowing the Ogg structure.
	 */
#ifdef FLAC__HAS_OGG
	if(file_encoder->private_->use_ogg)
		return;
#endif

	/*
	 * we get called by the encoder when the encoding process has
	 * finished so that we can update the STREAMINFO and SEEKTABLE
	 * blocks.
	 */

	(void)encoder; /* silence compiler warning about unused parameter */

	if(f != stdout) {
		fclose(file_encoder->private_->fout);
		if(0 == (f = fopen(file_encoder->private_->outfilename, "r+b")))
			return;
	}

	/* all this is based on intimate knowledge of the stream header
	 * layout, but a change to the header format that would break this
	 * would also break all streams encoded in the previous format.
	 */

	if(-1 == fseek(f, 26, SEEK_SET)) goto end_;
	fwrite(metadata->data.stream_info.md5sum, 1, 16, f);

	/* if we get this far we know we can seek so no need to check the
	 * return value from fseek()
	 */
	fseek(f, 21, SEEK_SET);
	if(fread(&b, 1, 1, f) != 1) goto framesize_;
	fseek(f, 21, SEEK_SET);
	b = (b & 0xf0) | (FLAC__byte)((samples >> 32) & 0x0F);
	if(fwrite(&b, 1, 1, f) != 1) goto framesize_;
	b = (FLAC__byte)((samples >> 24) & 0xFF);
	if(fwrite(&b, 1, 1, f) != 1) goto framesize_;
	b = (FLAC__byte)((samples >> 16) & 0xFF);
	if(fwrite(&b, 1, 1, f) != 1) goto framesize_;
	b = (FLAC__byte)((samples >> 8) & 0xFF);
	if(fwrite(&b, 1, 1, f) != 1) goto framesize_;
	b = (FLAC__byte)(samples & 0xFF);
	if(fwrite(&b, 1, 1, f) != 1) goto framesize_;

framesize_:
	fseek(f, 12, SEEK_SET);
	b = (FLAC__byte)((min_framesize >> 16) & 0xFF);
	if(fwrite(&b, 1, 1, f) != 1) goto seektable_;
	b = (FLAC__byte)((min_framesize >> 8) & 0xFF);
	if(fwrite(&b, 1, 1, f) != 1) goto seektable_;
	b = (FLAC__byte)(min_framesize & 0xFF);
	if(fwrite(&b, 1, 1, f) != 1) goto seektable_;
	b = (FLAC__byte)((max_framesize >> 16) & 0xFF);
	if(fwrite(&b, 1, 1, f) != 1) goto seektable_;
	b = (FLAC__byte)((max_framesize >> 8) & 0xFF);
	if(fwrite(&b, 1, 1, f) != 1) goto seektable_;
	b = (FLAC__byte)(max_framesize & 0xFF);
	if(fwrite(&b, 1, 1, f) != 1) goto seektable_;

seektable_:
	if(file_encoder->private_->seek_table.num_points > 0) {
		long pos;
		unsigned i;

		/* convert any unused seek points to placeholders */
		for(i = 0; i < file_encoder->private_->seek_table.num_points; i++) {
			if(file_encoder->private_->seek_table.points[i].sample_number == FLAC__STREAM_METADATA_SEEKPOINT_PLACEHOLDER)
				break;
			else if(file_encoder->private_->seek_table.points[i].frame_samples == 0)
				file_encoder->private_->seek_table.points[i].sample_number = FLAC__STREAM_METADATA_SEEKPOINT_PLACEHOLDER;
		}

		/* the offset of the seek table data 'pos' should be after then stream sync and STREAMINFO block and SEEKTABLE header */
		pos = (FLAC__STREAM_SYNC_LEN + FLAC__STREAM_METADATA_IS_LAST_LEN + FLAC__STREAM_METADATA_TYPE_LEN + FLAC__STREAM_METADATA_LENGTH_LEN) / 8;
		pos += metadata->length;
		pos += (FLAC__STREAM_METADATA_IS_LAST_LEN + FLAC__STREAM_METADATA_TYPE_LEN + FLAC__STREAM_METADATA_LENGTH_LEN) / 8;
		fseek(f, pos, SEEK_SET);
		for(i = 0; i < file_encoder->private_->seek_table.num_points; i++) {
			if(!write_big_endian_uint64(f, file_encoder->private_->seek_table.points[i].sample_number)) goto end_;
			if(!write_big_endian_uint64(f, file_encoder->private_->seek_table.points[i].stream_offset)) goto end_;
			if(!write_big_endian_uint16(f, (FLAC__uint16)file_encoder->private_->seek_table.points[i].frame_samples)) goto end_;
		}
	}

end_:
	fclose(f);
	return;
}
