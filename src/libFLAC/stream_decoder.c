/* libFLAC - Free Lossless Audio Codec library
 * Copyright (C) 2000,2001  Josh Coalson
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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h> /* for malloc() */
#include <string.h> /* for memset/memcpy() */
#include "FLAC/stream_decoder.h"
#include "private/bitbuffer.h"
#include "private/crc.h"
#include "private/fixed.h"
#include "private/lpc.h"

typedef struct FLAC__StreamDecoderPrivate {
	FLAC__StreamDecoderReadStatus (*read_callback)(const FLAC__StreamDecoder *decoder, byte buffer[], unsigned *bytes, void *client_data);
	FLAC__StreamDecoderWriteStatus (*write_callback)(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const int32 *buffer[], void *client_data);
	void (*metadata_callback)(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetaData *metadata, void *client_data);
	void (*error_callback)(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data);
	void *client_data;
	FLAC__BitBuffer input;
	int32 *output[FLAC__MAX_CHANNELS];
	int32 *residual[FLAC__MAX_CHANNELS];
	unsigned output_capacity, output_channels;
	uint32 last_frame_number;
	uint64 samples_decoded;
	bool has_stream_info, has_seek_table;
	FLAC__StreamMetaData stream_info;
	FLAC__StreamMetaData seek_table;
	FLAC__Frame frame;
	byte header_warmup[2]; /* contains the sync code and reserved bits */
	byte lookahead; /* temp storage when we need to look ahead one byte in the stream */
	bool cached; /* true if there is a byte in lookahead */
} FLAC__StreamDecoderPrivate;

static byte ID3V2_TAG_[3] = { 'I', 'D', '3' };

static bool stream_decoder_allocate_output_(FLAC__StreamDecoder *decoder, unsigned size, unsigned channels);
static bool stream_decoder_find_metadata_(FLAC__StreamDecoder *decoder);
static bool stream_decoder_read_metadata_(FLAC__StreamDecoder *decoder);
static bool stream_decoder_skip_id3v2_tag_(FLAC__StreamDecoder *decoder);
static bool stream_decoder_frame_sync_(FLAC__StreamDecoder *decoder);
static bool stream_decoder_read_frame_(FLAC__StreamDecoder *decoder, bool *got_a_frame);
static bool stream_decoder_read_frame_header_(FLAC__StreamDecoder *decoder);
static bool stream_decoder_read_subframe_(FLAC__StreamDecoder *decoder, unsigned channel, unsigned bps);
static bool stream_decoder_read_subframe_constant_(FLAC__StreamDecoder *decoder, unsigned channel, unsigned bps);
static bool stream_decoder_read_subframe_fixed_(FLAC__StreamDecoder *decoder, unsigned channel, unsigned bps, const unsigned order);
static bool stream_decoder_read_subframe_lpc_(FLAC__StreamDecoder *decoder, unsigned channel, unsigned bps, const unsigned order);
static bool stream_decoder_read_subframe_verbatim_(FLAC__StreamDecoder *decoder, unsigned channel, unsigned bps);
static bool stream_decoder_read_residual_partitioned_rice_(FLAC__StreamDecoder *decoder, unsigned predictor_order, unsigned partition_order, int32 *residual);
static bool stream_decoder_read_zero_padding_(FLAC__StreamDecoder *decoder);
static bool read_callback_(byte buffer[], unsigned *bytes, void *client_data);

const char *FLAC__StreamDecoderStateString[] = {
	"FLAC__STREAM_DECODER_SEARCH_FOR_METADATA",
	"FLAC__STREAM_DECODER_READ_METADATA",
	"FLAC__STREAM_DECODER_SEARCH_FOR_FRAME_SYNC",
	"FLAC__STREAM_DECODER_READ_FRAME",
	"FLAC__STREAM_DECODER_END_OF_STREAM",
	"FLAC__STREAM_DECODER_ABORTED",
	"FLAC__STREAM_DECODER_UNPARSEABLE_STREAM",
	"FLAC__STREAM_DECODER_MEMORY_ALLOCATION_ERROR",
	"FLAC__STREAM_DECODER_UNINITIALIZED"
};

const char *FLAC__StreamDecoderReadStatusString[] = {
	"FLAC__STREAM_DECODER_READ_CONTINUE",
	"FLAC__STREAM_DECODER_READ_END_OF_STREAM",
	"FLAC__STREAM_DECODER_READ_ABORT"
};

const char *FLAC__StreamDecoderWriteStatusString[] = {
	"FLAC__STREAM_DECODER_WRITE_CONTINUE",
	"FLAC__STREAM_DECODER_WRITE_ABORT"
};

const char *FLAC__StreamDecoderErrorStatusString[] = {
	"FLAC__STREAM_DECODER_ERROR_LOST_SYNC",
	"FLAC__STREAM_DECODER_ERROR_BAD_HEADER",
	"FLAC__STREAM_DECODER_ERROR_FRAME_CRC_MISMATCH"
};

FLAC__StreamDecoder *FLAC__stream_decoder_get_new_instance()
{
	FLAC__StreamDecoder *decoder = (FLAC__StreamDecoder*)malloc(sizeof(FLAC__StreamDecoder));
	if(decoder != 0) {
		decoder->state = FLAC__STREAM_DECODER_UNINITIALIZED;
		decoder->guts = 0;
	}
	return decoder;
}

void FLAC__stream_decoder_free_instance(FLAC__StreamDecoder *decoder)
{
	free(decoder);
}

FLAC__StreamDecoderState FLAC__stream_decoder_init(
	FLAC__StreamDecoder *decoder,
	FLAC__StreamDecoderReadStatus (*read_callback)(const FLAC__StreamDecoder *decoder, byte buffer[], unsigned *bytes, void *client_data),
	FLAC__StreamDecoderWriteStatus (*write_callback)(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const int32 *buffer[], void *client_data),
	void (*metadata_callback)(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetaData *metadata, void *client_data),
	void (*error_callback)(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data),
	void *client_data
)
{
	unsigned i;

	assert(sizeof(int) >= 4); /* we want to die right away if this is not true */
	assert(decoder != 0);
	assert(read_callback != 0);
	assert(write_callback != 0);
	assert(metadata_callback != 0);
	assert(error_callback != 0);
	assert(decoder->state == FLAC__STREAM_DECODER_UNINITIALIZED);
	assert(decoder->guts == 0);

	decoder->state = FLAC__STREAM_DECODER_SEARCH_FOR_METADATA;

	decoder->guts = (FLAC__StreamDecoderPrivate*)malloc(sizeof(FLAC__StreamDecoderPrivate));
	if(decoder->guts == 0)
		return decoder->state = FLAC__STREAM_DECODER_MEMORY_ALLOCATION_ERROR;

	decoder->guts->read_callback = read_callback;
	decoder->guts->write_callback = write_callback;
	decoder->guts->metadata_callback = metadata_callback;
	decoder->guts->error_callback = error_callback;
	decoder->guts->client_data = client_data;

	FLAC__bitbuffer_init(&decoder->guts->input);

	for(i = 0; i < FLAC__MAX_CHANNELS; i++) {
		decoder->guts->output[i] = 0;
		decoder->guts->residual[i] = 0;
	}

	decoder->guts->output_capacity = 0;
	decoder->guts->output_channels = 0;
	decoder->guts->last_frame_number = 0;
	decoder->guts->samples_decoded = 0;
	decoder->guts->has_stream_info = false;
	decoder->guts->has_seek_table = false;
	decoder->guts->cached = false;

	return decoder->state;
}

