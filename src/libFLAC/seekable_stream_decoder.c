/* libFLAC - Free Lossless Audio Codec library
 * Copyright (C) 2000,2001,2002  Josh Coalson
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
#include <string.h> /* for memcpy()/memcmp() */
#include "FLAC/assert.h"
#include "protected/seekable_stream_decoder.h"
#include "protected/stream_decoder.h"
#include "private/md5.h"

/***********************************************************************
 *
 * Private class method prototypes
 *
 ***********************************************************************/

static void set_defaults_(FLAC__SeekableStreamDecoder *decoder);
static FLAC__StreamDecoderReadStatus read_callback_(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], unsigned *bytes, void *client_data);
static FLAC__StreamDecoderWriteStatus write_callback_(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data);
static void metadata_callback_(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data);
static void error_callback_(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data);
static FLAC__bool seek_to_absolute_sample_(FLAC__SeekableStreamDecoder *decoder, FLAC__uint64 stream_length, FLAC__uint64 target_sample);

/***********************************************************************
 *
 * Private class data
 *
 ***********************************************************************/

typedef struct FLAC__SeekableStreamDecoderPrivate {
	FLAC__SeekableStreamDecoderReadCallback read_callback;
	FLAC__SeekableStreamDecoderSeekCallback seek_callback;
	FLAC__SeekableStreamDecoderTellCallback tell_callback;
	FLAC__SeekableStreamDecoderLengthCallback length_callback;
	FLAC__SeekableStreamDecoderEofCallback eof_callback;
	FLAC__SeekableStreamDecoderWriteCallback write_callback;
	FLAC__SeekableStreamDecoderMetadataCallback metadata_callback;
	FLAC__SeekableStreamDecoderErrorCallback error_callback;
	void *client_data;
	FLAC__StreamDecoder *stream_decoder;
	FLAC__bool do_md5_checking; /* initially gets protected_->md5_checking but is turned off after a seek */
	struct MD5Context md5context;
	FLAC__byte stored_md5sum[16]; /* this is what is stored in the metadata */
	FLAC__byte computed_md5sum[16]; /* this is the sum we computed from the decoded data */
	/* the rest of these are only used for seeking: */
	FLAC__StreamMetadata_StreamInfo stream_info; /* we keep this around so we can figure out how to seek quickly */
	const FLAC__StreamMetadata_SeekTable *seek_table; /* we hold a pointer to the stream decoder's seek table for the same reason */
	/* Since we always want to see the STREAMINFO and SEEK_TABLE blocks at this level, we need some extra flags to keep track of whether they should be passed on up through the metadata_callback */
	FLAC__bool ignore_stream_info_block;
	FLAC__bool ignore_seek_table_block;
	FLAC__Frame last_frame; /* holds the info of the last frame we seeked to */
	FLAC__uint64 target_sample;
} FLAC__SeekableStreamDecoderPrivate;

/***********************************************************************
 *
 * Public static class data
 *
 ***********************************************************************/

const char * const FLAC__SeekableStreamDecoderStateString[] = {
	"FLAC__SEEKABLE_STREAM_DECODER_OK",
	"FLAC__SEEKABLE_STREAM_DECODER_SEEKING",
	"FLAC__SEEKABLE_STREAM_DECODER_END_OF_STREAM",
	"FLAC__SEEKABLE_STREAM_DECODER_MEMORY_ALLOCATION_ERROR",
	"FLAC__SEEKABLE_STREAM_DECODER_STREAM_DECODER_ERROR",
	"FLAC__SEEKABLE_STREAM_DECODER_READ_ERROR",
	"FLAC__SEEKABLE_STREAM_DECODER_SEEK_ERROR",
	"FLAC__SEEKABLE_STREAM_DECODER_ALREADY_INITIALIZED",
	"FLAC__SEEKABLE_STREAM_DECODER_INVALID_CALLBACK",
	"FLAC__SEEKABLE_STREAM_DECODER_UNINITIALIZED"
};

const char * const FLAC__SeekableStreamDecoderReadStatusString[] = {
	"FLAC__SEEKABLE_STREAM_DECODER_READ_STATUS_OK",
	"FLAC__SEEKABLE_STREAM_DECODER_READ_STATUS_ERROR"
};

const char * const FLAC__SeekableStreamDecoderSeekStatusString[] = {
	"FLAC__SEEKABLE_STREAM_DECODER_SEEK_STATUS_OK",
	"FLAC__SEEKABLE_STREAM_DECODER_SEEK_STATUS_ERROR"
};

const char * const FLAC__SeekableStreamDecoderTellStatusString[] = {
	"FLAC__SEEKABLE_STREAM_DECODER_TELL_STATUS_OK",
	"FLAC__SEEKABLE_STREAM_DECODER_TELL_STATUS_ERROR"
};

const char * const FLAC__SeekableStreamDecoderLengthStatusString[] = {
	"FLAC__SEEKABLE_STREAM_DECODER_LENGTH_STATUS_OK",
	"FLAC__SEEKABLE_STREAM_DECODER_LENGTH_STATUS_ERROR"
};


/***********************************************************************
 *
 * Class constructor/destructor
 *
 ***********************************************************************/

