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

#include <stdio.h>
#include <stdlib.h> /* for malloc() */
#include <string.h> /* for strcmp() */
#include <sys/stat.h> /* for stat() */
#include "FLAC/assert.h"
#include "protected/file_decoder.h"
#include "protected/stream_decoder.h"
#include "private/md5.h"

/***********************************************************************
 *
 * Private class method prototypes
 *
 ***********************************************************************/

static FLAC__StreamDecoderReadStatus read_callback_(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], unsigned *bytes, void *client_data);
static FLAC__StreamDecoderWriteStatus write_callback_(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 *buffer[], void *client_data);
static void metadata_callback_(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetaData *metadata, void *client_data);
static void error_callback_(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data);
static FLAC__bool seek_to_absolute_sample_(FLAC__FileDecoder *decoder, long filesize, FLAC__uint64 target_sample);

/***********************************************************************
 *
 * Private class data
 *
 ***********************************************************************/

typedef struct FLAC__FileDecoderPrivate {
	FLAC__StreamDecoderWriteStatus (*write_callback)(const FLAC__FileDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 *buffer[], void *client_data);
	void (*metadata_callback)(const FLAC__FileDecoder *decoder, const FLAC__StreamMetaData *metadata, void *client_data);
	void (*error_callback)(const FLAC__FileDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data);
	void *client_data;
	FILE *file;
	char *filename; /* == NULL if stdin */
	FLAC__StreamDecoder *stream_decoder;
	struct MD5Context md5context;
	FLAC__byte stored_md5sum[16]; /* this is what is stored in the metadata */
	FLAC__byte computed_md5sum[16]; /* this is the sum we computed from the decoded data */
	/* the rest of these are only used for seeking: */
	FLAC__StreamMetaData_StreamInfo stream_info; /* we keep this around so we can figure out how to seek quickly */
	const FLAC__StreamMetaData_SeekTable *seek_table; /* we hold a pointer to the stream decoder's seek table for the same reason */
	FLAC__Frame last_frame; /* holds the info of the last frame we seeked to */
	FLAC__uint64 target_sample;
} FLAC__FileDecoderPrivate;

/***********************************************************************
 *
 * Public static class data
 *
 ***********************************************************************/

const char *FLAC__FileDecoderStateString[] = {
	"FLAC__FILE_DECODER_OK",
	"FLAC__FILE_DECODER_SEEKING",
	"FLAC__FILE_DECODER_END_OF_FILE",
	"FLAC__FILE_DECODER_ERROR_OPENING_FILE",
	"FLAC__FILE_DECODER_MEMORY_ALLOCATION_ERROR",
	"FLAC__FILE_DECODER_SEEK_ERROR",
	"FLAC__FILE_DECODER_STREAM_ERROR",
	"FLAC__FILE_DECODER_STREAM_DECODER_ERROR",
	"FLAC__FILE_DECODER_ALREADY_INITIALIZED",
	"FLAC__FILE_DECODER_INVALID_CALLBACK",
	"FLAC__FILE_DECODER_UNINITIALIZED"
};

/***********************************************************************
 *
 * Class constructor/destructor
 *
 ***********************************************************************/

FLAC__FileDecoder *FLAC__file_decoder_new()
{
	FLAC__FileDecoder *decoder;

	FLAC__ASSERT(sizeof(int) >= 4); /* we want to die right away if this is not true */

	decoder = (FLAC__FileDecoder*)malloc(sizeof(FLAC__FileDecoder));
	if(decoder == 0) {
		return 0;
	}
	decoder->protected = (FLAC__FileDecoderProtected*)malloc(sizeof(FLAC__FileDecoderProtected));
	if(decoder->protected == 0) {
		free(decoder);
		return 0;
	}
	decoder->private = (FLAC__FileDecoderPrivate*)malloc(sizeof(FLAC__FileDecoderPrivate));
	if(decoder->private == 0) {
		free(decoder->protected);
		free(decoder);
		return 0;
	}

	decoder->protected->state = FLAC__FILE_DECODER_UNINITIALIZED;

	decoder->private->filename = 0;
	decoder->private->write_callback = 0;
	decoder->private->metadata_callback = 0;
	decoder->private->error_callback = 0;
	decoder->private->client_data = 0;

	return decoder;
}