void FLAC__stream_decoder_finish(FLAC__StreamDecoder *decoder)
{
	unsigned i;
	assert(decoder != 0);
	if(decoder->state == FLAC__STREAM_DECODER_UNINITIALIZED)
		return;
	if(decoder->guts != 0) {
		if(decoder->guts->has_seek_table) {
			free(decoder->guts->seek_table.data.seek_table.points);
			decoder->guts->seek_table.data.seek_table.points = 0;
		}
		FLAC__bitbuffer_free(&decoder->guts->input);
		for(i = 0; i < FLAC__MAX_CHANNELS; i++) {
			if(decoder->guts->output[i] != 0) {
				free(decoder->guts->output[i]);
				decoder->guts->output[i] = 0;
			}
			if(decoder->guts->residual[i] != 0) {
				free(decoder->guts->residual[i]);
				decoder->guts->residual[i] = 0;
			}
		}
		free(decoder->guts);
		decoder->guts = 0;
	}
	decoder->state = FLAC__STREAM_DECODER_UNINITIALIZED;
}

bool FLAC__stream_decoder_flush(FLAC__StreamDecoder *decoder)
{
	assert(decoder != 0);

	if(!FLAC__bitbuffer_clear(&decoder->guts->input)) {
		decoder->state = FLAC__STREAM_DECODER_MEMORY_ALLOCATION_ERROR;
		return false;
	}

	return true;
}

bool FLAC__stream_decoder_reset(FLAC__StreamDecoder *decoder)
{
	assert(decoder != 0);

	if(!FLAC__stream_decoder_flush(decoder)) {
		decoder->state = FLAC__STREAM_DECODER_MEMORY_ALLOCATION_ERROR;
		return false;
	}
	decoder->state = FLAC__STREAM_DECODER_SEARCH_FOR_METADATA;

	decoder->guts->samples_decoded = 0;

	return true;
}

bool FLAC__stream_decoder_process_whole_stream(FLAC__StreamDecoder *decoder)
{
	bool dummy;
	assert(decoder != 0);

	if(decoder->state == FLAC__STREAM_DECODER_END_OF_STREAM)
		return true;

	assert(decoder->state == FLAC__STREAM_DECODER_SEARCH_FOR_METADATA);

	if(!FLAC__stream_decoder_reset(decoder)) {
		decoder->state = FLAC__STREAM_DECODER_MEMORY_ALLOCATION_ERROR;
		return false;
	}

	while(1) {
		switch(decoder->state) {
			case FLAC__STREAM_DECODER_SEARCH_FOR_METADATA:
				if(!stream_decoder_find_metadata_(decoder))
					return false; /* above function sets the status for us */
				break;
			case FLAC__STREAM_DECODER_READ_METADATA:
				if(!stream_decoder_read_metadata_(decoder))
					return false; /* above function sets the status for us */
				break;
			case FLAC__STREAM_DECODER_SEARCH_FOR_FRAME_SYNC:
				if(!stream_decoder_frame_sync_(decoder))
					return true; /* above function sets the status for us */
				break;
			case FLAC__STREAM_DECODER_READ_FRAME:
				if(!stream_decoder_read_frame_(decoder, &dummy))
					return false; /* above function sets the status for us */
				break;
			case FLAC__STREAM_DECODER_END_OF_STREAM:
				return true;
			default:
				assert(0);
		}
	}
}

bool FLAC__stream_decoder_process_metadata(FLAC__StreamDecoder *decoder)
{
	assert(decoder != 0);

	if(decoder->state == FLAC__STREAM_DECODER_END_OF_STREAM)
		return true;

	assert(decoder->state == FLAC__STREAM_DECODER_SEARCH_FOR_METADATA);

	if(!FLAC__stream_decoder_reset(decoder)) {
		decoder->state = FLAC__STREAM_DECODER_MEMORY_ALLOCATION_ERROR;
		return false;
	}

	while(1) {
		switch(decoder->state) {
			case FLAC__STREAM_DECODER_SEARCH_FOR_METADATA:
				if(!stream_decoder_find_metadata_(decoder))
					return false; /* above function sets the status for us */
				break;
			case FLAC__STREAM_DECODER_READ_METADATA:
				if(!stream_decoder_read_metadata_(decoder))
					return false; /* above function sets the status for us */
				break;
			case FLAC__STREAM_DECODER_SEARCH_FOR_FRAME_SYNC:
				return true;
				break;
			case FLAC__STREAM_DECODER_END_OF_STREAM:
				return true;
			default:
				assert(0);
		}
	}
}

bool FLAC__stream_decoder_process_one_frame(FLAC__StreamDecoder *decoder)
{
	bool got_a_frame;
	assert(decoder != 0);

	if(decoder->state == FLAC__STREAM_DECODER_END_OF_STREAM)
		return true;

	assert(decoder->state == FLAC__STREAM_DECODER_SEARCH_FOR_FRAME_SYNC);

	while(1) {
		switch(decoder->state) {
			case FLAC__STREAM_DECODER_SEARCH_FOR_FRAME_SYNC:
				if(!stream_decoder_frame_sync_(decoder))
					return true; /* above function sets the status for us */
				break;
			case FLAC__STREAM_DECODER_READ_FRAME:
				if(!stream_decoder_read_frame_(decoder, &got_a_frame))
					return false; /* above function sets the status for us */
				if(got_a_frame)
					return true; /* above function sets the status for us */
				break;
			case FLAC__STREAM_DECODER_END_OF_STREAM:
				return true;
			default:
				assert(0);
		}
	}
}

bool FLAC__stream_decoder_process_remaining_frames(FLAC__StreamDecoder *decoder)
{
	bool dummy;
	assert(decoder != 0);

	if(decoder->state == FLAC__STREAM_DECODER_END_OF_STREAM)
		return true;

	assert(decoder->state == FLAC__STREAM_DECODER_SEARCH_FOR_FRAME_SYNC);

	while(1) {
		switch(decoder->state) {
			case FLAC__STREAM_DECODER_SEARCH_FOR_FRAME_SYNC:
				if(!stream_decoder_frame_sync_(decoder))
					return true; /* above function sets the status for us */
				break;
			case FLAC__STREAM_DECODER_READ_FRAME:
				if(!stream_decoder_read_frame_(decoder, &dummy))
					return false; /* above function sets the status for us */
				break;
			case FLAC__STREAM_DECODER_END_OF_STREAM:
				return true;
			default:
				assert(0);
		}
	}
}

unsigned FLAC__stream_decoder_input_bytes_unconsumed(FLAC__StreamDecoder *decoder)
{
	assert(decoder != 0);
	return decoder->guts->input.bytes - decoder->guts->input.consumed_bytes;
}

bool stream_decoder_allocate_output_(FLAC__StreamDecoder *decoder, unsigned size, unsigned channels)
{
	unsigned i;
	int32 *tmp;

	if(size <= decoder->guts->output_capacity && channels <= decoder->guts->output_channels)
		return true;

	/* @@@ should change to use realloc() */

	for(i = 0; i < FLAC__MAX_CHANNELS; i++) {
		if(decoder->guts->output[i] != 0) {
			free(decoder->guts->output[i]);
			decoder->guts->output[i] = 0;
		}
		if(decoder->guts->residual[i] != 0) {
			free(decoder->guts->residual[i]);
			decoder->guts->residual[i] = 0;
		}
	}

	for(i = 0; i < channels; i++) {
		tmp = (int32*)malloc(sizeof(int32)*size);
		if(tmp == 0) {
			decoder->state = FLAC__STREAM_DECODER_MEMORY_ALLOCATION_ERROR;
			return false;
		}
		decoder->guts->output[i] = tmp;

		tmp = (int32*)malloc(sizeof(int32)*size);
		if(tmp == 0) {
			decoder->state = FLAC__STREAM_DECODER_MEMORY_ALLOCATION_ERROR;
			return false;
		}
		decoder->guts->residual[i] = tmp;
	}

	decoder->guts->output_capacity = size;
	decoder->guts->output_channels = channels;

	return true;
}