FLAC__SeekableStreamDecoder *FLAC__seekable_stream_decoder_new()
{
	FLAC__SeekableStreamDecoder *decoder;

	FLAC__ASSERT(sizeof(int) >= 4); /* we want to die right away if this is not true */

	decoder = (FLAC__SeekableStreamDecoder*)malloc(sizeof(FLAC__SeekableStreamDecoder));
	if(decoder == 0) {
		return 0;
	}
	decoder->protected_ = (FLAC__SeekableStreamDecoderProtected*)malloc(sizeof(FLAC__SeekableStreamDecoderProtected));
	if(decoder->protected_ == 0) {
		free(decoder);
		return 0;
	}
	decoder->private_ = (FLAC__SeekableStreamDecoderPrivate*)malloc(sizeof(FLAC__SeekableStreamDecoderPrivate));
	if(decoder->private_ == 0) {
		free(decoder->protected_);
		free(decoder);
		return 0;
	}

	decoder->private_->stream_decoder = FLAC__stream_decoder_new();

	if(0 == decoder->private_->stream_decoder) {
		free(decoder->private_);
		free(decoder->protected_);
		free(decoder);
		return 0;
	}

	set_defaults_(decoder);

	decoder->protected_->state = FLAC__SEEKABLE_STREAM_DECODER_UNINITIALIZED;

	return decoder;
}

void FLAC__seekable_stream_decoder_delete(FLAC__SeekableStreamDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->protected_);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->private_->stream_decoder);

	(void)FLAC__seekable_stream_decoder_finish(decoder);

	FLAC__stream_decoder_delete(decoder->private_->stream_decoder);

	free(decoder->private_);
	free(decoder->protected_);
	free(decoder);
}

/***********************************************************************
 *
 * Public class methods
 *
 ***********************************************************************/

FLAC__SeekableStreamDecoderState FLAC__seekable_stream_decoder_init(FLAC__SeekableStreamDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);

	if(decoder->protected_->state != FLAC__SEEKABLE_STREAM_DECODER_UNINITIALIZED)
		return decoder->protected_->state = FLAC__SEEKABLE_STREAM_DECODER_ALREADY_INITIALIZED;

	if(0 == decoder->private_->read_callback || 0 == decoder->private_->seek_callback || 0 == decoder->private_->tell_callback || 0 == decoder->private_->length_callback || 0 == decoder->private_->eof_callback)
		return decoder->protected_->state = FLAC__SEEKABLE_STREAM_DECODER_INVALID_CALLBACK;

	if(0 == decoder->private_->write_callback || 0 == decoder->private_->metadata_callback || 0 == decoder->private_->error_callback)
		return decoder->protected_->state = FLAC__SEEKABLE_STREAM_DECODER_INVALID_CALLBACK;

	decoder->private_->seek_table = 0;

	decoder->private_->do_md5_checking = decoder->protected_->md5_checking;

	/* We initialize the MD5Context even though we may never use it.  This is
	 * because md5 checking may be turned on to start and then turned off if a
	 * seek occurs.  So we always init the context here and finalize it in
	 * FLAC__seekable_stream_decoder_finish() to make sure things are always
	 * cleaned up properly.
	 */
	MD5Init(&decoder->private_->md5context);

	FLAC__stream_decoder_set_read_callback(decoder->private_->stream_decoder, read_callback_);
	FLAC__stream_decoder_set_write_callback(decoder->private_->stream_decoder, write_callback_);
	FLAC__stream_decoder_set_metadata_callback(decoder->private_->stream_decoder, metadata_callback_);
	FLAC__stream_decoder_set_error_callback(decoder->private_->stream_decoder, error_callback_);
	FLAC__stream_decoder_set_client_data(decoder->private_->stream_decoder, decoder);

	/* We always want to see these blocks.  Whether or not we pass them up
	 * through the metadata callback will be determined by flags set in our
	 * implementation of ..._set_metadata_respond/ignore...()
	 */
	FLAC__stream_decoder_set_metadata_respond(decoder->private_->stream_decoder, FLAC__METADATA_TYPE_STREAMINFO);
	FLAC__stream_decoder_set_metadata_respond(decoder->private_->stream_decoder, FLAC__METADATA_TYPE_SEEKTABLE);

	if(FLAC__stream_decoder_init(decoder->private_->stream_decoder) != FLAC__STREAM_DECODER_SEARCH_FOR_METADATA)
		return decoder->protected_->state = FLAC__SEEKABLE_STREAM_DECODER_STREAM_DECODER_ERROR;

	return decoder->protected_->state = FLAC__SEEKABLE_STREAM_DECODER_OK;
}

FLAC__bool FLAC__seekable_stream_decoder_finish(FLAC__SeekableStreamDecoder *decoder)
{
	FLAC__bool md5_failed = false;

	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);

	if(decoder->protected_->state == FLAC__SEEKABLE_STREAM_DECODER_UNINITIALIZED)
		return true;

	FLAC__ASSERT(0 != decoder->private_->stream_decoder);

	/* see the comment in FLAC__seekable_stream_decoder_init() as to why we
	 * always call MD5Final()
	 */
	MD5Final(decoder->private_->computed_md5sum, &decoder->private_->md5context);

	FLAC__stream_decoder_finish(decoder->private_->stream_decoder);

	if(decoder->private_->do_md5_checking) {
		if(memcmp(decoder->private_->stored_md5sum, decoder->private_->computed_md5sum, 16))
			md5_failed = true;
	}

	set_defaults_(decoder);

	decoder->protected_->state = FLAC__SEEKABLE_STREAM_DECODER_UNINITIALIZED;

	return !md5_failed;
}

