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
#include <string.h> /* for strcmp() */
#include <sys/stat.h> /* for stat() */
#if defined _MSC_VER || defined __MINGW32__
#include <io.h> /* for _setmode() */
#include <fcntl.h> /* for _O_BINARY */
#elif defined __CYGWIN__
#include <io.h> /* for _setmode(), O_BINARY */
#endif
#include "FLAC/assert.h"
#include "protected/file_decoder.h"
#include "protected/seekable_stream_decoder.h"
#include "private/md5.h"

/***********************************************************************
 *
 * Private class method prototypes
 *
 ***********************************************************************/

static FILE *get_binary_stdin_();
static FLAC__SeekableStreamDecoderReadStatus read_callback_(const FLAC__SeekableStreamDecoder *decoder, FLAC__byte buffer[], unsigned *bytes, void *client_data);
static FLAC__SeekableStreamDecoderSeekStatus seek_callback_(const FLAC__SeekableStreamDecoder *decoder, FLAC__uint64 absolute_byte_offset, void *client_data);
static FLAC__SeekableStreamDecoderTellStatus tell_callback_(const FLAC__SeekableStreamDecoder *decoder, FLAC__uint64 *absolute_byte_offset, void *client_data);
static FLAC__SeekableStreamDecoderLengthStatus length_callback_(const FLAC__SeekableStreamDecoder *decoder, FLAC__uint64 *stream_length, void *client_data);
static FLAC__bool eof_callback_(const FLAC__SeekableStreamDecoder *decoder, void *client_data);
static FLAC__StreamDecoderWriteStatus write_callback_(const FLAC__SeekableStreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 *buffer[], void *client_data);
static void metadata_callback_(const FLAC__SeekableStreamDecoder *decoder, const FLAC__StreamMetaData *metadata, void *client_data);
static void error_callback_(const FLAC__SeekableStreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data);

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
	FLAC__SeekableStreamDecoder *seekable_stream_decoder;
	struct {
		FLAC__bool md5_checking;
	} init_values_for_superclass;
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
	decoder->protected_ = (FLAC__FileDecoderProtected*)malloc(sizeof(FLAC__FileDecoderProtected));
	if(decoder->protected_ == 0) {
		free(decoder);
		return 0;
	}
	decoder->private_ = (FLAC__FileDecoderPrivate*)malloc(sizeof(FLAC__FileDecoderPrivate));
	if(decoder->private_ == 0) {
		free(decoder->protected_);
		free(decoder);
		return 0;
	}

	decoder->protected_->state = FLAC__FILE_DECODER_UNINITIALIZED;

	decoder->private_->filename = 0;
	decoder->private_->write_callback = 0;
	decoder->private_->metadata_callback = 0;
	decoder->private_->error_callback = 0;
	decoder->private_->client_data = 0;
	decoder->private_->init_values_for_superclass.md5_checking = false;

	return decoder;
}