bool stream_decoder_find_metadata_(FLAC__StreamDecoder *decoder)
{
	uint32 x;
	unsigned i, id;
	bool first = true;

	assert(decoder->guts->input.consumed_bits == 0); /* make sure we're byte aligned */

	for(i = id = 0; i < 4; ) {
		if(decoder->guts->cached) {
			x = (uint32)decoder->guts->lookahead;
			decoder->guts->cached = false;
		}
		else {
			if(!FLAC__bitbuffer_read_raw_uint32(&decoder->guts->input, &x, 8, read_callback_, decoder))
				return false; /* the read_callback_ sets the state for us */
		}
		if(x == FLAC__STREAM_SYNC_STRING[i]) {
			first = true;
			i++;
			id = 0;
			continue;
		}
		if(x == ID3V2_TAG_[id]) {
			id++;
			i = 0;
			if(id == 3) {
				if(!stream_decoder_skip_id3v2_tag_(decoder))
					return false; /* the read_callback_ sets the state for us */
			}
			continue;
		}
		if(x == 0xff) { /* MAGIC NUMBER for the first 8 frame sync bits */
			decoder->guts->header_warmup[0] = (byte)x;
			if(!FLAC__bitbuffer_read_raw_uint32(&decoder->guts->input, &x, 8, read_callback_, decoder))
				return false; /* the read_callback_ sets the state for us */

			/* we have to check if we just read two 0xff's in a row; the second may actually be the beginning of the sync code */
			/* else we have to check if the second byte is the end of a sync code */
			if(x == 0xff) { /* MAGIC NUMBER for the first 8 frame sync bits */
				decoder->guts->lookahead = (byte)x;
				decoder->guts->cached = true;
			}
			else if(x >> 2 == 0x3e) { /* MAGIC NUMBER for the last 6 sync bits */
				decoder->guts->header_warmup[1] = (byte)x;
				decoder->state = FLAC__STREAM_DECODER_READ_FRAME;
				return true;
			}
		}
		i = 0;
		if(first) {
			decoder->guts->error_callback(decoder, FLAC__STREAM_DECODER_ERROR_LOST_SYNC, decoder->guts->client_data);
			first = false;
		}
	}

	decoder->state = FLAC__STREAM_DECODER_READ_METADATA;
	return true;
}

bool stream_decoder_read_metadata_(FLAC__StreamDecoder *decoder)
{
	uint32 i, x, last_block, type, length;
	uint64 xx;

	assert(decoder->guts->input.consumed_bits == 0); /* make sure we're byte aligned */

	if(!FLAC__bitbuffer_read_raw_uint32(&decoder->guts->input, &last_block, FLAC__STREAM_METADATA_IS_LAST_LEN, read_callback_, decoder))
		return false; /* the read_callback_ sets the state for us */
	if(!FLAC__bitbuffer_read_raw_uint32(&decoder->guts->input, &type, FLAC__STREAM_METADATA_TYPE_LEN, read_callback_, decoder))
		return false; /* the read_callback_ sets the state for us */
	if(!FLAC__bitbuffer_read_raw_uint32(&decoder->guts->input, &length, FLAC__STREAM_METADATA_LENGTH_LEN, read_callback_, decoder))
		return false; /* the read_callback_ sets the state for us */
	if(type == FLAC__METADATA_TYPE_STREAMINFO) {
		unsigned used_bits = 0;
		decoder->guts->stream_info.type = type;
		decoder->guts->stream_info.is_last = last_block;
		decoder->guts->stream_info.length = length;

		if(!FLAC__bitbuffer_read_raw_uint32(&decoder->guts->input, &x, FLAC__STREAM_METADATA_STREAMINFO_MIN_BLOCK_SIZE_LEN, read_callback_, decoder))
			return false; /* the read_callback_ sets the state for us */
		decoder->guts->stream_info.data.stream_info.min_blocksize = x;
		used_bits += FLAC__STREAM_METADATA_STREAMINFO_MIN_BLOCK_SIZE_LEN;

		if(!FLAC__bitbuffer_read_raw_uint32(&decoder->guts->input, &x, FLAC__STREAM_METADATA_STREAMINFO_MAX_BLOCK_SIZE_LEN, read_callback_, decoder))
			return false; /* the read_callback_ sets the state for us */
		decoder->guts->stream_info.data.stream_info.max_blocksize = x;
		used_bits += FLAC__STREAM_METADATA_STREAMINFO_MAX_BLOCK_SIZE_LEN;

		if(!FLAC__bitbuffer_read_raw_uint32(&decoder->guts->input, &x, FLAC__STREAM_METADATA_STREAMINFO_MIN_FRAME_SIZE_LEN, read_callback_, decoder))
			return false; /* the read_callback_ sets the state for us */
		decoder->guts->stream_info.data.stream_info.min_framesize = x;
		used_bits += FLAC__STREAM_METADATA_STREAMINFO_MIN_FRAME_SIZE_LEN;

		if(!FLAC__bitbuffer_read_raw_uint32(&decoder->guts->input, &x, FLAC__STREAM_METADATA_STREAMINFO_MAX_FRAME_SIZE_LEN, read_callback_, decoder))
			return false; /* the read_callback_ sets the state for us */
		decoder->guts->stream_info.data.stream_info.max_framesize = x;
		used_bits += FLAC__STREAM_METADATA_STREAMINFO_MAX_FRAME_SIZE_LEN;

		if(!FLAC__bitbuffer_read_raw_uint32(&decoder->guts->input, &x, FLAC__STREAM_METADATA_STREAMINFO_SAMPLE_RATE_LEN, read_callback_, decoder))
			return false; /* the read_callback_ sets the state for us */
		decoder->guts->stream_info.data.stream_info.sample_rate = x;
		used_bits += FLAC__STREAM_METADATA_STREAMINFO_SAMPLE_RATE_LEN;

		if(!FLAC__bitbuffer_read_raw_uint32(&decoder->guts->input, &x, FLAC__STREAM_METADATA_STREAMINFO_CHANNELS_LEN, read_callback_, decoder))
			return false; /* the read_callback_ sets the state for us */
		decoder->guts->stream_info.data.stream_info.channels = x+1;
		used_bits += FLAC__STREAM_METADATA_STREAMINFO_CHANNELS_LEN;

		if(!FLAC__bitbuffer_read_raw_uint32(&decoder->guts->input, &x, FLAC__STREAM_METADATA_STREAMINFO_BITS_PER_SAMPLE_LEN, read_callback_, decoder))
			return false; /* the read_callback_ sets the state for us */
		decoder->guts->stream_info.data.stream_info.bits_per_sample = x+1;
		used_bits += FLAC__STREAM_METADATA_STREAMINFO_BITS_PER_SAMPLE_LEN;

		if(!FLAC__bitbuffer_read_raw_uint64(&decoder->guts->input, &decoder->guts->stream_info.data.stream_info.total_samples, FLAC__STREAM_METADATA_STREAMINFO_TOTAL_SAMPLES_LEN, read_callback_, decoder))
			return false; /* the read_callback_ sets the state for us */
		used_bits += FLAC__STREAM_METADATA_STREAMINFO_TOTAL_SAMPLES_LEN;

		for(i = 0; i < 16; i++) {
			if(!FLAC__bitbuffer_read_raw_uint32(&decoder->guts->input, &x, 8, read_callback_, decoder))
				return false; /* the read_callback_ sets the state for us */
			decoder->guts->stream_info.data.stream_info.md5sum[i] = (byte)x;
		}
		used_bits += i*8;

		/* skip the rest of the block */
		assert(used_bits % 8 == 0);
		length -= (used_bits / 8);
		for(i = 0; i < length; i++) {
			if(!FLAC__bitbuffer_read_raw_uint32(&decoder->guts->input, &x, 8, read_callback_, decoder))
				return false; /* the read_callback_ sets the state for us */
		}

		decoder->guts->has_stream_info = true;
		decoder->guts->metadata_callback(decoder, &decoder->guts->stream_info, decoder->guts->client_data);
	}
	else if(type == FLAC__METADATA_TYPE_SEEKTABLE) {
		unsigned real_points;

		decoder->guts->seek_table.type = type;
		decoder->guts->seek_table.is_last = last_block;
		decoder->guts->seek_table.length = length;

		decoder->guts->seek_table.data.seek_table.num_points = length / FLAC__STREAM_METADATA_SEEKPOINT_LEN;

		if(0 == (decoder->guts->seek_table.data.seek_table.points = (FLAC__StreamMetaData_SeekPoint*)malloc(decoder->guts->seek_table.data.seek_table.num_points * sizeof(FLAC__StreamMetaData_SeekPoint)))) {
			decoder->state = FLAC__STREAM_DECODER_MEMORY_ALLOCATION_ERROR;
			return false;
		}
		for(i = real_points = 0; i < decoder->guts->seek_table.data.seek_table.num_points; i++) {
			if(!FLAC__bitbuffer_read_raw_uint64(&decoder->guts->input, &xx, FLAC__STREAM_METADATA_SEEKPOINT_SAMPLE_NUMBER_LEN, read_callback_, decoder))
				return false; /* the read_callback_ sets the state for us */
			decoder->guts->seek_table.data.seek_table.points[real_points].sample_number = xx;

			if(!FLAC__bitbuffer_read_raw_uint64(&decoder->guts->input, &xx, FLAC__STREAM_METADATA_SEEKPOINT_STREAM_OFFSET_LEN, read_callback_, decoder))
				return false; /* the read_callback_ sets the state for us */
			decoder->guts->seek_table.data.seek_table.points[real_points].stream_offset = xx;

			if(!FLAC__bitbuffer_read_raw_uint32(&decoder->guts->input, &x, FLAC__STREAM_METADATA_SEEKPOINT_FRAME_SAMPLES_LEN, read_callback_, decoder))
				return false; /* the read_callback_ sets the state for us */
			decoder->guts->seek_table.data.seek_table.points[real_points].frame_samples = x;

			if(decoder->guts->seek_table.data.seek_table.points[real_points].sample_number != FLAC__STREAM_METADATA_SEEKPOINT_PLACEHOLDER)
				real_points++;
		}
		decoder->guts->seek_table.data.seek_table.num_points = real_points;

		decoder->guts->has_seek_table = true;
		decoder->guts->metadata_callback(decoder, &decoder->guts->seek_table, decoder->guts->client_data);
	}
	else {
		/* skip other metadata blocks */
		for(i = 0; i < length; i++) {
			if(!FLAC__bitbuffer_read_raw_uint32(&decoder->guts->input, &x, 8, read_callback_, decoder))
				return false; /* the read_callback_ sets the state for us */
		}
	}

	if(last_block)
		decoder->state = FLAC__STREAM_DECODER_SEARCH_FOR_FRAME_SYNC;

	return true;
}