FLAC__bool FLAC__seekable_stream_decoder_set_md5_checking(FLAC__SeekableStreamDecoder *decoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->protected_);
	if(decoder->protected_->state != FLAC__SEEKABLE_STREAM_DECODER_UNINITIALIZED)
		return false;
	decoder->protected_->md5_checking = value;
	return true;
}

FLAC__bool FLAC__seekable_stream_decoder_set_read_callback(FLAC__SeekableStreamDecoder *decoder, FLAC__SeekableStreamDecoderReadCallback value)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	if(decoder->protected_->state != FLAC__SEEKABLE_STREAM_DECODER_UNINITIALIZED)
		return false;
	decoder->private_->read_callback = value;
	return true;
}

FLAC__bool FLAC__seekable_stream_decoder_set_seek_callback(FLAC__SeekableStreamDecoder *decoder, FLAC__SeekableStreamDecoderSeekCallback value)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	if(decoder->protected_->state != FLAC__SEEKABLE_STREAM_DECODER_UNINITIALIZED)
		return false;
	decoder->private_->seek_callback = value;
	return true;
}

FLAC__bool FLAC__seekable_stream_decoder_set_tell_callback(FLAC__SeekableStreamDecoder *decoder, FLAC__SeekableStreamDecoderTellCallback value)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	if(decoder->protected_->state != FLAC__SEEKABLE_STREAM_DECODER_UNINITIALIZED)
		return false;
	decoder->private_->tell_callback = value;
	return true;
}

FLAC__bool FLAC__seekable_stream_decoder_set_length_callback(FLAC__SeekableStreamDecoder *decoder, FLAC__SeekableStreamDecoderLengthCallback value)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	if(decoder->protected_->state != FLAC__SEEKABLE_STREAM_DECODER_UNINITIALIZED)
		return false;
	decoder->private_->length_callback = value;
	return true;
}

FLAC__bool FLAC__seekable_stream_decoder_set_eof_callback(FLAC__SeekableStreamDecoder *decoder, FLAC__SeekableStreamDecoderEofCallback value)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	if(decoder->protected_->state != FLAC__SEEKABLE_STREAM_DECODER_UNINITIALIZED)
		return false;
	decoder->private_->eof_callback = value;
	return true;
}

FLAC__bool FLAC__seekable_stream_decoder_set_write_callback(FLAC__SeekableStreamDecoder *decoder, FLAC__SeekableStreamDecoderWriteCallback value)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	if(decoder->protected_->state != FLAC__SEEKABLE_STREAM_DECODER_UNINITIALIZED)
		return false;
	decoder->private_->write_callback = value;
	return true;
}

FLAC__bool FLAC__seekable_stream_decoder_set_metadata_callback(FLAC__SeekableStreamDecoder *decoder, FLAC__SeekableStreamDecoderMetadataCallback value)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	if(decoder->protected_->state != FLAC__SEEKABLE_STREAM_DECODER_UNINITIALIZED)
		return false;
	decoder->private_->metadata_callback = value;
	return true;
}

FLAC__bool FLAC__seekable_stream_decoder_set_error_callback(FLAC__SeekableStreamDecoder *decoder, FLAC__SeekableStreamDecoderErrorCallback value)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	if(decoder->protected_->state != FLAC__SEEKABLE_STREAM_DECODER_UNINITIALIZED)
		return false;
	decoder->private_->error_callback = value;
	return true;
}

FLAC__bool FLAC__seekable_stream_decoder_set_client_data(FLAC__SeekableStreamDecoder *decoder, void *value)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	if(decoder->protected_->state != FLAC__SEEKABLE_STREAM_DECODER_UNINITIALIZED)
		return false;
	decoder->private_->client_data = value;
	return true;
}

FLAC__bool FLAC__seekable_stream_decoder_set_metadata_respond(FLAC__SeekableStreamDecoder *decoder, FLAC__MetadataType type)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	FLAC__ASSERT(0 != decoder->private_->stream_decoder);
	if(decoder->protected_->state != FLAC__SEEKABLE_STREAM_DECODER_UNINITIALIZED)
		return false;
	if(type == FLAC__METADATA_TYPE_STREAMINFO)
		decoder->private_->ignore_stream_info_block = false;
	else if(type == FLAC__METADATA_TYPE_SEEKTABLE)
		decoder->private_->ignore_seek_table_block = false;
	return FLAC__stream_decoder_set_metadata_respond(decoder->private_->stream_decoder, type);
}

FLAC__bool FLAC__seekable_stream_decoder_set_metadata_respond_application(FLAC__SeekableStreamDecoder *decoder, const FLAC__byte id[4])
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	FLAC__ASSERT(0 != decoder->private_->stream_decoder);
	if(decoder->protected_->state != FLAC__SEEKABLE_STREAM_DECODER_UNINITIALIZED)
		return false;
	return FLAC__stream_decoder_set_metadata_respond_application(decoder->private_->stream_decoder, id);
}

FLAC__bool FLAC__seekable_stream_decoder_set_metadata_respond_all(FLAC__SeekableStreamDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	FLAC__ASSERT(0 != decoder->private_->stream_decoder);
	if(decoder->protected_->state != FLAC__SEEKABLE_STREAM_DECODER_UNINITIALIZED)
		return false;
	decoder->private_->ignore_stream_info_block = false;
	decoder->private_->ignore_seek_table_block = false;
	return FLAC__stream_decoder_set_metadata_respond_all(decoder->private_->stream_decoder);
}