void FLAC__file_decoder_delete(FLAC__FileDecoder *decoder)
{
	FLAC__ASSERT(decoder != 0);
	FLAC__ASSERT(decoder->protected_ != 0);
	FLAC__ASSERT(decoder->private_ != 0);

	free(decoder->private_);
	free(decoder->protected_);
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

	if(decoder->protected_->state != FLAC__FILE_DECODER_UNINITIALIZED)
		return decoder->protected_->state = FLAC__FILE_DECODER_ALREADY_INITIALIZED;

	decoder->protected_->state = FLAC__FILE_DECODER_OK;

	if(0 == decoder->private_->write_callback || 0 == decoder->private_->metadata_callback || 0 == decoder->private_->error_callback)
		return decoder->protected_->state = FLAC__FILE_DECODER_INVALID_CALLBACK;

	decoder->private_->file = 0;
	decoder->private_->seekable_stream_decoder = 0;

	if(0 == decoder->private_->filename)
		decoder->private_->file = get_binary_stdin_();
	else
		decoder->private_->file = fopen(decoder->private_->filename, "rb");

	if(decoder->private_->file == 0)
		return decoder->protected_->state = FLAC__FILE_DECODER_ERROR_OPENING_FILE;

	decoder->private_->seekable_stream_decoder = FLAC__seekable_stream_decoder_new();

	if(0 == decoder->private_->seekable_stream_decoder)
		return decoder->protected_->state = FLAC__FILE_DECODER_MEMORY_ALLOCATION_ERROR;

	FLAC__seekable_stream_decoder_set_read_callback(decoder->private_->seekable_stream_decoder, read_callback_);
	FLAC__seekable_stream_decoder_set_seek_callback(decoder->private_->seekable_stream_decoder, seek_callback_);
	FLAC__seekable_stream_decoder_set_tell_callback(decoder->private_->seekable_stream_decoder, tell_callback_);
	FLAC__seekable_stream_decoder_set_length_callback(decoder->private_->seekable_stream_decoder, length_callback_);
	FLAC__seekable_stream_decoder_set_eof_callback(decoder->private_->seekable_stream_decoder, eof_callback_);
	FLAC__seekable_stream_decoder_set_write_callback(decoder->private_->seekable_stream_decoder, write_callback_);
	FLAC__seekable_stream_decoder_set_metadata_callback(decoder->private_->seekable_stream_decoder, metadata_callback_);
	FLAC__seekable_stream_decoder_set_error_callback(decoder->private_->seekable_stream_decoder, error_callback_);
	FLAC__seekable_stream_decoder_set_client_data(decoder->private_->seekable_stream_decoder, decoder);

	/*
	 * Unfortunately, because of the "_new() ... _set_() ... _init()" order of
	 * decoder initialization, settings that are 'inherited' from the superclass
	 * have to be passed up this way, because the superclass has not even been
	 * created yet when the value is set in the subclass.
	 */
	(void)FLAC__seekable_stream_decoder_set_md5_checking(decoder->private_->seekable_stream_decoder, decoder->private_->init_values_for_superclass.md5_checking);

	if(FLAC__seekable_stream_decoder_init(decoder->private_->seekable_stream_decoder) != FLAC__SEEKABLE_STREAM_DECODER_OK)
		return decoder->protected_->state = FLAC__FILE_DECODER_STREAM_DECODER_ERROR; /*@@@ change this to FLAC__FILE_DECODER_SEEKABLE_STREAM_ERROR in next minor-revision */

	return decoder->protected_->state;
}

FLAC__bool FLAC__file_decoder_finish(FLAC__FileDecoder *decoder)
{
	FLAC__bool ok = true;

	FLAC__ASSERT(decoder != 0);
	if(decoder->protected_->state == FLAC__FILE_DECODER_UNINITIALIZED)
		return true;
	if(decoder->private_->file != 0 && decoder->private_->file != stdin)
		fclose(decoder->private_->file);
	if(0 != decoder->private_->filename) {
		free(decoder->private_->filename);
		decoder->private_->filename = 0;
	}
	if(decoder->private_->seekable_stream_decoder != 0) {
		ok = FLAC__seekable_stream_decoder_finish(decoder->private_->seekable_stream_decoder);
		FLAC__seekable_stream_decoder_delete(decoder->private_->seekable_stream_decoder);
	}
	decoder->protected_->state = FLAC__FILE_DECODER_UNINITIALIZED;
	return ok;
}

FLAC__bool FLAC__file_decoder_set_md5_checking(const FLAC__FileDecoder *decoder, FLAC__bool value)
{
	if(decoder->protected_->state != FLAC__FILE_DECODER_UNINITIALIZED)
		return false;
	decoder->private_->init_values_for_superclass.md5_checking = value;
	return true;
}