void FLAC__file_decoder_delete(FLAC__FileDecoder *decoder)
{
	FLAC__ASSERT(decoder != 0);
	FLAC__ASSERT(decoder->protected != 0);
	FLAC__ASSERT(decoder->private != 0);

	free(decoder->private);
	free(decoder->protected);
	free(decoder);
}

/***********************************************************************
 *
 * Public class methods
 *
 ***********************************************************************/

FLAC__FileDecoderState FLAC__file_decoder_init(FLAC__FileDecoder *decoder)
{
	FLAC__ASSERT(decoder != 0);

	if(decoder->protected->state != FLAC__FILE_DECODER_UNINITIALIZED)
		return decoder->protected->state = FLAC__FILE_DECODER_ALREADY_INITIALIZED;

	decoder->protected->state = FLAC__FILE_DECODER_OK;

    if(0 == decoder->private->write_callback || 0 == decoder->private->metadata_callback || 0 == decoder->private->error_callback)
        return decoder->protected->state = FLAC__FILE_DECODER_INVALID_CALLBACK;

	decoder->private->file = 0;
	decoder->private->stream_decoder = 0;
	decoder->private->seek_table = 0;

	if(0 == strcmp(decoder->private->filename, "-"))
		decoder->private->file = stdin;
	else
		decoder->private->file = fopen(decoder->private->filename, "rb");

	if(decoder->private->file == 0)
		return decoder->protected->state = FLAC__FILE_DECODER_ERROR_OPENING_FILE;

	/* We initialize the MD5Context even though we may never use it.  This is
	 * because md5_checking may be turned on to start and then turned off if a
	 * seek occurs.  So we always init the context here and finalize it in
	 * FLAC__file_decoder_finish() to make sure things are always cleaned up
	 * properly.
	 */
	MD5Init(&decoder->private->md5context);

	decoder->private->stream_decoder = FLAC__stream_decoder_new();

	FLAC__stream_decoder_set_read_callback(decoder->private->stream_decoder, read_callback_);
	FLAC__stream_decoder_set_write_callback(decoder->private->stream_decoder, write_callback_);
	FLAC__stream_decoder_set_metadata_callback(decoder->private->stream_decoder, metadata_callback_);
	FLAC__stream_decoder_set_error_callback(decoder->private->stream_decoder, error_callback_);
	FLAC__stream_decoder_set_client_data(decoder->private->stream_decoder, decoder);

	if(FLAC__stream_decoder_init(decoder->private->stream_decoder) != FLAC__STREAM_DECODER_SEARCH_FOR_METADATA)
		return decoder->protected->state = FLAC__FILE_DECODER_STREAM_DECODER_ERROR;

	return decoder->protected->state;
}

FLAC__bool FLAC__file_decoder_finish(FLAC__FileDecoder *decoder)
{
	FLAC__bool md5_failed = false;

	FLAC__ASSERT(decoder != 0);
	if(decoder->protected->state == FLAC__FILE_DECODER_UNINITIALIZED)
		return true;
	if(decoder->private->file != 0 && decoder->private->file != stdin)
		fclose(decoder->private->file);
	if(0 != decoder->private->filename)
		free(decoder->private->filename);
	/* see the comment in FLAC__file_decoder_init() as to why we always
	 * call MD5Final()
	 */
	MD5Final(decoder->private->computed_md5sum, &decoder->private->md5context);
	if(decoder->private->stream_decoder != 0) {
		FLAC__stream_decoder_finish(decoder->private->stream_decoder);
		FLAC__stream_decoder_delete(decoder->private->stream_decoder);
	}
	if(decoder->protected->md5_checking) {
		if(memcmp(decoder->private->stored_md5sum, decoder->private->computed_md5sum, 16))
			md5_failed = true;
	}
	decoder->protected->state = FLAC__FILE_DECODER_UNINITIALIZED;
	return !md5_failed;
}

