/* libFLAC - Free Lossless Audio Coder library
 * Copyright (C) 2000  Josh Coalson
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
#include <string.h> /* for strcmp() */
#include "FLAC/file_decoder.h"
#include "protected/stream_decoder.h"

typedef struct FLAC__FileDecoderPrivate {
	FLAC__StreamDecoderWriteStatus (*write_callback)(const FLAC__FileDecoder *decoder, const FLAC__FrameHeader *header, const int32 *buffer[], void *client_data);
	void (*metadata_callback)(const FLAC__FileDecoder *decoder, const FLAC__StreamMetaData *metadata, void *client_data);
	void (*error_callback)(const FLAC__FileDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data);
	void *client_data;
	FILE *file;
	FLAC__StreamDecoder *stream;
	/* the rest of these are only used for seeking: */
	FLAC__StreamMetaData_Encoding metadata; /* we keep this around so we can figure out how to seek quickly */
	FLAC__FrameHeader last_frame_header; /* holds the info of the last frame we seeked to */
	uint64 target_sample;
} FLAC__FileDecoderPrivate;

static FLAC__StreamDecoderReadStatus read_callback_(const FLAC__StreamDecoder *decoder, byte buffer[], unsigned *bytes, void *client_data);
static FLAC__StreamDecoderWriteStatus write_callback_(const FLAC__StreamDecoder *decoder, const FLAC__FrameHeader *header, const int32 *buffer[], void *client_data);
static void metadata_callback_(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetaData *metadata, void *client_data);
static void error_callback_(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data);
static bool seek_to_absolute_sample_(FLAC__FileDecoder *decoder, long filesize, uint64 target_sample);

FLAC__FileDecoder *FLAC__file_decoder_get_new_instance()
{
	FLAC__FileDecoder *decoder = (FLAC__FileDecoder*)malloc(sizeof(FLAC__FileDecoder));
	if(decoder != 0) {
		decoder->state = FLAC__FILE_DECODER_UNINITIALIZED;
		decoder->guts = 0;
	}
	return decoder;
}

void FLAC__file_decoder_free_instance(FLAC__FileDecoder *decoder)
{
	free(decoder);
}

FLAC__FileDecoderState FLAC__file_decoder_init(
	FLAC__FileDecoder *decoder,
	const char *filename,
	FLAC__StreamDecoderWriteStatus (*write_callback)(const FLAC__FileDecoder *decoder, const FLAC__FrameHeader *header, const int32 *buffer[], void *client_data),
	void (*metadata_callback)(const FLAC__FileDecoder *decoder, const FLAC__StreamMetaData *metadata, void *client_data),
	void (*error_callback)(const FLAC__FileDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data),
	void *client_data
)
{
	assert(sizeof(int) >= 4); /* we want to die right away if this is not true */
	assert(decoder != 0);
	assert(write_callback != 0);
	assert(metadata_callback != 0);
	assert(error_callback != 0);
	assert(decoder->state == FLAC__FILE_DECODER_UNINITIALIZED);
	assert(decoder->guts == 0);

	decoder->state = FLAC__FILE_DECODER_OK;

	decoder->guts = (FLAC__FileDecoderPrivate*)malloc(sizeof(FLAC__FileDecoderPrivate));
	if(decoder->guts == 0)
		return decoder->state = FLAC__FILE_DECODER_MEMORY_ALLOCATION_ERROR;

	decoder->guts->write_callback = write_callback;
	decoder->guts->metadata_callback = metadata_callback;
	decoder->guts->error_callback = error_callback;
	decoder->guts->client_data = client_data;
	decoder->guts->stream = 0;

	if(0 == strcmp(filename, "-"))
		decoder->guts->file = stdin;
	else
		decoder->guts->file = fopen(filename, "rb");
	if(decoder->guts->file == 0)
		return decoder->state = FLAC__FILE_DECODER_ERROR_OPENING_FILE;

	decoder->guts->stream = FLAC__stream_decoder_get_new_instance();
	if(FLAC__stream_decoder_init(decoder->guts->stream, read_callback_, write_callback_, metadata_callback_, error_callback_, decoder) != FLAC__STREAM_DECODER_SEARCH_FOR_METADATA)
		return decoder->state = FLAC__FILE_DECODER_MEMORY_ALLOCATION_ERROR; /* this is based on internal knowledge of FLAC__stream_decoder_init() */

	return decoder->state;
}

