/* libFLAC - Free Lossless Audio Coder library
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
	unsigned output_capacity;
	uint32 last_frame_number;
	uint64 samples_decoded;
	bool has_stream_header;
	FLAC__StreamMetaData stream_header;
	FLAC__Frame frame;
} FLAC__StreamDecoderPrivate;

static byte ID3V2_TAG_[3] = { 'I', 'D', '3' };

static bool stream_decoder_allocate_output_(FLAC__StreamDecoder *decoder, unsigned size);
static bool stream_decoder_find_metadata_(FLAC__StreamDecoder *decoder);
static bool stream_decoder_read_metadata_(FLAC__StreamDecoder *decoder);
static bool stream_decoder_skip_id3v2_tag_(FLAC__StreamDecoder *decoder);
static bool stream_decoder_frame_sync_(FLAC__StreamDecoder *decoder);
static bool stream_decoder_read_frame_(FLAC__StreamDecoder *decoder, bool *got_a_frame);
static bool stream_decoder_read_frame_header_(FLAC__StreamDecoder *decoder);
static bool stream_decoder_read_subframe_(FLAC__StreamDecoder *decoder, unsigned channel);
static bool stream_decoder_read_subframe_constant_(FLAC__StreamDecoder *decoder, unsigned channel);
static bool stream_decoder_read_subframe_fixed_(FLAC__StreamDecoder *decoder, unsigned channel, const unsigned order);
static bool stream_decoder_read_subframe_lpc_(FLAC__StreamDecoder *decoder, unsigned channel, const unsigned order);
static bool stream_decoder_read_subframe_verbatim_(FLAC__StreamDecoder *decoder, unsigned channel);
static bool stream_decoder_read_residual_partitioned_rice_(FLAC__StreamDecoder *decoder, unsigned predictor_order, unsigned partition_order, int32 *residual);
static bool stream_decoder_read_zero_padding_(FLAC__StreamDecoder *decoder);
static bool read_callback_(byte buffer[], unsigned *bytes, void *client_data);

const char *FLAC__StreamDecoderStateString[] = {
	"FLAC__STREAM_DECODER_SEARCH_FOR_METADATA",
	"FLAC__STREAM_DECODER_READ_METADATA",
	"FLAC__STREAM_DECODER_SEARCH_FOR_FRAME_SYNC",
	"FLAC__STREAM_DECODER_READ_FRAME",
	"FLAC__STREAM_DECODER_RESYNC_IN_HEADER",
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
	"FLAC__STREAM_DECODER_ERROR_LOST_SYNC"
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
	decoder->guts->last_frame_number = 0;
	decoder->guts->samples_decoded = 0;
	decoder->guts->has_stream_header = false;

	return decoder->state;
}

void FLAC__stream_decoder_finish(FLAC__StreamDecoder *decoder)
{
	unsigned i;
	assert(decoder != 0);
	if(decoder->state == FLAC__STREAM_DECODER_UNINITIALIZED)
		return;
	if(decoder->guts != 0) {
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

bool stream_decoder_allocate_output_(FLAC__StreamDecoder *decoder, unsigned size)
{
	unsigned i;
	int32 *tmp;

	if(size <= decoder->guts->output_capacity)
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

	for(i = 0; i < decoder->guts->frame.header.channels; i++) {
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

	return true;
}

bool stream_decoder_find_metadata_(FLAC__StreamDecoder *decoder)
{
	uint32 x;
	unsigned i, id;
	bool first = 1;

	assert(decoder->guts->input.consumed_bits == 0); /* make sure we're byte aligned */

	for(i = id = 0; i < 4; ) {
		if(!FLAC__bitbuffer_read_raw_uint32(&decoder->guts->input, &x, 8, read_callback_, decoder))
			return false; /* the read_callback_ sets the state for us */
		if(x == FLAC__STREAM_SYNC_STRING[i]) {
			first = 1;
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
			unsigned y;
			if(!FLAC__bitbuffer_peek_bit(&decoder->guts->input, &y, read_callback_, decoder))
				return false; /* the read_callback_ sets the state for us */
			if(!y) { /* MAGIC NUMBER for the last sync bit */
				decoder->state = FLAC__STREAM_DECODER_READ_FRAME;
				return true;
			}
		}
		i = 0;
		if(first) {
			decoder->guts->error_callback(decoder, FLAC__STREAM_DECODER_ERROR_LOST_SYNC, decoder->guts->client_data);
			first = 0;
		}
	}

	decoder->state = FLAC__STREAM_DECODER_READ_METADATA;
	return true;
}