FLAC__bool FLAC__file_decoder_set_md5_checking(const FLAC__FileDecoder *decoder, FLAC__bool value)
{
	if(decoder->protected->state != FLAC__FILE_DECODER_UNINITIALIZED)
		return false;
	decoder->protected->md5_checking = value;
	return true;
}

FLAC__bool FLAC__file_decoder_set_filename(const FLAC__FileDecoder *decoder, const char *value)
{
	if(decoder->protected->state != FLAC__FILE_DECODER_UNINITIALIZED)
		return false;
	if(0 == (decoder->private->filename = (char*)malloc(strlen(value)+1))) {
		decoder->protected->state = FLAC__FILE_DECODER_MEMORY_ALLOCATION_ERROR;
		return false;
	}
	strcpy(decoder->private->filename, value);
	return true;
}

FLAC__bool FLAC__file_decoder_set_write_callback(const FLAC__FileDecoder *decoder, FLAC__StreamDecoderWriteStatus (*value)(const FLAC__FileDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 *buffer[], void *client_data))
{
	if(decoder->protected->state != FLAC__FILE_DECODER_UNINITIALIZED)
		return false;
	decoder->private->write_callback = value;
	return true;
}

FLAC__bool FLAC__file_decoder_set_metadata_callback(const FLAC__FileDecoder *decoder, void (*value)(const FLAC__FileDecoder *decoder, const FLAC__StreamMetaData *metadata, void *client_data))
{
	if(decoder->protected->state != FLAC__FILE_DECODER_UNINITIALIZED)
		return false;
	decoder->private->metadata_callback = value;
	return true;
}

FLAC__bool FLAC__file_decoder_set_error_callback(const FLAC__FileDecoder *decoder, void (*value)(const FLAC__FileDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data))
{
	if(decoder->protected->state != FLAC__FILE_DECODER_UNINITIALIZED)
		return false;
	decoder->private->error_callback = value;
	return true;
}

FLAC__bool FLAC__file_decoder_set_client_data(const FLAC__FileDecoder *decoder, void *value)
{
	if(decoder->protected->state != FLAC__FILE_DECODER_UNINITIALIZED)
		return false;
	decoder->private->client_data = value;
	return true;
}

FLAC__FileDecoderState FLAC__file_decoder_get_state(const FLAC__FileDecoder *decoder)
{
	return decoder->protected->state;
}

FLAC__bool FLAC__file_decoder_get_md5_checking(const FLAC__FileDecoder *decoder)
{
	return decoder->protected->md5_checking;
}

FLAC__bool FLAC__file_decoder_process_whole_file(FLAC__FileDecoder *decoder)
{
	FLAC__bool ret;
	FLAC__ASSERT(decoder != 0);

	if(decoder->private->stream_decoder->protected->state == FLAC__STREAM_DECODER_END_OF_STREAM)
		decoder->protected->state = FLAC__FILE_DECODER_END_OF_FILE;

	if(decoder->protected->state == FLAC__FILE_DECODER_END_OF_FILE)
		return true;

	FLAC__ASSERT(decoder->protected->state == FLAC__FILE_DECODER_OK);

	ret = FLAC__stream_decoder_process_whole_stream(decoder->private->stream_decoder);
	if(!ret)
		decoder->protected->state = FLAC__FILE_DECODER_STREAM_ERROR;

	return ret;
}

FLAC__bool FLAC__file_decoder_process_metadata(FLAC__FileDecoder *decoder)
{
	FLAC__bool ret;
	FLAC__ASSERT(decoder != 0);

	if(decoder->private->stream_decoder->protected->state == FLAC__STREAM_DECODER_END_OF_STREAM)
		decoder->protected->state = FLAC__FILE_DECODER_END_OF_FILE;

	if(decoder->protected->state == FLAC__FILE_DECODER_END_OF_FILE)
		return true;

	FLAC__ASSERT(decoder->protected->state == FLAC__FILE_DECODER_OK);

	ret = FLAC__stream_decoder_process_metadata(decoder->private->stream_decoder);
	if(!ret)
		decoder->protected->state = FLAC__FILE_DECODER_STREAM_ERROR;

	return ret;
}