void FLAC__file_decoder_finish(FLAC__FileDecoder *decoder)
{
	assert(decoder != 0);
	if(decoder->state == FLAC__FILE_DECODER_UNINITIALIZED)
		return;
	if(decoder->guts != 0) {
		if(decoder->guts->file != 0 && decoder->guts->file != stdin)
			fclose(decoder->guts->file);
		if(decoder->guts->stream != 0) {
			FLAC__stream_decoder_finish(decoder->guts->stream);
			FLAC__stream_decoder_free_instance(decoder->guts->stream);
		}
		free(decoder->guts);
		decoder->guts = 0;
	}
	decoder->state = FLAC__FILE_DECODER_UNINITIALIZED;
}

bool FLAC__file_decoder_process_whole_file(FLAC__FileDecoder *decoder)
{
	bool ret;
	assert(decoder != 0);

	if(decoder->state == FLAC__FILE_DECODER_END_OF_FILE)
		return true;

	assert(decoder->state == FLAC__FILE_DECODER_OK);

	ret = FLAC__stream_decoder_process_whole_stream(decoder->guts->stream);
	if(!ret)
		decoder->state = FLAC__FILE_DECODER_STREAM_ERROR;

	return ret;
}

bool FLAC__file_decoder_process_metadata(FLAC__FileDecoder *decoder)
{
	bool ret;
	assert(decoder != 0);

	if(decoder->state == FLAC__FILE_DECODER_END_OF_FILE)
		return true;

	assert(decoder->state == FLAC__FILE_DECODER_OK);

	ret = FLAC__stream_decoder_process_metadata(decoder->guts->stream);
	if(!ret)
		decoder->state = FLAC__FILE_DECODER_STREAM_ERROR;

	return ret;
}

bool FLAC__file_decoder_process_one_frame(FLAC__FileDecoder *decoder)
{
	bool ret;
	assert(decoder != 0);

	if(decoder->state == FLAC__FILE_DECODER_END_OF_FILE)
		return true;

	assert(decoder->state == FLAC__FILE_DECODER_OK);

	ret = FLAC__stream_decoder_process_one_frame(decoder->guts->stream);
	if(!ret)
		decoder->state = FLAC__FILE_DECODER_STREAM_ERROR;

	return ret;
}

bool FLAC__file_decoder_process_remaining_frames(FLAC__FileDecoder *decoder)
{
	bool ret;
	assert(decoder != 0);

	if(decoder->state == FLAC__FILE_DECODER_END_OF_FILE)
		return true;

	assert(decoder->state == FLAC__FILE_DECODER_OK);

	ret = FLAC__stream_decoder_process_remaining_frames(decoder->guts->stream);
	if(!ret)
		decoder->state = FLAC__FILE_DECODER_STREAM_ERROR;

	return ret;
}

bool FLAC__file_decoder_seek_absolute(FLAC__FileDecoder *decoder, uint64 sample)
{
	long filesize;

	assert(decoder != 0);
	assert(decoder->state == FLAC__FILE_DECODER_OK);

	decoder->state = FLAC__FILE_DECODER_SEEKING;

	if(!FLAC__stream_decoder_reset(decoder->guts->stream)) {
		decoder->state = FLAC__FILE_DECODER_STREAM_ERROR;
		return false;
	}
	/* get the file length */
	if(0 != fseek(decoder->guts->file, 0, SEEK_END)) {
		decoder->state = FLAC__FILE_DECODER_SEEK_ERROR;
		return false;
	}
	fflush(decoder->guts->file);
	if(-1 == (filesize = ftell(decoder->guts->file))) {
		decoder->state = FLAC__FILE_DECODER_SEEK_ERROR;
		return false;
	}
	/* rewind */
	if(0 != fseek(decoder->guts->file, 0, SEEK_SET)) {
		decoder->state = FLAC__FILE_DECODER_SEEK_ERROR;
		return false;
	}
	if(!FLAC__stream_decoder_process_metadata(decoder->guts->stream)) {
		decoder->state = FLAC__FILE_DECODER_STREAM_ERROR;
		return false;
	}
	if(sample > decoder->guts->metadata.total_samples) {
		decoder->state = FLAC__FILE_DECODER_SEEK_ERROR;
		return false;
	}

	return seek_to_absolute_sample_(decoder, filesize, sample);
}