FLAC__bool FLAC__seekable_stream_decoder_set_metadata_ignore(FLAC__SeekableStreamDecoder *decoder, FLAC__MetadataType type)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	FLAC__ASSERT(0 != decoder->private_->stream_decoder);
	if(decoder->protected_->state != FLAC__SEEKABLE_STREAM_DECODER_UNINITIALIZED)
		return false;
	if(type == FLAC__METADATA_TYPE_STREAMINFO)
		decoder->private_->ignore_stream_info_block = true;
	else if(type == FLAC__METADATA_TYPE_SEEKTABLE)
		decoder->private_->ignore_seek_table_block = true;
	return FLAC__stream_decoder_set_metadata_ignore(decoder->private_->stream_decoder, type);
}

FLAC__bool FLAC__seekable_stream_decoder_set_metadata_ignore_application(FLAC__SeekableStreamDecoder *decoder, const FLAC__byte id[4])
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	FLAC__ASSERT(0 != decoder->private_->stream_decoder);
	if(decoder->protected_->state != FLAC__SEEKABLE_STREAM_DECODER_UNINITIALIZED)
		return false;
	return FLAC__stream_decoder_set_metadata_ignore_application(decoder->private_->stream_decoder, id);
}

FLAC__bool FLAC__seekable_stream_decoder_set_metadata_ignore_all(FLAC__SeekableStreamDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	FLAC__ASSERT(0 != decoder->private_->stream_decoder);
	if(decoder->protected_->state != FLAC__SEEKABLE_STREAM_DECODER_UNINITIALIZED)
		return false;
	decoder->private_->ignore_stream_info_block = true;
	decoder->private_->ignore_seek_table_block = true;
	return FLAC__stream_decoder_set_metadata_ignore_all(decoder->private_->stream_decoder);
}

FLAC__SeekableStreamDecoderState FLAC__seekable_stream_decoder_get_state(const FLAC__SeekableStreamDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->protected_);
	return decoder->protected_->state;
}

FLAC__SeekableStreamDecoderState FLAC__seekable_stream_decoder_get_stream_decoder_state(const FLAC__SeekableStreamDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	return FLAC__stream_decoder_get_state(decoder->private_->stream_decoder);
}

FLAC__bool FLAC__seekable_stream_decoder_get_md5_checking(const FLAC__SeekableStreamDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->protected_);
	return decoder->protected_->md5_checking;
}

unsigned FLAC__seekable_stream_decoder_get_channels(const FLAC__SeekableStreamDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	return FLAC__stream_decoder_get_channels(decoder->private_->stream_decoder);
}

FLAC__ChannelAssignment FLAC__seekable_stream_decoder_get_channel_assignment(const FLAC__SeekableStreamDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	return FLAC__stream_decoder_get_channel_assignment(decoder->private_->stream_decoder);
}

unsigned FLAC__seekable_stream_decoder_get_bits_per_sample(const FLAC__SeekableStreamDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	return FLAC__stream_decoder_get_bits_per_sample(decoder->private_->stream_decoder);
}

unsigned FLAC__seekable_stream_decoder_get_sample_rate(const FLAC__SeekableStreamDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	return FLAC__stream_decoder_get_sample_rate(decoder->private_->stream_decoder);
}

unsigned FLAC__seekable_stream_decoder_get_blocksize(const FLAC__SeekableStreamDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	return FLAC__stream_decoder_get_blocksize(decoder->private_->stream_decoder);
}

FLAC__bool FLAC__seekable_stream_decoder_flush(FLAC__SeekableStreamDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);

	decoder->private_->do_md5_checking = false;

	if(!FLAC__stream_decoder_flush(decoder->private_->stream_decoder)) {
		decoder->protected_->state = FLAC__SEEKABLE_STREAM_DECODER_STREAM_DECODER_ERROR;
		return false;
	}

	decoder->protected_->state = FLAC__SEEKABLE_STREAM_DECODER_OK;

	return true;
}

FLAC__bool FLAC__seekable_stream_decoder_reset(FLAC__SeekableStreamDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);

	if(!FLAC__seekable_stream_decoder_flush(decoder)) {
		decoder->protected_->state = FLAC__SEEKABLE_STREAM_DECODER_STREAM_DECODER_ERROR;
		return false;
	}

	if(!FLAC__stream_decoder_reset(decoder->private_->stream_decoder)) {
		decoder->protected_->state = FLAC__SEEKABLE_STREAM_DECODER_STREAM_DECODER_ERROR;
		return false;
	}

	decoder->private_->seek_table = 0;

	decoder->private_->do_md5_checking = decoder->protected_->md5_checking;

	/* We initialize the MD5Context even though we may never use it.  This is
	 * because md5 checking may be turned on to start and then turned off if a
	 * seek occurs.  So we always init the context here and finalize it in
	 * FLAC__seekable_stream_decoder_finish() to make sure things are always
	 * cleaned up properly.
	 */
	MD5Init(&decoder->private_->md5context);

	decoder->protected_->state = FLAC__SEEKABLE_STREAM_DECODER_OK;

	return true;
}

FLAC__bool FLAC__seekable_stream_decoder_process_single(FLAC__SeekableStreamDecoder *decoder)
{
	FLAC__bool ret;
	FLAC__ASSERT(0 != decoder);

	if(decoder->private_->stream_decoder->protected_->state == FLAC__STREAM_DECODER_END_OF_STREAM)
		decoder->protected_->state = FLAC__SEEKABLE_STREAM_DECODER_END_OF_STREAM;

	if(decoder->protected_->state == FLAC__SEEKABLE_STREAM_DECODER_END_OF_STREAM)
		return true;

	FLAC__ASSERT(decoder->protected_->state == FLAC__SEEKABLE_STREAM_DECODER_OK);

	ret = FLAC__stream_decoder_process_single(decoder->private_->stream_decoder);
	if(!ret)
		decoder->protected_->state = FLAC__SEEKABLE_STREAM_DECODER_STREAM_DECODER_ERROR;

	return ret;
}