bool stream_decoder_skip_id3v2_tag_(FLAC__StreamDecoder *decoder)
{
	uint32 x;
	unsigned i, skip;

	/* skip the version and flags bytes */
	if(!FLAC__bitbuffer_read_raw_uint32(&decoder->guts->input, &x, 24, read_callback_, decoder))
		return false; /* the read_callback_ sets the state for us */
	/* get the size (in bytes) to skip */
	skip = 0;
	for(i = 0; i < 4; i++) {
		if(!FLAC__bitbuffer_read_raw_uint32(&decoder->guts->input, &x, 8, read_callback_, decoder))
			return false; /* the read_callback_ sets the state for us */
		skip <<= 7;
		skip |= (x & 0x7f);
	}
	/* skip the rest of the tag */
	for(i = 0; i < skip; i++) {
		if(!FLAC__bitbuffer_read_raw_uint32(&decoder->guts->input, &x, 8, read_callback_, decoder))
			return false; /* the read_callback_ sets the state for us */
	}
	return true;
}

bool stream_decoder_frame_sync_(FLAC__StreamDecoder *decoder)
{
	uint32 x;
	bool first = true;

	/* If we know the total number of samples in the stream, stop if we've read that many. */
	/* This will stop us, for example, from wasting time trying to sync on an ID3V1 tag. */
	if(decoder->guts->has_stream_info && decoder->guts->stream_info.data.stream_info.total_samples) {
		if(decoder->guts->samples_decoded >= decoder->guts->stream_info.data.stream_info.total_samples) {
			decoder->state = FLAC__STREAM_DECODER_END_OF_STREAM;
			return true;
		}
	}

	/* make sure we're byte aligned */
	if(decoder->guts->input.consumed_bits != 0) {
		if(!FLAC__bitbuffer_read_raw_uint32(&decoder->guts->input, &x, 8-decoder->guts->input.consumed_bits, read_callback_, decoder))
			return false; /* the read_callback_ sets the state for us */
	}

	while(1) {
		if(decoder->guts->cached) {
			x = (uint32)decoder->guts->lookahead;
			decoder->guts->cached = false;
		}
		else {
			if(!FLAC__bitbuffer_read_raw_uint32(&decoder->guts->input, &x, 8, read_callback_, decoder))
				return false; /* the read_callback_ sets the state for us */
		}
		if(x == 0xff) { /* MAGIC NUMBER for the first 8 frame sync bits */
			decoder->guts->header_warmup[0] = (byte)x;
			if(!FLAC__bitbuffer_read_raw_uint32(&decoder->guts->input, &x, 8, read_callback_, decoder))
				return false; /* the read_callback_ sets the state for us */

			/* we have to check if we just read two 0xff's in a row; the second may actually be the beginning of the sync code */
			/* else we have to check if the second byte is the end of a sync code */
			if(x == 0xff) { /* MAGIC NUMBER for the first 8 frame sync bits */
				decoder->guts->lookahead = (byte)x;
				decoder->guts->cached = true;
			}
			else if(x >> 2 == 0x3e) { /* MAGIC NUMBER for the last 6 sync bits */
				decoder->guts->header_warmup[1] = (byte)x;
				decoder->state = FLAC__STREAM_DECODER_READ_FRAME;
				return true;
			}
		}
		if(first) {
			decoder->guts->error_callback(decoder, FLAC__STREAM_DECODER_ERROR_LOST_SYNC, decoder->guts->client_data);
			first = 0;
		}
	}

	return true;
}