FLAC__bool FLAC__file_decoder_process_one_frame(FLAC__FileDecoder *decoder)
{
	FLAC__bool ret;
	FLAC__ASSERT(decoder != 0);

	if(decoder->private->stream_decoder->protected->state == FLAC__STREAM_DECODER_END_OF_STREAM)
		decoder->protected->state = FLAC__FILE_DECODER_END_OF_FILE;

	if(decoder->protected->state == FLAC__FILE_DECODER_END_OF_FILE)
		return true;

	FLAC__ASSERT(decoder->protected->state == FLAC__FILE_DECODER_OK);

	ret = FLAC__stream_decoder_process_one_frame(decoder->private->stream_decoder);
	if(!ret)
		decoder->protected->state = FLAC__FILE_DECODER_STREAM_ERROR;

	return ret;
}

FLAC__bool FLAC__file_decoder_process_remaining_frames(FLAC__FileDecoder *decoder)
{
	FLAC__bool ret;
	FLAC__ASSERT(decoder != 0);

	if(decoder->private->stream_decoder->protected->state == FLAC__STREAM_DECODER_END_OF_STREAM)
		decoder->protected->state = FLAC__FILE_DECODER_END_OF_FILE;

	if(decoder->protected->state == FLAC__FILE_DECODER_END_OF_FILE)
		return true;

	FLAC__ASSERT(decoder->protected->state == FLAC__FILE_DECODER_OK);

	ret = FLAC__stream_decoder_process_remaining_frames(decoder->private->stream_decoder);
	if(!ret)
		decoder->protected->state = FLAC__FILE_DECODER_STREAM_ERROR;

	return ret;
}

/***********************************************************************
 *
 * Private class methods
 *
 ***********************************************************************/

FLAC__bool FLAC__file_decoder_seek_absolute(FLAC__FileDecoder *decoder, FLAC__uint64 sample)
{
	long filesize;
	struct stat filestats;

	FLAC__ASSERT(decoder != 0);
	FLAC__ASSERT(decoder->protected->state == FLAC__FILE_DECODER_OK);

	if(decoder->private->filename == 0) { /* means the file is stdin... */
		decoder->protected->state = FLAC__FILE_DECODER_SEEK_ERROR;
		return false;
	}

	decoder->protected->state = FLAC__FILE_DECODER_SEEKING;

	/* turn off md5 checking if a seek is attempted */
	decoder->protected->md5_checking = false;

	if(!FLAC__stream_decoder_reset(decoder->private->stream_decoder)) {
		decoder->protected->state = FLAC__FILE_DECODER_STREAM_ERROR;
		return false;
	}
	/* get the file length */
	if(stat(decoder->private->filename, &filestats) != 0) {
		decoder->protected->state = FLAC__FILE_DECODER_SEEK_ERROR;
		return false;
	}
	filesize = filestats.st_size;
	/* rewind */
	if(0 != fseek(decoder->private->file, 0, SEEK_SET)) {
		decoder->protected->state = FLAC__FILE_DECODER_SEEK_ERROR;
		return false;
	}
	if(!FLAC__stream_decoder_process_metadata(decoder->private->stream_decoder)) {
		decoder->protected->state = FLAC__FILE_DECODER_STREAM_ERROR;
		return false;
	}
	if(sample > decoder->private->stream_info.total_samples) {
		decoder->protected->state = FLAC__FILE_DECODER_SEEK_ERROR;
		return false;
	}

	return seek_to_absolute_sample_(decoder, filesize, sample);
}