FLAC__bool FLAC__seekable_stream_decoder_process_until_end_of_metadata(FLAC__SeekableStreamDecoder *decoder)
{
	FLAC__bool ret;
	FLAC__ASSERT(0 != decoder);

	if(decoder->private_->stream_decoder->protected_->state == FLAC__STREAM_DECODER_END_OF_STREAM)
		decoder->protected_->state = FLAC__SEEKABLE_STREAM_DECODER_END_OF_STREAM;

	if(decoder->protected_->state == FLAC__SEEKABLE_STREAM_DECODER_END_OF_STREAM)
		return true;

	FLAC__ASSERT(decoder->protected_->state == FLAC__SEEKABLE_STREAM_DECODER_OK);

	ret = FLAC__stream_decoder_process_until_end_of_metadata(decoder->private_->stream_decoder);
	if(!ret)
		decoder->protected_->state = FLAC__SEEKABLE_STREAM_DECODER_STREAM_DECODER_ERROR;

	return ret;
}

FLAC__bool FLAC__seekable_stream_decoder_process_until_end_of_stream(FLAC__SeekableStreamDecoder *decoder)
{
	FLAC__bool ret;
	FLAC__ASSERT(0 != decoder);

	if(decoder->private_->stream_decoder->protected_->state == FLAC__STREAM_DECODER_END_OF_STREAM)
		decoder->protected_->state = FLAC__SEEKABLE_STREAM_DECODER_END_OF_STREAM;

	if(decoder->protected_->state == FLAC__SEEKABLE_STREAM_DECODER_END_OF_STREAM)
		return true;

	FLAC__ASSERT(decoder->protected_->state == FLAC__SEEKABLE_STREAM_DECODER_OK);

	ret = FLAC__stream_decoder_process_until_end_of_stream(decoder->private_->stream_decoder);
	if(!ret)
		decoder->protected_->state = FLAC__SEEKABLE_STREAM_DECODER_STREAM_DECODER_ERROR;

	return ret;
}

FLAC__bool FLAC__seekable_stream_decoder_seek_absolute(FLAC__SeekableStreamDecoder *decoder, FLAC__uint64 sample)
{
	FLAC__uint64 length;

	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(decoder->protected_->state == FLAC__SEEKABLE_STREAM_DECODER_OK || decoder->protected_->state == FLAC__SEEKABLE_STREAM_DECODER_END_OF_STREAM);

	decoder->protected_->state = FLAC__SEEKABLE_STREAM_DECODER_SEEKING;

	/* turn off md5 checking if a seek is attempted */
	decoder->private_->do_md5_checking = false;

	if(!FLAC__stream_decoder_reset(decoder->private_->stream_decoder)) {
		decoder->protected_->state = FLAC__SEEKABLE_STREAM_DECODER_STREAM_DECODER_ERROR;
		return false;
	}
	/* get the file length */
	if(decoder->private_->length_callback(decoder, &length, decoder->private_->client_data) != FLAC__SEEKABLE_STREAM_DECODER_LENGTH_STATUS_OK) {
		decoder->protected_->state = FLAC__SEEKABLE_STREAM_DECODER_SEEK_ERROR;
		return false;
	}
	/* rewind */
	if(decoder->private_->seek_callback(decoder, 0, decoder->private_->client_data) != FLAC__SEEKABLE_STREAM_DECODER_SEEK_STATUS_OK) {
		decoder->protected_->state = FLAC__SEEKABLE_STREAM_DECODER_SEEK_ERROR;
		return false;
	}
	if(!FLAC__stream_decoder_process_until_end_of_metadata(decoder->private_->stream_decoder)) {
		decoder->protected_->state = FLAC__SEEKABLE_STREAM_DECODER_STREAM_DECODER_ERROR;
		return false;
	}
	if(decoder->private_->stream_info.total_samples > 0 && sample > decoder->private_->stream_info.total_samples) {
		decoder->protected_->state = FLAC__SEEKABLE_STREAM_DECODER_SEEK_ERROR;
		return false;
	}

	return seek_to_absolute_sample_(decoder, length, sample);
}

/***********************************************************************
 *
 * Private class methods
 *
 ***********************************************************************/

void set_defaults_(FLAC__SeekableStreamDecoder *decoder)
{
	decoder->private_->read_callback = 0;
	decoder->private_->seek_callback = 0;
	decoder->private_->tell_callback = 0;
	decoder->private_->length_callback = 0;
	decoder->private_->eof_callback = 0;
	decoder->private_->write_callback = 0;
	decoder->private_->metadata_callback = 0;
	decoder->private_->error_callback = 0;
	decoder->private_->client_data = 0;
	/* WATCHOUT: these should match the default behavior of FLAC__StreamDecoder */
	decoder->private_->ignore_stream_info_block = false;
	decoder->private_->ignore_seek_table_block = true;

	decoder->protected_->md5_checking = false;
}