bool stream_decoder_read_frame_(FLAC__StreamDecoder *decoder, bool *got_a_frame)
{
	unsigned channel;
	unsigned i;
	int32 mid, side, left, right;
	uint16 frame_crc; /* the one we calculate from the input stream */
	uint32 x;

	*got_a_frame = false;

	/* init the CRC */
	frame_crc = 0;
	FLAC__CRC16_UPDATE(decoder->guts->header_warmup[0], frame_crc);
	FLAC__CRC16_UPDATE(decoder->guts->header_warmup[1], frame_crc);
	FLAC__bitbuffer_init_read_crc16(&decoder->guts->input, frame_crc);

	if(!stream_decoder_read_frame_header_(decoder))
		return false;
	if(decoder->state == FLAC__STREAM_DECODER_SEARCH_FOR_FRAME_SYNC)
		return true;
	if(!stream_decoder_allocate_output_(decoder, decoder->guts->frame.header.blocksize, decoder->guts->frame.header.channels))
		return false;
	for(channel = 0; channel < decoder->guts->frame.header.channels; channel++) {
		/*
		 * first figure the correct bits-per-sample of the subframe
		 */
		unsigned bps = decoder->guts->frame.header.bits_per_sample;
		switch(decoder->guts->frame.header.channel_assignment) {
			case FLAC__CHANNEL_ASSIGNMENT_INDEPENDENT:
				/* no adjustment needed */
				break;
			case FLAC__CHANNEL_ASSIGNMENT_LEFT_SIDE:
				assert(decoder->guts->frame.header.channels == 2);
				if(channel == 1)
					bps++;
				break;
			case FLAC__CHANNEL_ASSIGNMENT_RIGHT_SIDE:
				assert(decoder->guts->frame.header.channels == 2);
				if(channel == 0)
					bps++;
				break;
			case FLAC__CHANNEL_ASSIGNMENT_MID_SIDE:
				assert(decoder->guts->frame.header.channels == 2);
				if(channel == 1)
					bps++;
				break;
			default:
				assert(0);
		}
		/*
		 * now read it
		 */
		if(!stream_decoder_read_subframe_(decoder, channel, bps))
			return false;
		if(decoder->state != FLAC__STREAM_DECODER_READ_FRAME) {
			decoder->state = FLAC__STREAM_DECODER_SEARCH_FOR_FRAME_SYNC;
			return true;
		}
	}
	if(!stream_decoder_read_zero_padding_(decoder))
		return false;

	/*
	 * Read the frame CRC-16 from the footer and check
	 */
	frame_crc = decoder->guts->input.read_crc16;
	if(!FLAC__bitbuffer_read_raw_uint32(&decoder->guts->input, &x, FLAC__FRAME_FOOTER_CRC_LEN, read_callback_, decoder))
		return false; /* the read_callback_ sets the state for us */
	if(frame_crc == (uint16)x) {
		/* Undo any special channel coding */
		switch(decoder->guts->frame.header.channel_assignment) {
			case FLAC__CHANNEL_ASSIGNMENT_INDEPENDENT:
				/* do nothing */
				break;
			case FLAC__CHANNEL_ASSIGNMENT_LEFT_SIDE:
				assert(decoder->guts->frame.header.channels == 2);
				for(i = 0; i < decoder->guts->frame.header.blocksize; i++)
					decoder->guts->output[1][i] = decoder->guts->output[0][i] - decoder->guts->output[1][i];
				break;
			case FLAC__CHANNEL_ASSIGNMENT_RIGHT_SIDE:
				assert(decoder->guts->frame.header.channels == 2);
				for(i = 0; i < decoder->guts->frame.header.blocksize; i++)
					decoder->guts->output[0][i] += decoder->guts->output[1][i];
				break;
			case FLAC__CHANNEL_ASSIGNMENT_MID_SIDE:
				assert(decoder->guts->frame.header.channels == 2);
				for(i = 0; i < decoder->guts->frame.header.blocksize; i++) {
					mid = decoder->guts->output[0][i];
					side = decoder->guts->output[1][i];
					mid <<= 1;
					if(side & 1) /* i.e. if 'side' is odd... */
						mid++;
					left = mid + side;
					right = mid - side;
					decoder->guts->output[0][i] = left >> 1;
					decoder->guts->output[1][i] = right >> 1;
				}
				break;
			default:
				assert(0);
				break;
		}
	}
	else {
		/* Bad frame, emit error and zero the output signal */
		decoder->guts->error_callback(decoder, FLAC__STREAM_DECODER_ERROR_FRAME_CRC_MISMATCH, decoder->guts->client_data);
		for(channel = 0; channel < decoder->guts->frame.header.channels; channel++) {
			memset(decoder->guts->output[channel], 0, sizeof(int32) * decoder->guts->frame.header.blocksize);
		}
	}

	*got_a_frame = true;

	/* put the latest values into the public section of the decoder instance */
	decoder->channels = decoder->guts->frame.header.channels;
	decoder->channel_assignment = decoder->guts->frame.header.channel_assignment;
	decoder->bits_per_sample = decoder->guts->frame.header.bits_per_sample;
	decoder->sample_rate = decoder->guts->frame.header.sample_rate;
	decoder->blocksize = decoder->guts->frame.header.blocksize;

	decoder->guts->samples_decoded = decoder->guts->frame.header.number.sample_number + decoder->guts->frame.header.blocksize;

	/* write it */
	/* NOTE: some versions of GCC can't figure out const-ness right and will give you an 'incompatible pointer type' warning on arg 3 here: */
	if(decoder->guts->write_callback(decoder, &decoder->guts->frame, decoder->guts->output, decoder->guts->client_data) != FLAC__STREAM_DECODER_WRITE_CONTINUE)
		return false;

	decoder->state = FLAC__STREAM_DECODER_SEARCH_FOR_FRAME_SYNC;
	return true;
}