bool stream_decoder_read_metadata_(FLAC__StreamDecoder *decoder)
{
	uint32 i, x, last_block, type, length;

	assert(decoder->guts->input.consumed_bits == 0); /* make sure we're byte aligned */

	if(!FLAC__bitbuffer_read_raw_uint32(&decoder->guts->input, &last_block, FLAC__STREAM_METADATA_IS_LAST_LEN, read_callback_, decoder))
		return false; /* the read_callback_ sets the state for us */
	if(!FLAC__bitbuffer_read_raw_uint32(&decoder->guts->input, &type, FLAC__STREAM_METADATA_TYPE_LEN, read_callback_, decoder))
		return false; /* the read_callback_ sets the state for us */
	if(!FLAC__bitbuffer_read_raw_uint32(&decoder->guts->input, &length, FLAC__STREAM_METADATA_LENGTH_LEN, read_callback_, decoder))
		return false; /* the read_callback_ sets the state for us */
	if(type == FLAC__METADATA_TYPE_ENCODING) {
		unsigned used_bits = 0;
		decoder->guts->stream_header.type = type;
		decoder->guts->stream_header.is_last = last_block;
		decoder->guts->stream_header.length = length;

		if(!FLAC__bitbuffer_read_raw_uint32(&decoder->guts->input, &x, FLAC__STREAM_METADATA_ENCODING_MIN_BLOCK_SIZE_LEN, read_callback_, decoder))
			return false; /* the read_callback_ sets the state for us */
		decoder->guts->stream_header.data.encoding.min_blocksize = x;
		used_bits += FLAC__STREAM_METADATA_ENCODING_MIN_BLOCK_SIZE_LEN;

		if(!FLAC__bitbuffer_read_raw_uint32(&decoder->guts->input, &x, FLAC__STREAM_METADATA_ENCODING_MAX_BLOCK_SIZE_LEN, read_callback_, decoder))
			return false; /* the read_callback_ sets the state for us */
		decoder->guts->stream_header.data.encoding.max_blocksize = x;
		used_bits += FLAC__STREAM_METADATA_ENCODING_MAX_BLOCK_SIZE_LEN;

		if(!FLAC__bitbuffer_read_raw_uint32(&decoder->guts->input, &x, FLAC__STREAM_METADATA_ENCODING_MIN_FRAME_SIZE_LEN, read_callback_, decoder))
			return false; /* the read_callback_ sets the state for us */
		decoder->guts->stream_header.data.encoding.min_framesize = x;
		used_bits += FLAC__STREAM_METADATA_ENCODING_MIN_FRAME_SIZE_LEN;

		if(!FLAC__bitbuffer_read_raw_uint32(&decoder->guts->input, &x, FLAC__STREAM_METADATA_ENCODING_MAX_FRAME_SIZE_LEN, read_callback_, decoder))
			return false; /* the read_callback_ sets the state for us */
		decoder->guts->stream_header.data.encoding.max_framesize = x;
		used_bits += FLAC__STREAM_METADATA_ENCODING_MAX_FRAME_SIZE_LEN;

		if(!FLAC__bitbuffer_read_raw_uint32(&decoder->guts->input, &x, FLAC__STREAM_METADATA_ENCODING_SAMPLE_RATE_LEN, read_callback_, decoder))
			return false; /* the read_callback_ sets the state for us */
		decoder->guts->stream_header.data.encoding.sample_rate = x;
		used_bits += FLAC__STREAM_METADATA_ENCODING_SAMPLE_RATE_LEN;

		if(!FLAC__bitbuffer_read_raw_uint32(&decoder->guts->input, &x, FLAC__STREAM_METADATA_ENCODING_CHANNELS_LEN, read_callback_, decoder))
			return false; /* the read_callback_ sets the state for us */
		decoder->guts->stream_header.data.encoding.channels = x+1;
		used_bits += FLAC__STREAM_METADATA_ENCODING_CHANNELS_LEN;

		if(!FLAC__bitbuffer_read_raw_uint32(&decoder->guts->input, &x, FLAC__STREAM_METADATA_ENCODING_BITS_PER_SAMPLE_LEN, read_callback_, decoder))
			return false; /* the read_callback_ sets the state for us */
		decoder->guts->stream_header.data.encoding.bits_per_sample = x+1;
		used_bits += FLAC__STREAM_METADATA_ENCODING_BITS_PER_SAMPLE_LEN;

		if(!FLAC__bitbuffer_read_raw_uint64(&decoder->guts->input, &decoder->guts->stream_header.data.encoding.total_samples, FLAC__STREAM_METADATA_ENCODING_TOTAL_SAMPLES_LEN, read_callback_, decoder))
			return false; /* the read_callback_ sets the state for us */
		used_bits += FLAC__STREAM_METADATA_ENCODING_TOTAL_SAMPLES_LEN;

		for(i = 0; i < 16; i++) {
			if(!FLAC__bitbuffer_read_raw_uint32(&decoder->guts->input, &x, 8, read_callback_, decoder))
				return false; /* the read_callback_ sets the state for us */
			decoder->guts->stream_header.data.encoding.md5sum[i] = (byte)x;
		}
		used_bits += 128;

		/* skip the rest of the block */
		assert(used_bits % 8 == 0);
		length -= (used_bits / 8);
		for(i = 0; i < length; i++) {
			if(!FLAC__bitbuffer_read_raw_uint32(&decoder->guts->input, &x, 8, read_callback_, decoder))
				return false; /* the read_callback_ sets the state for us */
		}

		decoder->guts->has_stream_header = true;
		decoder->guts->metadata_callback(decoder, &decoder->guts->stream_header, decoder->guts->client_data);
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
	bool first = 1;

	/* If we know the total number of samples in the stream, stop if we've read that many. */
	/* This will stop us, for example, from wasting time trying to sync on an ID3V1 tag. */
	if(decoder->guts->has_stream_header && decoder->guts->stream_header.data.encoding.total_samples) {
		if(decoder->guts->samples_decoded >= decoder->guts->stream_header.data.encoding.total_samples) {
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
		if(!FLAC__bitbuffer_read_raw_uint32(&decoder->guts->input, &x, 8, read_callback_, decoder))
			return false; /* the read_callback_ sets the state for us */
		if(x == 0xff) { /* MAGIC NUMBER for the first 8 frame sync bits */
			unsigned y;
			if(!FLAC__bitbuffer_peek_bit(&decoder->guts->input, &y, read_callback_, decoder))
				return false; /* the read_callback_ sets the state for us */
			if(!y) { /* MAGIC NUMBER for the last sync bit */
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

	*got_a_frame = false;

	if(!stream_decoder_read_frame_header_(decoder))
		return false;
	if(decoder->state != FLAC__STREAM_DECODER_READ_FRAME) {
		if(decoder->state == FLAC__STREAM_DECODER_RESYNC_IN_HEADER)
			decoder->state = FLAC__STREAM_DECODER_READ_FRAME;
		else
			decoder->state = FLAC__STREAM_DECODER_SEARCH_FOR_FRAME_SYNC;
		return true;
	}
	if(!stream_decoder_allocate_output_(decoder, decoder->guts->frame.header.blocksize))
		return false;
	for(channel = 0; channel < decoder->guts->frame.header.channels; channel++) {
		if(!stream_decoder_read_subframe_(decoder, channel))
			return false;
		if(decoder->state != FLAC__STREAM_DECODER_READ_FRAME) {
			decoder->state = FLAC__STREAM_DECODER_SEARCH_FOR_FRAME_SYNC;
			return true;
		}
	}
	if(!stream_decoder_read_zero_padding_(decoder))
		return false;

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

	*got_a_frame = true;

	/* put the latest values into the public section of the decoder instance */
	decoder->channels = decoder->guts->frame.header.channels;
	decoder->channel_assignment = decoder->guts->frame.header.channel_assignment;
	decoder->bits_per_sample = decoder->guts->frame.header.bits_per_sample;
	decoder->sample_rate = decoder->guts->frame.header.sample_rate;
	decoder->blocksize = decoder->guts->frame.header.blocksize;

	decoder->guts->samples_decoded += decoder->guts->frame.header.blocksize;

	/* write it */
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
	byte crc, raw_header[15]; /* MAGIC NUMBER based on the maximum frame header size, including CRC */
	unsigned raw_header_len;
	bool is_unparseable = false;

	assert(decoder->guts->input.consumed_bits == 0); /* make sure we're byte aligned */

	/* init the raw header with the first 8 bits of the sync code */
	raw_header[0] = 0xff; /* MAGIC NUMBER for the first 8 frame sync bits */
	raw_header_len = 1;

	/*
	 * read in the raw header as bytes so we can CRC it, and parse it on the way
	 */
	for(i = 0; i < 2; i++) {
		if(!FLAC__bitbuffer_read_raw_uint32(&decoder->guts->input, &x, 8, read_callback_, decoder))
			return false; /* the read_callback_ sets the state for us */
		else if(x == 0xff) { /* MAGIC NUMBER for the first part of the sync code */
			/* if we get here it means our original sync was erroneous since the sync code cannot appear in the header */
			uint32 y;
			if(!FLAC__bitbuffer_peek_bit(&decoder->guts->input, &y, read_callback_, decoder))
				return false; /* the read_callback_ sets the state for us */
			if(!y) { /* MAGIC NUMBER for the last sync bit */
				decoder->state = FLAC__STREAM_DECODER_RESYNC_IN_HEADER;
				return true;
			}
			else {
				decoder->guts->error_callback(decoder, FLAC__STREAM_DECODER_ERROR_LOST_SYNC, decoder->guts->client_data);
				decoder->state = FLAC__STREAM_DECODER_SEARCH_FOR_FRAME_SYNC;
				return true;
			}
		}
		raw_header[raw_header_len++] = (byte)x;
	}
	assert(!(raw_header[1] & 0x80)); /* last sync bit should be confirmed zero before we get here */

	switch(x = raw_header[1] >> 4) {
		case 0:
			if(decoder->guts->has_stream_header && decoder->guts->stream_header.data.encoding.min_blocksize == decoder->guts->stream_header.data.encoding.max_blocksize) /* i.e. it's a fixed-blocksize stream */
				decoder->guts->frame.header.blocksize = decoder->guts->stream_header.data.encoding.min_blocksize;
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
		default:
			assert(0);
			break;
	}

	switch(x = raw_header[1] & 0x0f) {
		case 0:
			if(decoder->guts->has_stream_header)
				decoder->guts->frame.header.sample_rate = decoder->guts->stream_header.data.encoding.sample_rate;
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
			decoder->guts->error_callback(decoder, FLAC__STREAM_DECODER_ERROR_LOST_SYNC, decoder->guts->client_data);
			decoder->state = FLAC__STREAM_DECODER_SEARCH_FOR_FRAME_SYNC;
			return true;
		default:
			assert(0);
	}

	x = (unsigned)(raw_header[2] >> 4);
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

	switch(x = (unsigned)(raw_header[2] & 0x0e) >> 1) {
		case 0:
			if(decoder->guts->has_stream_header)
				decoder->guts->frame.header.bits_per_sample = decoder->guts->stream_header.data.encoding.bits_per_sample;
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

	if(raw_header[2] & 0x01) { /* this should be a zero padding bit */
		decoder->guts->error_callback(decoder, FLAC__STREAM_DECODER_ERROR_LOST_SYNC, decoder->guts->client_data);
		decoder->state = FLAC__STREAM_DECODER_SEARCH_FOR_FRAME_SYNC;
		return true;
	}

	if(blocksize_hint) {
		if(!FLAC__bitbuffer_read_utf8_uint64(&decoder->guts->input, &xx, read_callback_, decoder, raw_header, &raw_header_len))
			return false; /* the read_callback_ sets the state for us */
		if(xx == 0xffffffffffffffff) {
			if(raw_header[raw_header_len-1] == 0xff) { /* MAGIC NUMBER for sync code */
				uint32 y;
				if(!FLAC__bitbuffer_peek_bit(&decoder->guts->input, &y, read_callback_, decoder))
					return false; /* the read_callback_ sets the state for us */
				if(!y) { /* MAGIC NUMBER for the last sync bit */
					decoder->state = FLAC__STREAM_DECODER_RESYNC_IN_HEADER;
					return true;
				}
				else {
					decoder->guts->error_callback(decoder, FLAC__STREAM_DECODER_ERROR_LOST_SYNC, decoder->guts->client_data);
					decoder->state = FLAC__STREAM_DECODER_SEARCH_FOR_FRAME_SYNC;
					return true;
				}
			}
			else {
				decoder->guts->error_callback(decoder, FLAC__STREAM_DECODER_ERROR_LOST_SYNC, decoder->guts->client_data);
				decoder->state = FLAC__STREAM_DECODER_SEARCH_FOR_FRAME_SYNC;
				return true;
			}
		}
		if(decoder->guts->has_stream_header && decoder->guts->stream_header.data.encoding.min_blocksize == decoder->guts->stream_header.data.encoding.max_blocksize) /* i.e. it's a fixed-blocksize stream */
			decoder->guts->frame.header.number.sample_number = (uint64)decoder->guts->last_frame_number * (int64)decoder->guts->stream_header.data.encoding.min_blocksize + xx;
		else
			decoder->guts->frame.header.number.sample_number = xx;
	}
	else {
		if(!FLAC__bitbuffer_read_utf8_uint32(&decoder->guts->input, &x, read_callback_, decoder, raw_header, &raw_header_len))
			return false; /* the read_callback_ sets the state for us */
		if(x == 0xffffffff) {
			if(raw_header[raw_header_len-1] == 0xff) { /* MAGIC NUMBER for sync code */
				uint32 y;
				if(!FLAC__bitbuffer_peek_bit(&decoder->guts->input, &y, read_callback_, decoder))
					return false; /* the read_callback_ sets the state for us */
				if(!y) { /* MAGIC NUMBER for the last sync bit */
					decoder->state = FLAC__STREAM_DECODER_RESYNC_IN_HEADER;
					return true;
				}
				else {
					decoder->guts->error_callback(decoder, FLAC__STREAM_DECODER_ERROR_LOST_SYNC, decoder->guts->client_data);
					decoder->state = FLAC__STREAM_DECODER_SEARCH_FOR_FRAME_SYNC;
					return true;
				}
			}
			else {
				decoder->guts->error_callback(decoder, FLAC__STREAM_DECODER_ERROR_LOST_SYNC, decoder->guts->client_data);
				decoder->state = FLAC__STREAM_DECODER_SEARCH_FOR_FRAME_SYNC;
				return true;
			}
		}
		decoder->guts->last_frame_number = x;
		if(decoder->guts->has_stream_header) {
			decoder->guts->frame.header.number.sample_number = (int64)decoder->guts->stream_header.data.encoding.min_blocksize * (int64)x;
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

	/* read the crc byte */
	if(!FLAC__bitbuffer_read_raw_uint32(&decoder->guts->input, &x, 8, read_callback_, decoder))
		return false; /* the read_callback_ sets the state for us */
	crc = (byte)x;

	if(FLAC__crc8(raw_header, raw_header_len) != crc) {
		decoder->guts->error_callback(decoder, FLAC__STREAM_DECODER_ERROR_LOST_SYNC, decoder->guts->client_data);
		decoder->state = FLAC__STREAM_DECODER_SEARCH_FOR_FRAME_SYNC;
		return true;
	}

	if(is_unparseable) {
		decoder->state = FLAC__STREAM_DECODER_UNPARSEABLE_STREAM;
		return false;
	}

	return true;
}

bool stream_decoder_read_subframe_(FLAC__StreamDecoder *decoder, unsigned channel)
{
	uint32 x;

	if(!FLAC__bitbuffer_read_raw_uint32(&decoder->guts->input, &x, FLAC__SUBFRAME_TYPE_LEN, read_callback_, decoder))
		return false; /* the read_callback_ sets the state for us */
	if(x & 0x01 || x & 0x80) {
		decoder->guts->error_callback(decoder, FLAC__STREAM_DECODER_ERROR_LOST_SYNC, decoder->guts->client_data);
		decoder->state = FLAC__STREAM_DECODER_SEARCH_FOR_FRAME_SYNC;
		return true;
	}
	else if(x == 0) {
		return stream_decoder_read_subframe_constant_(decoder, channel);
	}
	else if(x == 2) {
		return stream_decoder_read_subframe_verbatim_(decoder, channel);
	}
	else if(x < 16) {
		decoder->state = FLAC__STREAM_DECODER_UNPARSEABLE_STREAM;
		return false;
	}
	else if(x <= 24) {
		return stream_decoder_read_subframe_fixed_(decoder, channel, (x>>1)&7);
	}
	else if(x < 64) {
		decoder->state = FLAC__STREAM_DECODER_UNPARSEABLE_STREAM;
		return false;
	}
	else {
		return stream_decoder_read_subframe_lpc_(decoder, channel, ((x>>1)&31)+1);
	}
}

bool stream_decoder_read_subframe_constant_(FLAC__StreamDecoder *decoder, unsigned channel)
{
	FLAC__Subframe_Constant *subframe = &decoder->guts->frame.subframes[channel].data.constant;
	int32 x;
	unsigned i;
	int32 *output = decoder->guts->output[channel];

	decoder->guts->frame.subframes[channel].type = FLAC__SUBFRAME_TYPE_CONSTANT;

	if(!FLAC__bitbuffer_read_raw_int32(&decoder->guts->input, &x, decoder->guts->frame.header.bits_per_sample, read_callback_, decoder))
		return false; /* the read_callback_ sets the state for us */

	subframe->value = x;

	/* decode the subframe */
	for(i = 0; i < decoder->guts->frame.header.blocksize; i++)
		output[i] = x;

	return true;
}

bool stream_decoder_read_subframe_fixed_(FLAC__StreamDecoder *decoder, unsigned channel, const unsigned order)
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
		if(!FLAC__bitbuffer_read_raw_int32(&decoder->guts->input, &i32, decoder->guts->frame.header.bits_per_sample, read_callback_, decoder))
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
			if(!stream_decoder_read_residual_partitioned_rice_(decoder, order, subframe->entropy_coding_method.data.partitioned_rice.order, subframe->residual))
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

bool stream_decoder_read_subframe_lpc_(FLAC__StreamDecoder *decoder, unsigned channel, const unsigned order)
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
		if(!FLAC__bitbuffer_read_raw_int32(&decoder->guts->input, &i32, decoder->guts->frame.header.bits_per_sample, read_callback_, decoder))
			return false; /* the read_callback_ sets the state for us */
		subframe->warmup[u] = i32;
	}

	/* read qlp coeff precision */
	if(!FLAC__bitbuffer_read_raw_uint32(&decoder->guts->input, &u32, FLAC__SUBFRAME_LPC_QLP_COEFF_PRECISION_LEN, read_callback_, decoder))
		return false; /* the read_callback_ sets the state for us */
	if(u32 == 15) {
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
			if(!stream_decoder_read_residual_partitioned_rice_(decoder, order, subframe->entropy_coding_method.data.partitioned_rice.order, subframe->residual))
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

bool stream_decoder_read_subframe_verbatim_(FLAC__StreamDecoder *decoder, unsigned channel)
{
	FLAC__Subframe_Verbatim *subframe = &decoder->guts->frame.subframes[channel].data.verbatim;
	int32 x;
	unsigned i;

	decoder->guts->frame.subframes[channel].type = FLAC__SUBFRAME_TYPE_VERBATIM;

	subframe->data = decoder->guts->residual[channel];

	for(i = 0; i < decoder->guts->frame.header.blocksize; i++) {
		if(!FLAC__bitbuffer_read_raw_int32(&decoder->guts->input, &x, decoder->guts->frame.header.bits_per_sample, read_callback_, decoder))
			return false; /* the read_callback_ sets the state for us */
		subframe->data[i] = x;
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
		for(u = (partition_order == 0 || partition > 0)? 0 : predictor_order; u < partition_samples; u++, sample++) {
			if(!FLAC__bitbuffer_read_rice_signed(&decoder->guts->input, &i, rice_parameter, read_callback_, decoder))
				return false; /* the read_callback_ sets the state for us */
			residual[sample] = i;
		}
	}

	return true;
}

bool stream_decoder_read_zero_padding_(FLAC__StreamDecoder *decoder)
{
	if(decoder->guts->input.consumed_bits != 0) {
		uint32 zero;
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