FLAC__bool FLAC__file_decoder_set_filename(const FLAC__FileDecoder *decoder, const char *value)
{
	FLAC__ASSERT(value != 0);
	if(decoder->protected_->state != FLAC__FILE_DECODER_UNINITIALIZED)
		return false;
	if(0 != decoder->private_->filename) {
		free(decoder->private_->filename);
		decoder->private_->filename = 0;
	}
	if(0 != strcmp(value, "-")) {
		if(0 == (decoder->private_->filename = (char*)malloc(strlen(value)+1))) {
			decoder->protected_->state = FLAC__FILE_DECODER_MEMORY_ALLOCATION_ERROR;
			return false;
		}
		strcpy(decoder->private_->filename, value);
	}
	return true;
}

FLAC__bool FLAC__file_decoder_set_write_callback(const FLAC__FileDecoder *decoder, FLAC__StreamDecoderWriteStatus (*value)(const FLAC__FileDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 *buffer[], void *client_data))
{
	if(decoder->protected_->state != FLAC__FILE_DECODER_UNINITIALIZED)
		return false;
	decoder->private_->write_callback = value;
	return true;
}

FLAC__bool FLAC__file_decoder_set_metadata_callback(const FLAC__FileDecoder *decoder, void (*value)(const FLAC__FileDecoder *decoder, const FLAC__StreamMetaData *metadata, void *client_data))
{
	if(decoder->protected_->state != FLAC__FILE_DECODER_UNINITIALIZED)
		return false;
	decoder->private_->metadata_callback = value;
	return true;
}

FLAC__bool FLAC__file_decoder_set_error_callback(const FLAC__FileDecoder *decoder, void (*value)(const FLAC__FileDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data))
{
	if(decoder->protected_->state != FLAC__FILE_DECODER_UNINITIALIZED)
		return false;
	decoder->private_->error_callback = value;
	return true;
}

FLAC__bool FLAC__file_decoder_set_client_data(const FLAC__FileDecoder *decoder, void *value)
{
	if(decoder->protected_->state != FLAC__FILE_DECODER_UNINITIALIZED)
		return false;
	decoder->private_->client_data = value;
	return true;
}

FLAC__FileDecoderState FLAC__file_decoder_get_state(const FLAC__FileDecoder *decoder)
{
	return decoder->protected_->state;
}

FLAC__bool FLAC__file_decoder_get_md5_checking(const FLAC__FileDecoder *decoder)
{
	return FLAC__seekable_stream_decoder_get_md5_checking(decoder->private_->seekable_stream_decoder);
}

FLAC__bool FLAC__file_decoder_process_whole_file(FLAC__FileDecoder *decoder)
{
	FLAC__bool ret;
	FLAC__ASSERT(decoder != 0);

	if(decoder->private_->seekable_stream_decoder->protected_->state == FLAC__SEEKABLE_STREAM_DECODER_END_OF_STREAM)
		decoder->protected_->state = FLAC__FILE_DECODER_END_OF_FILE;

	if(decoder->protected_->state == FLAC__FILE_DECODER_END_OF_FILE)
		return true;

	FLAC__ASSERT(decoder->protected_->state == FLAC__FILE_DECODER_OK);

	ret = FLAC__seekable_stream_decoder_process_whole_stream(decoder->private_->seekable_stream_decoder);
	if(!ret)
		decoder->protected_->state = FLAC__FILE_DECODER_SEEKABLE_STREAM_ERROR;

	return ret;
}

FLAC__bool FLAC__file_decoder_process_metadata(FLAC__FileDecoder *decoder)
{
	FLAC__bool ret;
	FLAC__ASSERT(decoder != 0);

	if(decoder->private_->seekable_stream_decoder->protected_->state == FLAC__SEEKABLE_STREAM_DECODER_END_OF_STREAM)
		decoder->protected_->state = FLAC__FILE_DECODER_END_OF_FILE;

	if(decoder->protected_->state == FLAC__FILE_DECODER_END_OF_FILE)
		return true;

	FLAC__ASSERT(decoder->protected_->state == FLAC__FILE_DECODER_OK);

	ret = FLAC__seekable_stream_decoder_process_metadata(decoder->private_->seekable_stream_decoder);
	if(!ret)
		decoder->protected_->state = FLAC__FILE_DECODER_SEEKABLE_STREAM_ERROR;

	return ret;
}