bool stream_decoder_read_frame_header_(FLAC__StreamDecoder *decoder)
{
	uint32 x;
	uint64 xx;
	unsigned i, blocksize_hint = 0, sample_rate_hint = 0;
	byte crc8, raw_header[16]; /* MAGIC NUMBER based on the maximum frame header size, including CRC */
	unsigned raw_header_len;
	bool is_unparseable = false;
	const bool is_known_variable_blocksize_stream = (decoder->guts->has_stream_info && decoder->guts->stream_info.data.stream_info.min_blocksize != decoder->guts->stream_info.data.stream_info.max_blocksize);
	const bool is_known_fixed_blocksize_stream = (decoder->guts->has_stream_info && decoder->guts->stream_info.data.stream_info.min_blocksize == decoder->guts->stream_info.data.stream_info.max_blocksize);

	assert(decoder->guts->input.consumed_bits == 0); /* make sure we're byte aligned */

	/* init the raw header with the saved bits from synchronization */
	raw_header[0] = decoder->guts->header_warmup[0];
	raw_header[1] = decoder->guts->header_warmup[1];
	raw_header_len = 2;

	/*
	 * check to make sure that the reserved bits are 0
	 */
	if(raw_header[1] & 0x03) { /* MAGIC NUMBER */
		is_unparseable = true;
	}

	/*
	 * Note that along the way as we read the header, we look for a sync
	 * code inside.  If we find one it would indicate that our original
	 * sync was bad since there cannot be a sync code in a valid header.
	 */

	/*
	 * read in the raw header as bytes so we can CRC it, and parse it on the way
	 */
	for(i = 0; i < 2; i++) {
		if(!FLAC__bitbuffer_read_raw_uint32(&decoder->guts->input, &x, 8, read_callback_, decoder))
			return false; /* the read_callback_ sets the state for us */
		if(x == 0xff) { /* MAGIC NUMBER for the first 8 frame sync bits */
			/* if we get here it means our original sync was erroneous since the sync code cannot appear in the header */
			decoder->guts->lookahead = (byte)x;
			decoder->guts->cached = true;
			decoder->guts->error_callback(decoder, FLAC__STREAM_DECODER_ERROR_BAD_HEADER, decoder->guts->client_data);
			decoder->state = FLAC__STREAM_DECODER_SEARCH_FOR_FRAME_SYNC;
			return true;
		}
		raw_header[raw_header_len++] = (byte)x;
	}

	switch(x = raw_header[2] >> 4) {
		case 0:
			if(is_known_fixed_blocksize_stream)
				decoder->guts->frame.header.blocksize = decoder->guts->stream_info.data.stream_info.min_blocksize;
			else
				is_unparseable = true;
			break;
		case 1:
			decoder->guts->frame.header.blocksize = 192;
			break;
		case 2:
		case 3:
		case 4:
		case 5:
			decoder->guts->frame.header.blocksize = 576 << (x-2);
			break;
		case 6:
		case 7:
			blocksize_hint = x;
			break;
		case 8:
		case 9:
		case 10:
		case 11:
		case 12:
		case 13:
		case 14:
		case 15:
			decoder->guts->frame.header.blocksize = 256 << (x-8);
			break;
		default:
			assert(0);
			break;
	}

	switch(x = raw_header[2] & 0x0f) {
		case 0:
			if(decoder->guts->has_stream_info)
				decoder->guts->frame.header.sample_rate = decoder->guts->stream_info.data.stream_info.sample_rate;
			else
				is_unparseable = true;
			break;
		case 1:
		case 2:
		case 3:
			is_unparseable = true;
			break;
		case 4:
			decoder->guts->frame.header.sample_rate = 8000;
			break;
		case 5:
			decoder->guts->frame.header.sample_rate = 16000;
			break;
		case 6:
			decoder->guts->frame.header.sample_rate = 22050;
			break;
		case 7:
			decoder->guts->frame.header.sample_rate = 24000;
			break;
		case 8:
			decoder->guts->frame.header.sample_rate = 32000;
			break;
		case 9:
			decoder->guts->frame.header.sample_rate = 44100;
			break;
		case 10:
			decoder->guts->frame.header.sample_rate = 48000;
			break;
		case 11:
			decoder->guts->frame.header.sample_rate = 96000;
			break;
		case 12:
		case 13:
		case 14:
			sample_rate_hint = x;
			break;
		case 15:
			decoder->guts->error_callback(decoder, FLAC__STREAM_DECODER_ERROR_BAD_HEADER, decoder->guts->client_data);
			decoder->state = FLAC__STREAM_DECODER_SEARCH_FOR_FRAME_SYNC;
			return true;
		default:
			assert(0);
	}

	x = (unsigned)(raw_header[3] >> 4);
	if(x & 8) {
		decoder->guts->frame.header.channels = 2;
		switch(x & 7) {
			case 0:
				decoder->guts->frame.header.channel_assignment = FLAC__CHANNEL_ASSIGNMENT_LEFT_SIDE;
				break;
			case 1:
				decoder->guts->frame.header.channel_assignment = FLAC__CHANNEL_ASSIGNMENT_RIGHT_SIDE;
				break;
			case 2:
				decoder->guts->frame.header.channel_assignment = FLAC__CHANNEL_ASSIGNMENT_MID_SIDE;
				break;
			default:
				is_unparseable = true;
				break;
		}
	}
	else {
		decoder->guts->frame.header.channels = (unsigned)x + 1;
		decoder->guts->frame.header.channel_assignment = FLAC__CHANNEL_ASSIGNMENT_INDEPENDENT;
	}

	switch(x = (unsigned)(raw_header[3] & 0x0e) >> 1) {
		case 0:
			if(decoder->guts->has_stream_info)
				decoder->guts->frame.header.bits_per_sample = decoder->guts->stream_info.data.stream_info.bits_per_sample;
			else
				is_unparseable = true;
			break;
		case 1:
			decoder->guts->frame.header.bits_per_sample = 8;
			break;
		case 2:
			decoder->guts->frame.header.bits_per_sample = 12;
			break;
		case 4:
			decoder->guts->frame.header.bits_per_sample = 16;
			break;
		case 5:
			decoder->guts->frame.header.bits_per_sample = 20;
			break;
		case 6:
			decoder->guts->frame.header.bits_per_sample = 24;
			break;
		case 3:
		case 7:
			is_unparseable = true;
			break;
		default:
			assert(0);
			break;
	}

	if(raw_header[3] & 0x01) { /* this should be a zero padding bit */
		decoder->guts->error_callback(decoder, FLAC__STREAM_DECODER_ERROR_BAD_HEADER, decoder->guts->client_data);
		decoder->state = FLAC__STREAM_DECODER_SEARCH_FOR_FRAME_SYNC;
		return true;
	}

	if(blocksize_hint && is_known_variable_blocksize_stream) {
		if(!FLAC__bitbuffer_read_utf8_uint64(&decoder->guts->input, &xx, read_callback_, decoder, raw_header, &raw_header_len))
			return false; /* the read_callback_ sets the state for us */
		if(xx == 0xffffffffffffffff) { /* i.e. non-UTF8 code... */
			decoder->guts->lookahead = raw_header[raw_header_len-1]; /* back up as much as we can */
			decoder->guts->cached = true;
			decoder->guts->error_callback(decoder, FLAC__STREAM_DECODER_ERROR_BAD_HEADER, decoder->guts->client_data);
			decoder->state = FLAC__STREAM_DECODER_SEARCH_FOR_FRAME_SYNC;
			return true;
		}
		decoder->guts->frame.header.number.sample_number = xx;
	}
	else {
		if(!FLAC__bitbuffer_read_utf8_uint32(&decoder->guts->input, &x, read_callback_, decoder, raw_header, &raw_header_len))
			return false; /* the read_callback_ sets the state for us */
		if(x == 0xffffffff) { /* i.e. non-UTF8 code... */
			decoder->guts->lookahead = raw_header[raw_header_len-1]; /* back up as much as we can */
			decoder->guts->cached = true;
			decoder->guts->error_callback(decoder, FLAC__STREAM_DECODER_ERROR_BAD_HEADER, decoder->guts->client_data);
			decoder->state = FLAC__STREAM_DECODER_SEARCH_FOR_FRAME_SYNC;
			return true;
		}
		decoder->guts->last_frame_number = x;
		if(decoder->guts->has_stream_info) {
			decoder->guts->frame.header.number.sample_number = (int64)decoder->guts->stream_info.data.stream_info.min_blocksize * (int64)x;
		}
		else {
			is_unparseable = true;
		}
	}

	if(blocksize_hint) {
		if(!FLAC__bitbuffer_read_raw_uint32(&decoder->guts->input, &x, 8, read_callback_, decoder))
			return false; /* the read_callback_ sets the state for us */
		raw_header[raw_header_len++] = (byte)x;
		if(blocksize_hint == 7) {
			uint32 _x;
			if(!FLAC__bitbuffer_read_raw_uint32(&decoder->guts->input, &_x, 8, read_callback_, decoder))
				return false; /* the read_callback_ sets the state for us */
			raw_header[raw_header_len++] = (byte)_x;
			x = (x << 8) | _x;
		}
		decoder->guts->frame.header.blocksize = x+1;
	}

	if(sample_rate_hint) {
		if(!FLAC__bitbuffer_read_raw_uint32(&decoder->guts->input, &x, 8, read_callback_, decoder))
			return false; /* the read_callback_ sets the state for us */
		raw_header[raw_header_len++] = (byte)x;
		if(sample_rate_hint != 12) {
			uint32 _x;
			if(!FLAC__bitbuffer_read_raw_uint32(&decoder->guts->input, &_x, 8, read_callback_, decoder))
				return false; /* the read_callback_ sets the state for us */
			raw_header[raw_header_len++] = (byte)_x;
			x = (x << 8) | _x;
		}
		if(sample_rate_hint == 12)
			decoder->guts->frame.header.sample_rate = x*1000;
		else if(sample_rate_hint == 13)
			decoder->guts->frame.header.sample_rate = x;
		else
			decoder->guts->frame.header.sample_rate = x*10;
	}

	/* read the CRC-8 byte */
	if(!FLAC__bitbuffer_read_raw_uint32(&decoder->guts->input, &x, 8, read_callback_, decoder))
		return false; /* the read_callback_ sets the state for us */
	crc8 = (byte)x;

	if(FLAC__crc8(raw_header, raw_header_len) != crc8) {
		decoder->guts->error_callback(decoder, FLAC__STREAM_DECODER_ERROR_BAD_HEADER, decoder->guts->client_data);
		decoder->state = FLAC__STREAM_DECODER_SEARCH_FOR_FRAME_SYNC;
		return true;
	}

	if(is_unparseable) {
		decoder->state = FLAC__STREAM_DECODER_UNPARSEABLE_STREAM;
		return false;
	}

	return true;
}