FLAC__StreamDecoderReadStatus read_callback_(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], unsigned *bytes, void *client_data)
{
	FLAC__SeekableStreamDecoder *seekable_stream_decoder = (FLAC__SeekableStreamDecoder *)client_data;
	(void)decoder;
	if(seekable_stream_decoder->private_->eof_callback(seekable_stream_decoder, seekable_stream_decoder->private_->client_data)) {
		seekable_stream_decoder->protected_->state = FLAC__SEEKABLE_STREAM_DECODER_END_OF_STREAM;
		return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
	}
	else if(*bytes > 0) {
		unsigned bytes_read = *bytes;
		if(seekable_stream_decoder->private_->read_callback(seekable_stream_decoder, buffer, &bytes_read, seekable_stream_decoder->private_->client_data) != FLAC__SEEKABLE_STREAM_DECODER_READ_STATUS_OK) {
			seekable_stream_decoder->protected_->state = FLAC__SEEKABLE_STREAM_DECODER_READ_ERROR;
			return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
		}
		if(bytes_read == 0) {
			if(seekable_stream_decoder->private_->eof_callback(seekable_stream_decoder, seekable_stream_decoder->private_->client_data)) {
				seekable_stream_decoder->protected_->state = FLAC__SEEKABLE_STREAM_DECODER_END_OF_STREAM;
				return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
			}
			else
				return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
		}
		else {
			*bytes = bytes_read;
			return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
		}
	}
	else
		return FLAC__STREAM_DECODER_READ_STATUS_ABORT; /* abort to avoid a deadlock */
}

FLAC__StreamDecoderWriteStatus write_callback_(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data)
{
	FLAC__SeekableStreamDecoder *seekable_stream_decoder = (FLAC__SeekableStreamDecoder *)client_data;
	(void)decoder;

	if(seekable_stream_decoder->protected_->state == FLAC__SEEKABLE_STREAM_DECODER_SEEKING) {
		FLAC__uint64 this_frame_sample = frame->header.number.sample_number;
		FLAC__uint64 next_frame_sample = this_frame_sample + (FLAC__uint64)frame->header.blocksize;
		FLAC__uint64 target_sample = seekable_stream_decoder->private_->target_sample;

		FLAC__ASSERT(frame->header.number_type == FLAC__FRAME_NUMBER_TYPE_SAMPLE_NUMBER);

		seekable_stream_decoder->private_->last_frame = *frame; /* save the frame */
		if(this_frame_sample <= target_sample && target_sample < next_frame_sample) { /* we hit our target frame */
			unsigned delta = (unsigned)(target_sample - this_frame_sample);
			/* kick out of seek mode */
			seekable_stream_decoder->protected_->state = FLAC__SEEKABLE_STREAM_DECODER_OK;
			/* shift out the samples before target_sample */
			if(delta > 0) {
				unsigned channel;
				const FLAC__int32 *newbuffer[FLAC__MAX_CHANNELS];
				for(channel = 0; channel < frame->header.channels; channel++)
					newbuffer[channel] = buffer[channel] + delta;
				seekable_stream_decoder->private_->last_frame.header.blocksize -= delta;
				seekable_stream_decoder->private_->last_frame.header.number.sample_number += (FLAC__uint64)delta;
				/* write the relevant samples */
				return seekable_stream_decoder->private_->write_callback(seekable_stream_decoder, &seekable_stream_decoder->private_->last_frame, newbuffer, seekable_stream_decoder->private_->client_data);
			}
			else {
				/* write the relevant samples */
				return seekable_stream_decoder->private_->write_callback(seekable_stream_decoder, frame, buffer, seekable_stream_decoder->private_->client_data);
			}
		}
		else {
			return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
		}
	}
	else {
		if(seekable_stream_decoder->private_->do_md5_checking) {
			if(!FLAC__MD5Accumulate(&seekable_stream_decoder->private_->md5context, buffer, frame->header.channels, frame->header.blocksize, (frame->header.bits_per_sample+7) / 8))
				return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
		}
		return seekable_stream_decoder->private_->write_callback(seekable_stream_decoder, frame, buffer, seekable_stream_decoder->private_->client_data);
	}
}