FLAC__bool FLAC__file_decoder_process_one_frame(FLAC__FileDecoder *decoder)
{
	FLAC__bool ret;
	FLAC__ASSERT(decoder != 0);

	if(decoder->private_->seekable_stream_decoder->protected_->state == FLAC__SEEKABLE_STREAM_DECODER_END_OF_STREAM)
		decoder->protected_->state = FLAC__FILE_DECODER_END_OF_FILE;

	if(decoder->protected_->state == FLAC__FILE_DECODER_END_OF_FILE)
		return true;

	FLAC__ASSERT(decoder->protected_->state == FLAC__FILE_DECODER_OK);

	ret = FLAC__seekable_stream_decoder_process_one_frame(decoder->private_->seekable_stream_decoder);
	if(!ret)
		decoder->protected_->state = FLAC__FILE_DECODER_SEEKABLE_STREAM_ERROR;

	return ret;
}

FLAC__bool FLAC__file_decoder_process_remaining_frames(FLAC__FileDecoder *decoder)
{
	FLAC__bool ret;
	FLAC__ASSERT(decoder != 0);

	if(decoder->private_->seekable_stream_decoder->protected_->state == FLAC__SEEKABLE_STREAM_DECODER_END_OF_STREAM)
		decoder->protected_->state = FLAC__FILE_DECODER_END_OF_FILE;

	if(decoder->protected_->state == FLAC__FILE_DECODER_END_OF_FILE)
		return true;

	FLAC__ASSERT(decoder->protected_->state == FLAC__FILE_DECODER_OK);

	ret = FLAC__seekable_stream_decoder_process_remaining_frames(decoder->private_->seekable_stream_decoder);
	if(!ret)
		decoder->protected_->state = FLAC__FILE_DECODER_SEEKABLE_STREAM_ERROR;

	return ret;
}

FLAC__bool FLAC__file_decoder_seek_absolute(FLAC__FileDecoder *decoder, FLAC__uint64 sample)
{
	FLAC__ASSERT(decoder != 0);
	FLAC__ASSERT(decoder->protected_->state == FLAC__FILE_DECODER_OK || decoder->protected_->state == FLAC__FILE_DECODER_END_OF_FILE);

	if(decoder->private_->filename == 0) { /* means the file is stdin... */
		decoder->protected_->state = FLAC__FILE_DECODER_SEEK_ERROR;
		return false;
	}

	if(!FLAC__seekable_stream_decoder_seek_absolute(decoder->private_->seekable_stream_decoder, sample)) {
		decoder->protected_->state = FLAC__FILE_DECODER_SEEK_ERROR;
		return false;
	}
	else {
		decoder->protected_->state = FLAC__FILE_DECODER_OK;
		return true;
	}
}

/***********************************************************************
 *
 * Private class methods
 *
 ***********************************************************************/

/*
 * This will forcibly set stdin to binary mode (for OSes that require it)
 */
FILE *get_binary_stdin_()
{
	/* if something breaks here it is probably due to the presence or
	 * absence of an underscore before the identifiers 'setmode',
	 * 'fileno', and/or 'O_BINARY'; check your system header files.
	 */
#if defined _MSC_VER || defined __MINGW32__
	_setmode(_fileno(stdin), _O_BINARY);
#elif defined __CYGWIN__
	/* almost certainly not needed for any modern Cygwin, but let's be safe... */
	setmode(_fileno(stdin), _O_BINARY);
#endif

	return stdin;
}