FLAC__StreamDecoderReadStatus read_callback_(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], unsigned *bytes, void *client_data)
{
	FLAC__FileDecoder *file_decoder = (FLAC__FileDecoder *)client_data;
	(void)decoder;
	if(feof(file_decoder->private->file)) {
		file_decoder->protected->state = FLAC__FILE_DECODER_END_OF_FILE;
		return FLAC__STREAM_DECODER_READ_END_OF_STREAM;
	}
	else if(*bytes > 0) {
		size_t bytes_read = fread(buffer, sizeof(FLAC__byte), *bytes, file_decoder->private->file);
		if(bytes_read == 0) {
			if(feof(file_decoder->private->file)) {
				file_decoder->protected->state = FLAC__FILE_DECODER_END_OF_FILE;
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

FLAC__StreamDecoderWriteStatus write_callback_(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 *buffer[], void *client_data)
{
	FLAC__FileDecoder *file_decoder = (FLAC__FileDecoder *)client_data;
	(void)decoder;

	if(file_decoder->protected->state == FLAC__FILE_DECODER_SEEKING) {
		FLAC__uint64 this_frame_sample = frame->header.number.sample_number;
		FLAC__uint64 next_frame_sample = this_frame_sample + (FLAC__uint64)frame->header.blocksize;
		FLAC__uint64 target_sample = file_decoder->private->target_sample;

		file_decoder->private->last_frame = *frame; /* save the frame in the private */
		if(this_frame_sample <= target_sample && target_sample < next_frame_sample) { /* we hit our target frame */
			unsigned delta = (unsigned)(target_sample - this_frame_sample);
			/* kick out of seek mode */
			file_decoder->protected->state = FLAC__FILE_DECODER_OK;
			/* shift out the samples before target_sample */
			if(delta > 0) {
				unsigned channel;
				const FLAC__int32 *newbuffer[FLAC__MAX_CHANNELS];
				for(channel = 0; channel < frame->header.channels; channel++)
					newbuffer[channel] = buffer[channel] + delta;
				file_decoder->private->last_frame.header.blocksize -= delta;
				file_decoder->private->last_frame.header.number.sample_number += (FLAC__uint64)delta;
				/* write the relevant samples */
				return file_decoder->private->write_callback(file_decoder, &file_decoder->private->last_frame, newbuffer, file_decoder->private->client_data);
			}
			else {
				/* write the relevant samples */
				return file_decoder->private->write_callback(file_decoder, frame, buffer, file_decoder->private->client_data);
			}
		}
		else {
			return FLAC__STREAM_DECODER_WRITE_CONTINUE;
		}
	}
	else {
		if(file_decoder->protected->md5_checking) {
			if(!FLAC__MD5Accumulate(&file_decoder->private->md5context, buffer, frame->header.channels, frame->header.blocksize, (frame->header.bits_per_sample+7) / 8))
				return FLAC__STREAM_DECODER_WRITE_ABORT;
		}
		return file_decoder->private->write_callback(file_decoder, frame, buffer, file_decoder->private->client_data);
	}
}

void metadata_callback_(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetaData *metadata, void *client_data)
{
	FLAC__FileDecoder *file_decoder = (FLAC__FileDecoder *)client_data;
	(void)decoder;

	if(metadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
		file_decoder->private->stream_info = metadata->data.stream_info;
		/* save the MD5 signature for comparison later */
		memcpy(file_decoder->private->stored_md5sum, metadata->data.stream_info.md5sum, 16);
		if(0 == memcmp(file_decoder->private->stored_md5sum, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", 16))
			file_decoder->protected->md5_checking = false;
	}
	else if(metadata->type == FLAC__METADATA_TYPE_SEEKTABLE) {
		file_decoder->private->seek_table = &metadata->data.seek_table;
	}

	if(file_decoder->protected->state != FLAC__FILE_DECODER_SEEKING)
		file_decoder->private->metadata_callback(file_decoder, metadata, file_decoder->private->client_data);
}

void error_callback_(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
	FLAC__FileDecoder *file_decoder = (FLAC__FileDecoder *)client_data;
	(void)decoder;

	if(file_decoder->protected->state != FLAC__FILE_DECODER_SEEKING)
		file_decoder->private->error_callback(file_decoder, status, file_decoder->private->client_data);
}

FLAC__bool seek_to_absolute_sample_(FLAC__FileDecoder *decoder, long filesize, FLAC__uint64 target_sample)
{
	long first_frame_offset, lower_bound, upper_bound, pos = -1, last_pos = -1;
	int i, lower_seek_point = -1, upper_seek_point = -1;
	unsigned approx_bytes_per_frame;
	FLAC__uint64 last_frame_sample = 0xffffffffffffffff;
	FLAC__bool needs_seek;
	const FLAC__bool is_variable_blocksize_stream = (decoder->private->stream_info.min_blocksize != decoder->private->stream_info.max_blocksize);

	/* we are just guessing here, but we want to guess high, not low */
	if(decoder->private->stream_info.max_framesize > 0) {
		approx_bytes_per_frame = decoder->private->stream_info.max_framesize;
	}
	else if(!is_variable_blocksize_stream) {
		/* note there are no () around 'decoder->private->stream_info.bits_per_sample/8' to keep precision up since it's an integer calulation */
		approx_bytes_per_frame = decoder->private->stream_info.min_blocksize * decoder->private->stream_info.channels * decoder->private->stream_info.bits_per_sample/8 + 64;
	}
	else
		approx_bytes_per_frame = 1152 * decoder->private->stream_info.channels * decoder->private->stream_info.bits_per_sample/8 + 64;

	/*
	 * The file pointer is currently at the first frame plus any read
	 * ahead data, so first we get the file pointer, then subtract
	 * uncomsumed bytes to get the position of the first frame in the
	 * file.
	 */
	if(-1 == (first_frame_offset = ftell(decoder->private->file))) {
		decoder->protected->state = FLAC__FILE_DECODER_SEEK_ERROR;
		return false;
	}
	first_frame_offset -= FLAC__stream_decoder_get_input_bytes_unconsumed(decoder->private->stream_decoder);
	FLAC__ASSERT(first_frame_offset >= 0);

	/*
	 * First, we set an upper and lower bound on where in the
	 * file we will search.  For now we assume the worst case
	 * scenario, which is our best guess at the beginning of
	 * the first and last frames.
	 */
	lower_bound = first_frame_offset;

	/* calc the upper_bound, beyond which we never want to seek */
	if(decoder->private->stream_info.max_framesize > 0)
		upper_bound = filesize - (decoder->private->stream_info.max_framesize + 128 + 2); /* 128 for a possible ID3V1 tag, 2 for indexing differences */
	else
		upper_bound = filesize - ((decoder->private->stream_info.channels * decoder->private->stream_info.bits_per_sample * FLAC__MAX_BLOCK_SIZE) / 8 + 128 + 2);

	/*
	 * Now we refine the bounds if we have a seektable with
	 * suitable points.  Note that according to the spec they
	 * must be ordered by ascending sample number.
	 */
	if(0 != decoder->private->seek_table) {
		/* find the closest seek point <= target_sample, if it exists */
		for(i = (int)decoder->private->seek_table->num_points - 1; i >= 0; i--) {
			if(decoder->private->seek_table->points[i].sample_number != FLAC__STREAM_METADATA_SEEKPOINT_PLACEHOLDER && decoder->private->seek_table->points[i].sample_number <= target_sample)
				break;
		}
		if(i >= 0) { /* i.e. we found a suitable seek point... */
			lower_bound = first_frame_offset + decoder->private->seek_table->points[i].stream_offset;
			lower_seek_point = i;
		}

		/* find the closest seek point > target_sample, if it exists */
		for(i = 0; i < (int)decoder->private->seek_table->num_points; i++) {
			if(decoder->private->seek_table->points[i].sample_number != FLAC__STREAM_METADATA_SEEKPOINT_PLACEHOLDER && decoder->private->seek_table->points[i].sample_number > target_sample)
				break;
		}
		if(i < (int)decoder->private->seek_table->num_points) { /* i.e. we found a suitable seek point... */
			upper_bound = first_frame_offset + decoder->private->seek_table->points[i].stream_offset;
			upper_seek_point = i;
		}
	}

	/*
	 * Now guess at where within those bounds our target
	 * sample will be.
	 */
	if(lower_seek_point >= 0) {
		/* first see if our sample is within a few frames of the lower seekpoint */
		if(decoder->private->seek_table->points[lower_seek_point].sample_number <= target_sample && target_sample < decoder->private->seek_table->points[lower_seek_point].sample_number + (decoder->private->seek_table->points[lower_seek_point].frame_samples * 4)) {
			pos = lower_bound;
		}
		else if(upper_seek_point >= 0) {
			const FLAC__uint64 target_offset = target_sample - decoder->private->seek_table->points[lower_seek_point].sample_number;
			const FLAC__uint64 range_samples = decoder->private->seek_table->points[upper_seek_point].sample_number - decoder->private->seek_table->points[lower_seek_point].sample_number;
			const long range_bytes = upper_bound - lower_bound;
#ifdef _MSC_VER
			/* with VC++ you have to spoon feed it the casting */
			pos = lower_bound + (long)((double)(FLAC__int64)target_offset / (double)(FLAC__int64)range_samples * (double)(range_bytes-1)) - approx_bytes_per_frame;
#else
			pos = lower_bound + (long)((double)target_offset / (double)range_samples * (double)(range_bytes-1)) - approx_bytes_per_frame;
#endif
		}
	}
	if(pos < 0) {
		/* We need to use the metadata and the filelength to estimate the position of the frame with the correct sample */
#ifdef _MSC_VER
		/* with VC++ you have to spoon feed it the casting */
		pos = first_frame_offset + (long)((double)(FLAC__int64)target_sample / (double)(FLAC__int64)decoder->private->stream_info.total_samples * (double)(filesize-first_frame_offset-1)) - approx_bytes_per_frame;
#else
		pos = first_frame_offset + (long)((double)target_sample / (double)decoder->private->stream_info.total_samples * (double)(filesize-first_frame_offset-1)) - approx_bytes_per_frame;
#endif
	}

	/* clip the position to the bounds, lower bound takes precedence */
	if(pos >= upper_bound)
		pos = upper_bound-1;
	if(pos < lower_bound)
		pos = lower_bound;
	needs_seek = true;

	decoder->private->target_sample = target_sample;
	while(1) {
		if(needs_seek) {
			if(-1 == fseek(decoder->private->file, pos, SEEK_SET)) {
				decoder->protected->state = FLAC__FILE_DECODER_SEEK_ERROR;
				return false;
			}
			if(!FLAC__stream_decoder_flush(decoder->private->stream_decoder)) {
				decoder->protected->state = FLAC__FILE_DECODER_STREAM_ERROR;
				return false;
			}
		}
		if(!FLAC__stream_decoder_process_one_frame(decoder->private->stream_decoder)) {
			decoder->protected->state = FLAC__FILE_DECODER_SEEK_ERROR;
			return false;
		}
		/* our write callback will change the state when it gets to the target frame */
		if(decoder->protected->state != FLAC__FILE_DECODER_SEEKING) {
			break;
		}
		else { /* we need to narrow the search */
			FLAC__uint64 this_frame_sample = decoder->private->last_frame.header.number.sample_number;
			if(this_frame_sample == last_frame_sample) {
				/* our last move backwards wasn't big enough */
				pos -= (last_pos - pos);
				needs_seek = true;
			}
			else {
				if(target_sample < this_frame_sample) {
					last_pos = pos;
					approx_bytes_per_frame = decoder->private->last_frame.header.blocksize * decoder->private->last_frame.header.channels * decoder->private->last_frame.header.bits_per_sample/8 + 64;
					pos -= approx_bytes_per_frame;
					needs_seek = true;
				}
				else { /* target_sample >= this_frame_sample + this frame's blocksize */
					last_pos = pos;
					if(-1 == (pos = ftell(decoder->private->file))) {
						decoder->protected->state = FLAC__FILE_DECODER_SEEK_ERROR;
						return false;
					}
					pos -= FLAC__stream_decoder_get_input_bytes_unconsumed(decoder->private->stream_decoder);
					needs_seek = false;
				}
			}
			if(pos < lower_bound)
				pos = lower_bound;
			last_frame_sample = this_frame_sample;
		}
	}

	return true;
}