void metadata_callback_(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data)
{
	FLAC__SeekableStreamDecoder *seekable_stream_decoder = (FLAC__SeekableStreamDecoder *)client_data;
	(void)decoder;

	if(metadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
		seekable_stream_decoder->private_->stream_info = metadata->data.stream_info;
		/* save the MD5 signature for comparison later */
		memcpy(seekable_stream_decoder->private_->stored_md5sum, metadata->data.stream_info.md5sum, 16);
		if(0 == memcmp(seekable_stream_decoder->private_->stored_md5sum, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", 16))
			seekable_stream_decoder->private_->do_md5_checking = false;
	}
	else if(metadata->type == FLAC__METADATA_TYPE_SEEKTABLE) {
		seekable_stream_decoder->private_->seek_table = &metadata->data.seek_table;
	}

	if(seekable_stream_decoder->protected_->state != FLAC__SEEKABLE_STREAM_DECODER_SEEKING) {
		FLAC__bool ignore_block = false;
		if(metadata->type == FLAC__METADATA_TYPE_STREAMINFO && seekable_stream_decoder->private_->ignore_stream_info_block)
			ignore_block = true;
		else if(metadata->type == FLAC__METADATA_TYPE_SEEKTABLE && seekable_stream_decoder->private_->ignore_seek_table_block)
			ignore_block = true;
		if(!ignore_block)
			seekable_stream_decoder->private_->metadata_callback(seekable_stream_decoder, metadata, seekable_stream_decoder->private_->client_data);
	}
}

void error_callback_(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
	FLAC__SeekableStreamDecoder *seekable_stream_decoder = (FLAC__SeekableStreamDecoder *)client_data;
	(void)decoder;

	if(seekable_stream_decoder->protected_->state != FLAC__SEEKABLE_STREAM_DECODER_SEEKING)
		seekable_stream_decoder->private_->error_callback(seekable_stream_decoder, status, seekable_stream_decoder->private_->client_data);
}

FLAC__bool seek_to_absolute_sample_(FLAC__SeekableStreamDecoder *decoder, FLAC__uint64 stream_length, FLAC__uint64 target_sample)
{
	FLAC__uint64 first_frame_offset, lower_bound, upper_bound;
	FLAC__int64 pos = -1, last_pos = -1;
	int i, lower_seek_point = -1, upper_seek_point = -1;
	unsigned approx_bytes_per_frame;
	FLAC__uint64 last_frame_sample = 0xffffffffffffffff;
	FLAC__bool needs_seek;
	const FLAC__uint64 total_samples = decoder->private_->stream_info.total_samples;
	const unsigned min_blocksize = decoder->private_->stream_info.min_blocksize;
	const unsigned max_blocksize = decoder->private_->stream_info.max_blocksize;
	const unsigned max_framesize = decoder->private_->stream_info.max_framesize;
	const unsigned channels = FLAC__seekable_stream_decoder_get_channels(decoder);
	const unsigned bps = FLAC__seekable_stream_decoder_get_bits_per_sample(decoder);

	/* we are just guessing here, but we want to guess high, not low */
	if(max_framesize > 0) {
		approx_bytes_per_frame = max_framesize;
	}
	/*
	 * Check if it's a known fixed-blocksize stream.  Note that though
	 * the spec doesn't allow zeroes in the STREAMINFO block, we may
	 * never get a STREAMINFO block when decoding so the value of
	 * min_blocksize might be zero.
	 */
	else if(min_blocksize == max_blocksize && min_blocksize > 0) {
		/* note there are no () around 'bps/8' to keep precision up since it's an integer calulation */
		approx_bytes_per_frame = min_blocksize * channels * bps/8 + 64;
	}
	else
		approx_bytes_per_frame = 4608 * channels * bps/8 + 64;

	/*
	 * The stream position is currently at the first frame plus any read
	 * ahead data, so first we get the stream position, then subtract
	 * uncomsumed bytes to get the position of the first frame in the
	 * stream.
	 */
	if(decoder->private_->tell_callback(decoder, &first_frame_offset, decoder->private_->client_data) != FLAC__SEEKABLE_STREAM_DECODER_TELL_STATUS_OK) {
		decoder->protected_->state = FLAC__SEEKABLE_STREAM_DECODER_SEEK_ERROR;
		return false;
	}
	FLAC__ASSERT(first_frame_offset >= FLAC__stream_decoder_get_input_bytes_unconsumed(decoder->private_->stream_decoder));
	first_frame_offset -= FLAC__stream_decoder_get_input_bytes_unconsumed(decoder->private_->stream_decoder);

	/*
	 * First, we set an upper and lower bound on where in the
	 * stream we will search.  For now we assume the worst case
	 * scenario, which is our best guess at the beginning of
	 * the first and last frames.
	 */
	lower_bound = first_frame_offset;

	/* calc the upper_bound, beyond which we never want to seek */
	if(max_framesize > 0)
		upper_bound = stream_length - (max_framesize + 128 + 2); /* 128 for a possible ID3V1 tag, 2 for indexing differences */
	else
		upper_bound = stream_length - ((channels * bps * FLAC__MAX_BLOCK_SIZE) / 8 + 128 + 2);

	/*
	 * Now we refine the bounds if we have a seektable with
	 * suitable points.  Note that according to the spec they
	 * must be ordered by ascending sample number.
	 */
	if(0 != decoder->private_->seek_table) {
		/* find the closest seek point <= target_sample, if it exists */
		for(i = (int)decoder->private_->seek_table->num_points - 1; i >= 0; i--) {
			if(decoder->private_->seek_table->points[i].sample_number != FLAC__STREAM_METADATA_SEEKPOINT_PLACEHOLDER && decoder->private_->seek_table->points[i].sample_number <= target_sample)
				break;
		}
		if(i >= 0) { /* i.e. we found a suitable seek point... */
			lower_bound = first_frame_offset + decoder->private_->seek_table->points[i].stream_offset;
			lower_seek_point = i;
		}

		/* find the closest seek point > target_sample, if it exists */
		for(i = 0; i < (int)decoder->private_->seek_table->num_points; i++) {
			if(decoder->private_->seek_table->points[i].sample_number != FLAC__STREAM_METADATA_SEEKPOINT_PLACEHOLDER && decoder->private_->seek_table->points[i].sample_number > target_sample)
				break;
		}
		if(i < (int)decoder->private_->seek_table->num_points) { /* i.e. we found a suitable seek point... */
			upper_bound = first_frame_offset + decoder->private_->seek_table->points[i].stream_offset;
			upper_seek_point = i;
		}
	}

	/*
	 * Now guess at where within those bounds our target
	 * sample will be.
	 */
	if(lower_seek_point >= 0) {
		/* first see if our sample is within a few frames of the lower seekpoint */
		if(decoder->private_->seek_table->points[lower_seek_point].sample_number <= target_sample && target_sample < decoder->private_->seek_table->points[lower_seek_point].sample_number + (decoder->private_->seek_table->points[lower_seek_point].frame_samples * 4)) {
			pos = (FLAC__int64)lower_bound;
		}
		else if(upper_seek_point >= 0) {
			const FLAC__uint64 target_offset = target_sample - decoder->private_->seek_table->points[lower_seek_point].sample_number;
			const FLAC__uint64 range_samples = decoder->private_->seek_table->points[upper_seek_point].sample_number - decoder->private_->seek_table->points[lower_seek_point].sample_number;
			const FLAC__uint64 range_bytes = upper_bound - lower_bound;
#if defined _MSC_VER || defined __MINGW32__
			/* with VC++ you have to spoon feed it the casting */
			pos = (FLAC__int64)lower_bound + (FLAC__int64)((double)(FLAC__int64)target_offset / (double)(FLAC__int64)range_samples * (double)(FLAC__int64)(range_bytes-1)) - approx_bytes_per_frame;
#else
			pos = (FLAC__int64)lower_bound + (FLAC__int64)((double)target_offset / (double)range_samples * (double)(range_bytes-1)) - approx_bytes_per_frame;
#endif
		}
	}

	/*
	 * If there's no seek table, we need to use the metadata (if we
	 * have it) and the filelength to estimate the position of the
	 * frame with the correct sample.
	 */
	if(pos < 0 && total_samples > 0) {
#if defined _MSC_VER || defined __MINGW32__
		/* with VC++ you have to spoon feed it the casting */
		pos = (FLAC__int64)first_frame_offset + (FLAC__int64)((double)(FLAC__int64)target_sample / (double)(FLAC__int64)total_samples * (double)(FLAC__int64)(stream_length-first_frame_offset-1)) - approx_bytes_per_frame;
#else
		pos = (FLAC__int64)first_frame_offset + (FLAC__int64)((double)target_sample / (double)total_samples * (double)(stream_length-first_frame_offset-1)) - approx_bytes_per_frame;
#endif
	}

	/*
	 * If there's no seek table and total_samples is unknown, we
	 * don't even bother trying to figure out a target, we just use
	 * our current position.
	 */
	if(pos < 0) {
		FLAC__uint64 upos;
		if(decoder->private_->tell_callback(decoder, &upos, decoder->private_->client_data) != FLAC__SEEKABLE_STREAM_DECODER_TELL_STATUS_OK) {
			decoder->protected_->state = FLAC__SEEKABLE_STREAM_DECODER_SEEK_ERROR;
			return false;
		}
		pos = (FLAC__int32)upos;
		needs_seek = false;
	}
	else
		needs_seek = true;

	/* clip the position to the bounds, lower bound takes precedence */
	if(pos >= (FLAC__int64)upper_bound) {
		pos = (FLAC__int64)upper_bound-1;
		needs_seek = true;
	}
	if(pos < (FLAC__int64)lower_bound) {
		pos = (FLAC__int64)lower_bound;
		needs_seek = true;
	}

	decoder->private_->target_sample = target_sample;
	while(1) {
		if(needs_seek) {
			if(decoder->private_->seek_callback(decoder, (FLAC__uint64)pos, decoder->private_->client_data) != FLAC__SEEKABLE_STREAM_DECODER_SEEK_STATUS_OK) {
				decoder->protected_->state = FLAC__SEEKABLE_STREAM_DECODER_SEEK_ERROR;
				return false;
			}
			if(!FLAC__stream_decoder_flush(decoder->private_->stream_decoder)) {
				decoder->protected_->state = FLAC__SEEKABLE_STREAM_DECODER_STREAM_DECODER_ERROR;
				return false;
			}
		}
		if(!FLAC__stream_decoder_process_single(decoder->private_->stream_decoder)) {
			decoder->protected_->state = FLAC__SEEKABLE_STREAM_DECODER_SEEK_ERROR;
			return false;
		}
		/* our write callback will change the state when it gets to the target frame */
		if(decoder->protected_->state != FLAC__SEEKABLE_STREAM_DECODER_SEEKING) {
			break;
		}
		else { /* we need to narrow the search */
			FLAC__uint64 this_frame_sample = decoder->private_->last_frame.header.number.sample_number;
			FLAC__ASSERT(decoder->private_->last_frame.header.number_type == FLAC__FRAME_NUMBER_TYPE_SAMPLE_NUMBER);
			if(this_frame_sample == last_frame_sample) {
				/* our last move backwards wasn't big enough */
				pos -= (last_pos - pos);
				needs_seek = true;
			}
			else {
				if(target_sample < this_frame_sample) {
					last_pos = pos;
					approx_bytes_per_frame = decoder->private_->last_frame.header.blocksize * channels * bps/8 + 64;
					pos -= approx_bytes_per_frame;
					needs_seek = true;
				}
				else { /* target_sample >= this_frame_sample + this frame's blocksize */
					FLAC__uint64 upos;
					if(decoder->private_->tell_callback(decoder, &upos, decoder->private_->client_data) != FLAC__SEEKABLE_STREAM_DECODER_TELL_STATUS_OK) {
						decoder->protected_->state = FLAC__SEEKABLE_STREAM_DECODER_SEEK_ERROR;
						return false;
					}
					last_pos = pos;
					pos = (FLAC__int32)upos;
					pos -= FLAC__stream_decoder_get_input_bytes_unconsumed(decoder->private_->stream_decoder);
					needs_seek = false;
				}
			}
			if(pos < (FLAC__int64)lower_bound)
				pos = (FLAC__int64)lower_bound;
			last_frame_sample = this_frame_sample;
		}
	}

	return true;
}