FLAC__SeekableStreamDecoderReadStatus read_callback_(const FLAC__SeekableStreamDecoder *decoder, FLAC__byte buffer[], unsigned *bytes, void *client_data)
{
	FLAC__FileDecoder *file_decoder = (FLAC__FileDecoder *)client_data;
	(void)decoder;

	if(*bytes > 0) {
		size_t bytes_read = fread(buffer, sizeof(FLAC__byte), *bytes, file_decoder->private_->file);
		if(bytes_read == 0 && !feof(file_decoder->private_->file)) {
			return FLAC__SEEKABLE_STREAM_DECODER_READ_STATUS_ERROR;
		}
		else {
			*bytes = (unsigned)bytes_read;
			return FLAC__SEEKABLE_STREAM_DECODER_READ_STATUS_OK;
		}
	}
	else
		return FLAC__SEEKABLE_STREAM_DECODER_READ_STATUS_ERROR; /* abort to avoid a deadlock */
}

FLAC__SeekableStreamDecoderSeekStatus seek_callback_(const FLAC__SeekableStreamDecoder *decoder, FLAC__uint64 absolute_byte_offset, void *client_data)
{
	FLAC__FileDecoder *file_decoder = (FLAC__FileDecoder *)client_data;
	(void)decoder;

	if(fseek(file_decoder->private_->file, (long)absolute_byte_offset, SEEK_SET) < 0)
		return FLAC__SEEKABLE_STREAM_DECODER_SEEK_STATUS_ERROR;
	else
		return FLAC__SEEKABLE_STREAM_DECODER_SEEK_STATUS_OK;
}

FLAC__SeekableStreamDecoderTellStatus tell_callback_(const FLAC__SeekableStreamDecoder *decoder, FLAC__uint64 *absolute_byte_offset, void *client_data)
{
	FLAC__FileDecoder *file_decoder = (FLAC__FileDecoder *)client_data;
	long pos;
	(void)decoder;

	if((pos = ftell(file_decoder->private_->file)) < 0)
		return FLAC__SEEKABLE_STREAM_DECODER_TELL_STATUS_ERROR;
	else {
		*absolute_byte_offset = (FLAC__uint64)pos;
		return FLAC__SEEKABLE_STREAM_DECODER_TELL_STATUS_OK;
	}
}

FLAC__SeekableStreamDecoderLengthStatus length_callback_(const FLAC__SeekableStreamDecoder *decoder, FLAC__uint64 *stream_length, void *client_data)
{
	FLAC__FileDecoder *file_decoder = (FLAC__FileDecoder *)client_data;
	struct stat filestats;
	(void)decoder;

	if(0 == file_decoder->private_->filename || stat(file_decoder->private_->filename, &filestats) != 0)
		return FLAC__SEEKABLE_STREAM_DECODER_LENGTH_STATUS_ERROR;
	else {
		*stream_length = (FLAC__uint64)filestats.st_size;
		return FLAC__SEEKABLE_STREAM_DECODER_LENGTH_STATUS_OK;
	}
}

FLAC__bool eof_callback_(const FLAC__SeekableStreamDecoder *decoder, void *client_data)
{
	FLAC__FileDecoder *file_decoder = (FLAC__FileDecoder *)client_data;
	(void)decoder;

	return feof(file_decoder->private_->file);
}

FLAC__StreamDecoderWriteStatus write_callback_(const FLAC__SeekableStreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 *buffer[], void *client_data)
{
	FLAC__FileDecoder *file_decoder = (FLAC__FileDecoder *)client_data;
	(void)decoder;

	return file_decoder->private_->write_callback(file_decoder, frame, buffer, file_decoder->private_->client_data);
}

void metadata_callback_(const FLAC__SeekableStreamDecoder *decoder, const FLAC__StreamMetaData *metadata, void *client_data)
{
	FLAC__FileDecoder *file_decoder = (FLAC__FileDecoder *)client_data;
	(void)decoder;

	file_decoder->private_->metadata_callback(file_decoder, metadata, file_decoder->private_->client_data);
}

void error_callback_(const FLAC__SeekableStreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
	FLAC__FileDecoder *file_decoder = (FLAC__FileDecoder *)client_data;
	(void)decoder;

	file_decoder->private_->error_callback(file_decoder, status, file_decoder->private_->client_data);
}