FLAC__StreamDecoderReadStatus read_callback_(const FLAC__StreamDecoder *decoder, byte buffer[], unsigned *bytes, void *client_data)
{
	FLAC__FileDecoder *file_decoder = (FLAC__FileDecoder *)client_data;
	(void)decoder;
	if(feof(file_decoder->guts->file)) {
		file_decoder->state = FLAC__FILE_DECODER_END_OF_FILE;
		return FLAC__STREAM_DECODER_READ_END_OF_STREAM;
	}
	else if(*bytes > 0) {
		size_t bytes_read = fread(buffer, sizeof(byte), *bytes, file_decoder->guts->file);
		if(bytes_read == 0) {
			if(feof(file_decoder->guts->file)) {
				file_decoder->state = FLAC__FILE_DECODER_END_OF_FILE;
				return FLAC__STREAM_DECODER_READ_END_OF_STREAM;
			}
			else
				return FLAC__STREAM_DECODER_READ_ABORT;
		}
		else {
			*bytes = (unsigned)bytes_read;
			return FLAC__STREAM_DECODER_READ_CONTINUE;
		}
	}
	else
		return FLAC__STREAM_DECODER_READ_ABORT; /* abort to avoid a deadlock */
}

FLAC__StreamDecoderWriteStatus write_callback_(const FLAC__StreamDecoder *decoder, const FLAC__FrameHeader *header, const int32 *buffer[], void *client_data)
{
	FLAC__FileDecoder *file_decoder = (FLAC__FileDecoder *)client_data;
	(void)decoder;

	if(file_decoder->state == FLAC__FILE_DECODER_SEEKING) {
		uint64 this_frame_sample = header->number.sample_number;
		uint64 next_frame_sample = this_frame_sample + (uint64)header->blocksize;
		uint64 target_sample = file_decoder->guts->target_sample;

		file_decoder->guts->last_frame_header = *header; /* save the header in the guts */
		if(this_frame_sample <= target_sample && target_sample < next_frame_sample) { /* we hit our target frame */
			unsigned delta = (unsigned)(target_sample - this_frame_sample);
			/* kick out of seek mode */
			file_decoder->state = FLAC__FILE_DECODER_OK;
			/* shift out the samples before target_sample */
			if(delta > 0) {
				unsigned channel;
				const int32 *newbuffer[FLAC__MAX_CHANNELS];
				for(channel = 0; channel < header->channels; channel++)
					newbuffer[channel] = buffer[channel] + delta;
				file_decoder->guts->last_frame_header.blocksize -= delta;
				file_decoder->guts->last_frame_header.number.sample_number += (uint64)delta;
				/* write the relevant samples */
				return file_decoder->guts->write_callback(file_decoder, &file_decoder->guts->last_frame_header, newbuffer, file_decoder->guts->client_data);
			}
			else {
				/* write the relevant samples */
				return file_decoder->guts->write_callback(file_decoder, header, buffer, file_decoder->guts->client_data);
			}
		}
		else {
			return FLAC__STREAM_DECODER_WRITE_CONTINUE;
		}
	}
	else {
		return file_decoder->guts->write_callback(file_decoder, header, buffer, file_decoder->guts->client_data);
	}
}

void metadata_callback_(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetaData *metadata, void *client_data)
{
	FLAC__FileDecoder *file_decoder = (FLAC__FileDecoder *)client_data;
	(void)decoder;

	if(metadata->type == FLAC__METADATA_TYPE_ENCODING)
		file_decoder->guts->metadata = metadata->data.encoding;
	if(file_decoder->state != FLAC__FILE_DECODER_SEEKING)
		file_decoder->guts->metadata_callback(file_decoder, metadata, file_decoder->guts->client_data);
}