bool stream_decoder_read_subframe_(FLAC__StreamDecoder *decoder, unsigned channel, unsigned bps)
{
	uint32 x;
	bool wasted_bits;

	if(!FLAC__bitbuffer_read_raw_uint32(&decoder->guts->input, &x, 8, read_callback_, decoder)) /* MAGIC NUMBER */
		return false; /* the read_callback_ sets the state for us */

	wasted_bits = (x & 1);
	x &= 0xfe;

	if(wasted_bits) {
		unsigned u;
		if(!FLAC__bitbuffer_read_unary_unsigned(&decoder->guts->input, &u, read_callback_, decoder))
			return false; /* the read_callback_ sets the state for us */
		decoder->guts->frame.subframes[channel].wasted_bits = u+1;
		bps -= decoder->guts->frame.subframes[channel].wasted_bits;
	}
	else
		decoder->guts->frame.subframes[channel].wasted_bits = 0;

	/*
	 * Lots of magic numbers here
	 */
	if(x & 0x80) {
		decoder->guts->error_callback(decoder, FLAC__STREAM_DECODER_ERROR_LOST_SYNC, decoder->guts->client_data);
		decoder->state = FLAC__STREAM_DECODER_SEARCH_FOR_FRAME_SYNC;
		return true;
	}
	else if(x == 0) {
		if(!stream_decoder_read_subframe_constant_(decoder, channel, bps))
			return false;
	}
	else if(x == 2) {
		if(!stream_decoder_read_subframe_verbatim_(decoder, channel, bps))
			return false;
	}
	else if(x < 16) {
		decoder->state = FLAC__STREAM_DECODER_UNPARSEABLE_STREAM;
		return false;
	}
	else if(x <= 24) {
		if(!stream_decoder_read_subframe_fixed_(decoder, channel, bps, (x>>1)&7))
			return false;
	}
	else if(x < 64) {
		decoder->state = FLAC__STREAM_DECODER_UNPARSEABLE_STREAM;
		return false;
	}
	else {
		if(!stream_decoder_read_subframe_lpc_(decoder, channel, bps, ((x>>1)&31)+1))
			return false;
	}

	if(wasted_bits) {
		unsigned i;
		x = decoder->guts->frame.subframes[channel].wasted_bits;
		for(i = 0; i < decoder->guts->frame.header.blocksize; i++)
			decoder->guts->output[channel][i] <<= x;
	}

	return true;
}

bool stream_decoder_read_subframe_constant_(FLAC__StreamDecoder *decoder, unsigned channel, unsigned bps)
{
	FLAC__Subframe_Constant *subframe = &decoder->guts->frame.subframes[channel].data.constant;
	int32 x;
	unsigned i;
	int32 *output = decoder->guts->output[channel];

	decoder->guts->frame.subframes[channel].type = FLAC__SUBFRAME_TYPE_CONSTANT;

	if(!FLAC__bitbuffer_read_raw_int32(&decoder->guts->input, &x, bps, read_callback_, decoder))
		return false; /* the read_callback_ sets the state for us */

	subframe->value = x;

	/* decode the subframe */
	for(i = 0; i < decoder->guts->frame.header.blocksize; i++)
		output[i] = x;

	return true;
}

bool stream_decoder_read_subframe_fixed_(FLAC__StreamDecoder *decoder, unsigned channel, unsigned bps, const unsigned order)
{
	FLAC__Subframe_Fixed *subframe = &decoder->guts->frame.subframes[channel].data.fixed;
	int32 i32;
	uint32 u32;
	unsigned u;

	decoder->guts->frame.subframes[channel].type = FLAC__SUBFRAME_TYPE_FIXED;

	subframe->residual = decoder->guts->residual[channel];
	subframe->order = order;

	/* read warm-up samples */
	for(u = 0; u < order; u++) {
		if(!FLAC__bitbuffer_read_raw_int32(&decoder->guts->input, &i32, bps, read_callback_, decoder))
			return false; /* the read_callback_ sets the state for us */
		subframe->warmup[u] = i32;
	}

	/* read entropy coding method info */
	if(!FLAC__bitbuffer_read_raw_uint32(&decoder->guts->input, &u32, FLAC__ENTROPY_CODING_METHOD_TYPE_LEN, read_callback_, decoder))
		return false; /* the read_callback_ sets the state for us */
	subframe->entropy_coding_method.type = u32;
	switch(subframe->entropy_coding_method.type) {
		case FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE:
			if(!FLAC__bitbuffer_read_raw_uint32(&decoder->guts->input, &u32, FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_ORDER_LEN, read_callback_, decoder))
				return false; /* the read_callback_ sets the state for us */
			subframe->entropy_coding_method.data.partitioned_rice.order = u32;
			break;
		default:
			decoder->state = FLAC__STREAM_DECODER_UNPARSEABLE_STREAM;
			return false;
	}

	/* read residual */
	switch(subframe->entropy_coding_method.type) {
		case FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE:
			if(!stream_decoder_read_residual_partitioned_rice_(decoder, order, subframe->entropy_coding_method.data.partitioned_rice.order, decoder->guts->residual[channel]))
				return false;
			break;
		default:
			assert(0);
	}

	/* decode the subframe */
	memcpy(decoder->guts->output[channel], subframe->warmup, sizeof(int32) * order);
	FLAC__fixed_restore_signal(decoder->guts->residual[channel], decoder->guts->frame.header.blocksize-order, order, decoder->guts->output[channel]+order);

	return true;
}

bool stream_decoder_read_subframe_lpc_(FLAC__StreamDecoder *decoder, unsigned channel, unsigned bps, const unsigned order)
{
	FLAC__Subframe_LPC *subframe = &decoder->guts->frame.subframes[channel].data.lpc;
	int32 i32;
	uint32 u32;
	unsigned u;

	decoder->guts->frame.subframes[channel].type = FLAC__SUBFRAME_TYPE_LPC;

	subframe->residual = decoder->guts->residual[channel];
	subframe->order = order;

	/* read warm-up samples */
	for(u = 0; u < order; u++) {
		if(!FLAC__bitbuffer_read_raw_int32(&decoder->guts->input, &i32, bps, read_callback_, decoder))
			return false; /* the read_callback_ sets the state for us */
		subframe->warmup[u] = i32;
	}

	/* read qlp coeff precision */
	if(!FLAC__bitbuffer_read_raw_uint32(&decoder->guts->input, &u32, FLAC__SUBFRAME_LPC_QLP_COEFF_PRECISION_LEN, read_callback_, decoder))
		return false; /* the read_callback_ sets the state for us */
	if(u32 == (1u << FLAC__SUBFRAME_LPC_QLP_COEFF_PRECISION_LEN) - 1) {
		decoder->guts->error_callback(decoder, FLAC__STREAM_DECODER_ERROR_LOST_SYNC, decoder->guts->client_data);
		decoder->state = FLAC__STREAM_DECODER_SEARCH_FOR_FRAME_SYNC;
		return true;
	}
	subframe->qlp_coeff_precision = u32+1;

	/* read qlp shift */
	if(!FLAC__bitbuffer_read_raw_int32(&decoder->guts->input, &i32, FLAC__SUBFRAME_LPC_QLP_SHIFT_LEN, read_callback_, decoder))
		return false; /* the read_callback_ sets the state for us */
	subframe->quantization_level = i32;

	/* read quantized lp coefficiencts */
	for(u = 0; u < order; u++) {
		if(!FLAC__bitbuffer_read_raw_int32(&decoder->guts->input, &i32, subframe->qlp_coeff_precision, read_callback_, decoder))
			return false; /* the read_callback_ sets the state for us */
		subframe->qlp_coeff[u] = i32;
	}

	/* read entropy coding method info */
	if(!FLAC__bitbuffer_read_raw_uint32(&decoder->guts->input, &u32, FLAC__ENTROPY_CODING_METHOD_TYPE_LEN, read_callback_, decoder))
		return false; /* the read_callback_ sets the state for us */
	subframe->entropy_coding_method.type = u32;
	switch(subframe->entropy_coding_method.type) {
		case FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE:
			if(!FLAC__bitbuffer_read_raw_uint32(&decoder->guts->input, &u32, FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_ORDER_LEN, read_callback_, decoder))
				return false; /* the read_callback_ sets the state for us */
			subframe->entropy_coding_method.data.partitioned_rice.order = u32;
			break;
		default:
			decoder->state = FLAC__STREAM_DECODER_UNPARSEABLE_STREAM;
			return false;
	}

	/* read residual */
	switch(subframe->entropy_coding_method.type) {
		case FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE:
			if(!stream_decoder_read_residual_partitioned_rice_(decoder, order, subframe->entropy_coding_method.data.partitioned_rice.order, decoder->guts->residual[channel]))
				return false;
			break;
		default:
			assert(0);
	}

	/* decode the subframe */
	memcpy(decoder->guts->output[channel], subframe->warmup, sizeof(int32) * order);
	FLAC__lpc_restore_signal(decoder->guts->residual[channel], decoder->guts->frame.header.blocksize-order, subframe->qlp_coeff, order, subframe->quantization_level, decoder->guts->output[channel]+order);

	return true;
}

bool stream_decoder_read_subframe_verbatim_(FLAC__StreamDecoder *decoder, unsigned channel, unsigned bps)
{
	FLAC__Subframe_Verbatim *subframe = &decoder->guts->frame.subframes[channel].data.verbatim;
	int32 x, *residual = decoder->guts->residual[channel];
	unsigned i;

	decoder->guts->frame.subframes[channel].type = FLAC__SUBFRAME_TYPE_VERBATIM;

	subframe->data = residual;

	for(i = 0; i < decoder->guts->frame.header.blocksize; i++) {
		if(!FLAC__bitbuffer_read_raw_int32(&decoder->guts->input, &x, bps, read_callback_, decoder))
			return false; /* the read_callback_ sets the state for us */
		residual[i] = x;
	}

	/* decode the subframe */
	memcpy(decoder->guts->output[channel], subframe->data, sizeof(int32) * decoder->guts->frame.header.blocksize);

	return true;
}

bool stream_decoder_read_residual_partitioned_rice_(FLAC__StreamDecoder *decoder, unsigned predictor_order, unsigned partition_order, int32 *residual)
{
	uint32 rice_parameter;
	int i;
	unsigned partition, sample, u;
	const unsigned partitions = 1u << partition_order;
	const unsigned partition_samples = partition_order > 0? decoder->guts->frame.header.blocksize >> partition_order : decoder->guts->frame.header.blocksize - predictor_order;

	sample = 0;
	for(partition = 0; partition < partitions; partition++) {
		if(!FLAC__bitbuffer_read_raw_uint32(&decoder->guts->input, &rice_parameter, FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_PARAMETER_LEN, read_callback_, decoder))
			return false; /* the read_callback_ sets the state for us */
		if(rice_parameter < FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_ESCAPE_PARAMETER) {
			for(u = (partition_order == 0 || partition > 0)? 0 : predictor_order; u < partition_samples; u++, sample++) {
#ifdef FLAC__SYMMETRIC_RICE
				if(!FLAC__bitbuffer_read_symmetric_rice_signed(&decoder->guts->input, &i, rice_parameter, read_callback_, decoder))
					return false; /* the read_callback_ sets the state for us */
#else
				if(!FLAC__bitbuffer_read_rice_signed(&decoder->guts->input, &i, rice_parameter, read_callback_, decoder))
					return false; /* the read_callback_ sets the state for us */
#endif
				residual[sample] = i;
			}
		}
		else {
			if(!FLAC__bitbuffer_read_raw_uint32(&decoder->guts->input, &rice_parameter, FLAC__ENTROPY_CODING_METHOD_PARTITIONED_RICE_RAW_LEN, read_callback_, decoder))
				return false; /* the read_callback_ sets the state for us */
			for(u = (partition_order == 0 || partition > 0)? 0 : predictor_order; u < partition_samples; u++, sample++) {
				if(!FLAC__bitbuffer_read_raw_int32(&decoder->guts->input, &i, rice_parameter, read_callback_, decoder))
					return false; /* the read_callback_ sets the state for us */
				residual[sample] = i;
			}
		}
	}

	return true;
}

bool stream_decoder_read_zero_padding_(FLAC__StreamDecoder *decoder)
{
	if(decoder->guts->input.consumed_bits != 0) {
		uint32 zero = 0;
		if(!FLAC__bitbuffer_read_raw_uint32(&decoder->guts->input, &zero, 8-decoder->guts->input.consumed_bits, read_callback_, decoder))
			return false; /* the read_callback_ sets the state for us */
		if(zero != 0) {
			decoder->guts->error_callback(decoder, FLAC__STREAM_DECODER_ERROR_LOST_SYNC, decoder->guts->client_data);
			decoder->state = FLAC__STREAM_DECODER_SEARCH_FOR_FRAME_SYNC;
		}
	}
	return true;
}

bool read_callback_(byte buffer[], unsigned *bytes, void *client_data)
{
	FLAC__StreamDecoder *decoder = (FLAC__StreamDecoder *)client_data;
	FLAC__StreamDecoderReadStatus status;
	status = decoder->guts->read_callback(decoder, buffer, bytes, decoder->guts->client_data);
	if(status == FLAC__STREAM_DECODER_READ_END_OF_STREAM)
		decoder->state = FLAC__STREAM_DECODER_END_OF_STREAM;
	else if(status == FLAC__STREAM_DECODER_READ_ABORT)
		decoder->state = FLAC__STREAM_DECODER_ABORTED;
	return status == FLAC__STREAM_DECODER_READ_CONTINUE;
}