void error_callback_(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
	FLAC__FileDecoder *file_decoder = (FLAC__FileDecoder *)client_data;
	(void)decoder;

	if(file_decoder->state != FLAC__FILE_DECODER_SEEKING)
		file_decoder->guts->error_callback(file_decoder, status, file_decoder->guts->client_data);
}

bool seek_to_absolute_sample_(FLAC__FileDecoder *decoder, long filesize, uint64 target_sample)
{
	long l, r, pos, last_pos = -1;
	unsigned approx_bytes_per_frame;
	uint64 last_frame_sample = 0xffffffffffffffff;
	bool needs_seek;
	const bool is_variable_blocksize_stream = (decoder->guts->metadata.min_blocksize != decoder->guts->metadata.max_blocksize);

	if(!is_variable_blocksize_stream) {
		/* we are just guessing here, but we want to guess high, not low */
		/* note there are no () around 'decoder->guts->metadata.bits_per_sample/8' to keep precision up since it's an integer calulation */
		approx_bytes_per_frame = decoder->guts->metadata.min_blocksize * decoder->guts->metadata.channels * decoder->guts->metadata.bits_per_sample/8 + 64;
	}
	else
		approx_bytes_per_frame = 1152 * decoder->guts->metadata.channels * decoder->guts->metadata.bits_per_sample/8 + 64;

	/* Now we need to use the metadata and the filelength to search to the frame with the correct sample */
	if(-1 == (l = ftell(decoder->guts->file))) {
		decoder->state = FLAC__FILE_DECODER_SEEK_ERROR;
		return false;
	}
	l -= FLAC__stream_decoder_input_bytes_unconsumed(decoder->guts->stream);
#ifdef _MSC_VER
	/* with VC++ you have to spoon feed it the casting */
	pos = l + (long)((double)(int64)target_sample / (double)(int64)decoder->guts->metadata.total_samples * (double)(filesize-l+1)) - approx_bytes_per_frame;
#else
	pos = l + (long)((double)target_sample / (double)decoder->guts->metadata.total_samples * (double)(filesize-l+1)) - approx_bytes_per_frame;
#endif
	r = filesize - ((decoder->guts->metadata.channels * decoder->guts->metadata.bits_per_sample * FLAC__MAX_BLOCK_SIZE) / 8 + 64);
	if(pos >= r)
		pos = r-1;
	if(pos < l)
		pos = l;
	needs_seek = true;

	decoder->guts->target_sample = target_sample;
	while(1) {
		if(needs_seek) {
			if(-1 == fseek(decoder->guts->file, pos, SEEK_SET)) {
				decoder->state = FLAC__FILE_DECODER_SEEK_ERROR;
				return false;
			}
			if(!FLAC__stream_decoder_flush(decoder->guts->stream)) {
				decoder->state = FLAC__FILE_DECODER_STREAM_ERROR;
				return false;
			}
		}
		if(!FLAC__stream_decoder_process_one_frame(decoder->guts->stream)) {
			decoder->state = FLAC__FILE_DECODER_SEEK_ERROR;
			return false;
		}
		/* our write callback will change the state when it gets to the target frame */
		if(decoder->state != FLAC__FILE_DECODER_SEEKING) {
			break;
		}
		else { /* we need to narrow the search */
			uint64 this_frame_sample = decoder->guts->last_frame_header.number.sample_number;
			if(this_frame_sample == last_frame_sample) {
				/* our last move backwards wasn't big enough */
				pos -= (last_pos - pos);
				needs_seek = true;
			}
			else {
				if(target_sample < this_frame_sample) {
					last_pos = pos;
					approx_bytes_per_frame = decoder->guts->last_frame_header.blocksize * decoder->guts->last_frame_header.channels * decoder->guts->last_frame_header.bits_per_sample/8 + 64;
					pos -= approx_bytes_per_frame;
					needs_seek = true;
				}
				else {
					last_pos = pos;
					if(-1 == (pos = ftell(decoder->guts->file))) {
						decoder->state = FLAC__FILE_DECODER_SEEK_ERROR;
						return false;
					}
					pos -= FLAC__stream_decoder_input_bytes_unconsumed(decoder->guts->stream);
					needs_seek = false;
				}
			}
			if(pos < l)
				pos = l;
			last_frame_sample = this_frame_sample;
		}
	}

	return true;
}
