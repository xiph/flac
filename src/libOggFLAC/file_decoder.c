/* libOggFLAC - Free Lossless Audio Codec + Ogg library
 * Copyright (C) 2002,2003,2004  Josh Coalson
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
#include <stdlib.h> /* for malloc() */
#include <string.h> /* for strcmp() */
#include <sys/stat.h> /* for stat() */
#if defined _MSC_VER || defined __MINGW32__
#include <io.h> /* for _setmode() */
#include <fcntl.h> /* for _O_BINARY */
#elif defined __CYGWIN__
#include <io.h> /* for setmode(), O_BINARY */
#include <fcntl.h> /* for _O_BINARY */
#endif
#include "FLAC/assert.h"
#include "protected/file_decoder.h"
#include "protected/seekable_stream_decoder.h"

/***********************************************************************
 *
 * Private class method prototypes
 *
 ***********************************************************************/

static void set_defaults_(OggFLAC__FileDecoder *decoder);
static FILE *get_binary_stdin_();
static FLAC__SeekableStreamDecoderReadStatus read_callback_(const OggFLAC__SeekableStreamDecoder *decoder, FLAC__byte buffer[], unsigned *bytes, void *client_data);
static FLAC__SeekableStreamDecoderSeekStatus seek_callback_(const OggFLAC__SeekableStreamDecoder *decoder, FLAC__uint64 absolute_byte_offset, void *client_data);
static FLAC__SeekableStreamDecoderTellStatus tell_callback_(const OggFLAC__SeekableStreamDecoder *decoder, FLAC__uint64 *absolute_byte_offset, void *client_data);
static FLAC__SeekableStreamDecoderLengthStatus length_callback_(const OggFLAC__SeekableStreamDecoder *decoder, FLAC__uint64 *stream_length, void *client_data);
static FLAC__bool eof_callback_(const OggFLAC__SeekableStreamDecoder *decoder, void *client_data);
static FLAC__StreamDecoderWriteStatus write_callback_(const OggFLAC__SeekableStreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data);
static void metadata_callback_(const OggFLAC__SeekableStreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data);
static void error_callback_(const OggFLAC__SeekableStreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data);

/***********************************************************************
 *
 * Private class data
 *
 ***********************************************************************/

typedef struct OggFLAC__FileDecoderPrivate {
	OggFLAC__FileDecoderWriteCallback write_callback;
	OggFLAC__FileDecoderMetadataCallback metadata_callback;
	OggFLAC__FileDecoderErrorCallback error_callback;
	void *client_data;
	FILE *file;
	char *filename; /* == NULL if stdin */
	OggFLAC__SeekableStreamDecoder *seekable_stream_decoder;
} OggFLAC__FileDecoderPrivate;

/***********************************************************************
 *
 * Public static class data
 *
 ***********************************************************************/

OggFLAC_API const char * const OggFLAC__FileDecoderStateString[] = {
	"OggFLAC__FILE_DECODER_OK",
	"OggFLAC__FILE_DECODER_END_OF_FILE",
	"OggFLAC__FILE_DECODER_ERROR_OPENING_FILE",
	"OggFLAC__FILE_DECODER_SEEK_ERROR",
	"OggFLAC__FILE_DECODER_SEEKABLE_STREAM_DECODER_ERROR",
	"OggFLAC__FILE_DECODER_INVALID_CALLBACK",
	"OggFLAC__FILE_DECODER_MEMORY_ALLOCATION_ERROR",
	"OggFLAC__FILE_DECODER_ALREADY_INITIALIZED",
	"OggFLAC__FILE_DECODER_UNINITIALIZED"
};

/***********************************************************************
 *
 * Class constructor/destructor
 *
 ***********************************************************************/

OggFLAC_API OggFLAC__FileDecoder *OggFLAC__file_decoder_new()
{
	OggFLAC__FileDecoder *decoder;

	FLAC__ASSERT(sizeof(int) >= 4); /* we want to die right away if this is not true */

	decoder = (OggFLAC__FileDecoder*)calloc(1, sizeof(OggFLAC__FileDecoder));
	if(decoder == 0) {
		return 0;
	}

	decoder->protected_ = (OggFLAC__FileDecoderProtected*)calloc(1, sizeof(OggFLAC__FileDecoderProtected));
	if(decoder->protected_ == 0) {
		free(decoder);
		return 0;
	}

	decoder->private_ = (OggFLAC__FileDecoderPrivate*)calloc(1, sizeof(OggFLAC__FileDecoderPrivate));
	if(decoder->private_ == 0) {
		free(decoder->protected_);
		free(decoder);
		return 0;
	}

	decoder->private_->seekable_stream_decoder = OggFLAC__seekable_stream_decoder_new();
	if(0 == decoder->private_->seekable_stream_decoder) {
		free(decoder->private_);
		free(decoder->protected_);
		free(decoder);
		return 0;
	}

	decoder->private_->file = 0;

	set_defaults_(decoder);

	decoder->protected_->state = OggFLAC__FILE_DECODER_UNINITIALIZED;

	return decoder;
}

OggFLAC_API void OggFLAC__file_decoder_delete(OggFLAC__FileDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->protected_);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->private_->seekable_stream_decoder);

	(void)OggFLAC__file_decoder_finish(decoder);

	OggFLAC__seekable_stream_decoder_delete(decoder->private_->seekable_stream_decoder);

	free(decoder->private_);
	free(decoder->protected_);
	free(decoder);
}

/***********************************************************************
 *
 * Public class methods
 *
 ***********************************************************************/

OggFLAC_API OggFLAC__FileDecoderState OggFLAC__file_decoder_init(OggFLAC__FileDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);

	if(decoder->protected_->state != OggFLAC__FILE_DECODER_UNINITIALIZED)
		return decoder->protected_->state = OggFLAC__FILE_DECODER_ALREADY_INITIALIZED;

	if(0 == decoder->private_->write_callback || 0 == decoder->private_->metadata_callback || 0 == decoder->private_->error_callback)
		return decoder->protected_->state = OggFLAC__FILE_DECODER_INVALID_CALLBACK;

	if(0 == decoder->private_->filename)
		decoder->private_->file = get_binary_stdin_();
	else
		decoder->private_->file = fopen(decoder->private_->filename, "rb");

	if(decoder->private_->file == 0)
		return decoder->protected_->state = OggFLAC__FILE_DECODER_ERROR_OPENING_FILE;

	OggFLAC__seekable_stream_decoder_set_read_callback(decoder->private_->seekable_stream_decoder, read_callback_);
	OggFLAC__seekable_stream_decoder_set_seek_callback(decoder->private_->seekable_stream_decoder, seek_callback_);
	OggFLAC__seekable_stream_decoder_set_tell_callback(decoder->private_->seekable_stream_decoder, tell_callback_);
	OggFLAC__seekable_stream_decoder_set_length_callback(decoder->private_->seekable_stream_decoder, length_callback_);
	OggFLAC__seekable_stream_decoder_set_eof_callback(decoder->private_->seekable_stream_decoder, eof_callback_);
	OggFLAC__seekable_stream_decoder_set_write_callback(decoder->private_->seekable_stream_decoder, write_callback_);
	OggFLAC__seekable_stream_decoder_set_metadata_callback(decoder->private_->seekable_stream_decoder, metadata_callback_);
	OggFLAC__seekable_stream_decoder_set_error_callback(decoder->private_->seekable_stream_decoder, error_callback_);
	OggFLAC__seekable_stream_decoder_set_client_data(decoder->private_->seekable_stream_decoder, decoder);

	if(OggFLAC__seekable_stream_decoder_init(decoder->private_->seekable_stream_decoder) != OggFLAC__SEEKABLE_STREAM_DECODER_OK)
		return decoder->protected_->state = OggFLAC__FILE_DECODER_SEEKABLE_STREAM_DECODER_ERROR;

	return decoder->protected_->state = OggFLAC__FILE_DECODER_OK;
}

OggFLAC_API FLAC__bool OggFLAC__file_decoder_finish(OggFLAC__FileDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);

	if(decoder->protected_->state == OggFLAC__FILE_DECODER_UNINITIALIZED)
		return true;

	FLAC__ASSERT(0 != decoder->private_->seekable_stream_decoder);

	if(0 != decoder->private_->file && decoder->private_->file != stdin) {
		fclose(decoder->private_->file);
		decoder->private_->file = 0;
	}

	if(0 != decoder->private_->filename) {
		free(decoder->private_->filename);
		decoder->private_->filename = 0;
	}

	set_defaults_(decoder);

	decoder->protected_->state = OggFLAC__FILE_DECODER_UNINITIALIZED;

	return OggFLAC__seekable_stream_decoder_finish(decoder->private_->seekable_stream_decoder);
}

OggFLAC_API FLAC__bool OggFLAC__file_decoder_set_md5_checking(OggFLAC__FileDecoder *decoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	FLAC__ASSERT(0 != decoder->private_->seekable_stream_decoder);
	if(decoder->protected_->state != OggFLAC__FILE_DECODER_UNINITIALIZED)
		return false;
	return OggFLAC__seekable_stream_decoder_set_md5_checking(decoder->private_->seekable_stream_decoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__file_decoder_set_filename(OggFLAC__FileDecoder *decoder, const char *value)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	FLAC__ASSERT(0 != value);
	if(decoder->protected_->state != OggFLAC__FILE_DECODER_UNINITIALIZED)
		return false;
	if(0 != decoder->private_->filename) {
		free(decoder->private_->filename);
		decoder->private_->filename = 0;
	}
	if(0 != strcmp(value, "-")) {
		if(0 == (decoder->private_->filename = (char*)malloc(strlen(value)+1))) {
			decoder->protected_->state = OggFLAC__FILE_DECODER_MEMORY_ALLOCATION_ERROR;
			return false;
		}
		strcpy(decoder->private_->filename, value);
	}
	return true;
}

OggFLAC_API FLAC__bool OggFLAC__file_decoder_set_write_callback(OggFLAC__FileDecoder *decoder, OggFLAC__FileDecoderWriteCallback value)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	if(decoder->protected_->state != OggFLAC__FILE_DECODER_UNINITIALIZED)
		return false;
	decoder->private_->write_callback = value;
	return true;
}

OggFLAC_API FLAC__bool OggFLAC__file_decoder_set_metadata_callback(OggFLAC__FileDecoder *decoder, OggFLAC__FileDecoderMetadataCallback value)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	if(decoder->protected_->state != OggFLAC__FILE_DECODER_UNINITIALIZED)
		return false;
	decoder->private_->metadata_callback = value;
	return true;
}

OggFLAC_API FLAC__bool OggFLAC__file_decoder_set_error_callback(OggFLAC__FileDecoder *decoder, OggFLAC__FileDecoderErrorCallback value)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	if(decoder->protected_->state != OggFLAC__FILE_DECODER_UNINITIALIZED)
		return false;
	decoder->private_->error_callback = value;
	return true;
}

OggFLAC_API FLAC__bool OggFLAC__file_decoder_set_client_data(OggFLAC__FileDecoder *decoder, void *value)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	if(decoder->protected_->state != OggFLAC__FILE_DECODER_UNINITIALIZED)
		return false;
	decoder->private_->client_data = value;
	return true;
}

OggFLAC_API FLAC__bool OggFLAC__file_decoder_set_serial_number(OggFLAC__FileDecoder *decoder, long value)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	if(decoder->protected_->state != OggFLAC__FILE_DECODER_UNINITIALIZED)
		return false;
	OggFLAC__seekable_stream_decoder_set_serial_number(decoder->private_->seekable_stream_decoder, value);
	return true;
}

OggFLAC_API FLAC__bool OggFLAC__file_decoder_set_metadata_respond(OggFLAC__FileDecoder *decoder, FLAC__MetadataType type)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	FLAC__ASSERT(0 != decoder->private_->seekable_stream_decoder);
	if(decoder->protected_->state != OggFLAC__FILE_DECODER_UNINITIALIZED)
		return false;
	return OggFLAC__seekable_stream_decoder_set_metadata_respond(decoder->private_->seekable_stream_decoder, type);
}

OggFLAC_API FLAC__bool OggFLAC__file_decoder_set_metadata_respond_application(OggFLAC__FileDecoder *decoder, const FLAC__byte id[4])
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	FLAC__ASSERT(0 != decoder->private_->seekable_stream_decoder);
	if(decoder->protected_->state != OggFLAC__FILE_DECODER_UNINITIALIZED)
		return false;
	return OggFLAC__seekable_stream_decoder_set_metadata_respond_application(decoder->private_->seekable_stream_decoder, id);
}

OggFLAC_API FLAC__bool OggFLAC__file_decoder_set_metadata_respond_all(OggFLAC__FileDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	FLAC__ASSERT(0 != decoder->private_->seekable_stream_decoder);
	if(decoder->protected_->state != OggFLAC__FILE_DECODER_UNINITIALIZED)
		return false;
	return OggFLAC__seekable_stream_decoder_set_metadata_respond_all(decoder->private_->seekable_stream_decoder);
}

OggFLAC_API FLAC__bool OggFLAC__file_decoder_set_metadata_ignore(OggFLAC__FileDecoder *decoder, FLAC__MetadataType type)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	FLAC__ASSERT(0 != decoder->private_->seekable_stream_decoder);
	if(decoder->protected_->state != OggFLAC__FILE_DECODER_UNINITIALIZED)
		return false;
	return OggFLAC__seekable_stream_decoder_set_metadata_ignore(decoder->private_->seekable_stream_decoder, type);
}

OggFLAC_API FLAC__bool OggFLAC__file_decoder_set_metadata_ignore_application(OggFLAC__FileDecoder *decoder, const FLAC__byte id[4])
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	FLAC__ASSERT(0 != decoder->private_->seekable_stream_decoder);
	if(decoder->protected_->state != OggFLAC__FILE_DECODER_UNINITIALIZED)
		return false;
	return OggFLAC__seekable_stream_decoder_set_metadata_ignore_application(decoder->private_->seekable_stream_decoder, id);
}

OggFLAC_API FLAC__bool OggFLAC__file_decoder_set_metadata_ignore_all(OggFLAC__FileDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	FLAC__ASSERT(0 != decoder->private_->seekable_stream_decoder);
	if(decoder->protected_->state != OggFLAC__FILE_DECODER_UNINITIALIZED)
		return false;
	return OggFLAC__seekable_stream_decoder_set_metadata_ignore_all(decoder->private_->seekable_stream_decoder);
}

OggFLAC_API OggFLAC__FileDecoderState OggFLAC__file_decoder_get_state(const OggFLAC__FileDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->protected_);
	return decoder->protected_->state;
}

OggFLAC_API OggFLAC__SeekableStreamDecoderState OggFLAC__file_decoder_get_seekable_stream_decoder_state(const OggFLAC__FileDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	return OggFLAC__seekable_stream_decoder_get_state(decoder->private_->seekable_stream_decoder);
}

OggFLAC_API FLAC__SeekableStreamDecoderState OggFLAC__file_decoder_get_FLAC_seekable_stream_decoder_state(const OggFLAC__FileDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	return OggFLAC__seekable_stream_decoder_get_FLAC_seekable_stream_decoder_state(decoder->private_->seekable_stream_decoder);
}

OggFLAC_API FLAC__StreamDecoderState OggFLAC__file_decoder_get_FLAC_stream_decoder_state(const OggFLAC__FileDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	return OggFLAC__seekable_stream_decoder_get_FLAC_stream_decoder_state(decoder->private_->seekable_stream_decoder);
}

OggFLAC_API const char *OggFLAC__file_decoder_get_resolved_state_string(const OggFLAC__FileDecoder *decoder)
{
	if(decoder->protected_->state != OggFLAC__FILE_DECODER_SEEKABLE_STREAM_DECODER_ERROR)
		return OggFLAC__FileDecoderStateString[decoder->protected_->state];
	else
		return OggFLAC__seekable_stream_decoder_get_resolved_state_string(decoder->private_->seekable_stream_decoder);
}

OggFLAC_API FLAC__bool OggFLAC__file_decoder_get_md5_checking(const OggFLAC__FileDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	return OggFLAC__seekable_stream_decoder_get_md5_checking(decoder->private_->seekable_stream_decoder);
}

OggFLAC_API unsigned OggFLAC__file_decoder_get_channels(const OggFLAC__FileDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	return OggFLAC__seekable_stream_decoder_get_channels(decoder->private_->seekable_stream_decoder);
}

OggFLAC_API FLAC__ChannelAssignment OggFLAC__file_decoder_get_channel_assignment(const OggFLAC__FileDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	return OggFLAC__seekable_stream_decoder_get_channel_assignment(decoder->private_->seekable_stream_decoder);
}

OggFLAC_API unsigned OggFLAC__file_decoder_get_bits_per_sample(const OggFLAC__FileDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	return OggFLAC__seekable_stream_decoder_get_bits_per_sample(decoder->private_->seekable_stream_decoder);
}

OggFLAC_API unsigned OggFLAC__file_decoder_get_sample_rate(const OggFLAC__FileDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	return OggFLAC__seekable_stream_decoder_get_sample_rate(decoder->private_->seekable_stream_decoder);
}

OggFLAC_API unsigned OggFLAC__file_decoder_get_blocksize(const OggFLAC__FileDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	return OggFLAC__seekable_stream_decoder_get_blocksize(decoder->private_->seekable_stream_decoder);
}

OggFLAC_API FLAC__bool OggFLAC__file_decoder_get_decode_position(const OggFLAC__FileDecoder *decoder, FLAC__uint64 *position)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	return OggFLAC__seekable_stream_decoder_get_decode_position(decoder->private_->seekable_stream_decoder, position);
}

OggFLAC_API FLAC__bool OggFLAC__file_decoder_process_single(OggFLAC__FileDecoder *decoder)
{
	FLAC__bool ret;
	FLAC__ASSERT(0 != decoder);

	if(decoder->private_->seekable_stream_decoder->protected_->state == OggFLAC__SEEKABLE_STREAM_DECODER_END_OF_STREAM)
		decoder->protected_->state = OggFLAC__FILE_DECODER_END_OF_FILE;

	if(decoder->protected_->state == OggFLAC__FILE_DECODER_END_OF_FILE)
		return true;

	FLAC__ASSERT(decoder->protected_->state == OggFLAC__FILE_DECODER_OK);

	ret = OggFLAC__seekable_stream_decoder_process_single(decoder->private_->seekable_stream_decoder);
	if(!ret)
		decoder->protected_->state = OggFLAC__FILE_DECODER_SEEKABLE_STREAM_DECODER_ERROR;

	return ret;
}

OggFLAC_API FLAC__bool OggFLAC__file_decoder_process_until_end_of_metadata(OggFLAC__FileDecoder *decoder)
{
	FLAC__bool ret;
	FLAC__ASSERT(0 != decoder);

	if(decoder->private_->seekable_stream_decoder->protected_->state == OggFLAC__SEEKABLE_STREAM_DECODER_END_OF_STREAM)
		decoder->protected_->state = OggFLAC__FILE_DECODER_END_OF_FILE;

	if(decoder->protected_->state == OggFLAC__FILE_DECODER_END_OF_FILE)
		return true;

	FLAC__ASSERT(decoder->protected_->state == OggFLAC__FILE_DECODER_OK);

	ret = OggFLAC__seekable_stream_decoder_process_until_end_of_metadata(decoder->private_->seekable_stream_decoder);
	if(!ret)
		decoder->protected_->state = OggFLAC__FILE_DECODER_SEEKABLE_STREAM_DECODER_ERROR;

	return ret;
}

OggFLAC_API FLAC__bool OggFLAC__file_decoder_process_until_end_of_file(OggFLAC__FileDecoder *decoder)
{
	FLAC__bool ret;
	FLAC__ASSERT(0 != decoder);

	if(decoder->private_->seekable_stream_decoder->protected_->state == OggFLAC__SEEKABLE_STREAM_DECODER_END_OF_STREAM)
		decoder->protected_->state = OggFLAC__FILE_DECODER_END_OF_FILE;

	if(decoder->protected_->state == OggFLAC__FILE_DECODER_END_OF_FILE)
		return true;

	FLAC__ASSERT(decoder->protected_->state == OggFLAC__FILE_DECODER_OK);

	ret = OggFLAC__seekable_stream_decoder_process_until_end_of_stream(decoder->private_->seekable_stream_decoder);
	if(!ret)
		decoder->protected_->state = OggFLAC__FILE_DECODER_SEEKABLE_STREAM_DECODER_ERROR;

	return ret;
}

OggFLAC_API FLAC__bool OggFLAC__file_decoder_seek_absolute(OggFLAC__FileDecoder *decoder, FLAC__uint64 sample)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(decoder->protected_->state == OggFLAC__FILE_DECODER_OK || decoder->protected_->state == OggFLAC__FILE_DECODER_END_OF_FILE);

	if(decoder->private_->filename == 0) { /* means the file is stdin... */
		decoder->protected_->state = OggFLAC__FILE_DECODER_SEEK_ERROR;
		return false;
	}

	if(!OggFLAC__seekable_stream_decoder_seek_absolute(decoder->private_->seekable_stream_decoder, sample)) {
		decoder->protected_->state = OggFLAC__FILE_DECODER_SEEK_ERROR;
		return false;
	}
	else {
		decoder->protected_->state = OggFLAC__FILE_DECODER_OK;
		return true;
	}
}


/***********************************************************************
 *
 * Private class methods
 *
 ***********************************************************************/

void set_defaults_(OggFLAC__FileDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);

	decoder->private_->filename = 0;
	decoder->private_->write_callback = 0;
	decoder->private_->metadata_callback = 0;
	decoder->private_->error_callback = 0;
	decoder->private_->client_data = 0;
}

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

FLAC__SeekableStreamDecoderReadStatus read_callback_(const OggFLAC__SeekableStreamDecoder *decoder, FLAC__byte buffer[], unsigned *bytes, void *client_data)
{
	OggFLAC__FileDecoder *file_decoder = (OggFLAC__FileDecoder *)client_data;
	(void)decoder;

	if(*bytes > 0) {
		*bytes = (unsigned)fread(buffer, sizeof(FLAC__byte), *bytes, file_decoder->private_->file);
		if(ferror(file_decoder->private_->file)) {
			return FLAC__SEEKABLE_STREAM_DECODER_READ_STATUS_ERROR;
		}
		else {
			return FLAC__SEEKABLE_STREAM_DECODER_READ_STATUS_OK;
		}
	}
	else
		return FLAC__SEEKABLE_STREAM_DECODER_READ_STATUS_ERROR; /* abort to avoid a deadlock */
}

FLAC__SeekableStreamDecoderSeekStatus seek_callback_(const OggFLAC__SeekableStreamDecoder *decoder, FLAC__uint64 absolute_byte_offset, void *client_data)
{
	OggFLAC__FileDecoder *file_decoder = (OggFLAC__FileDecoder *)client_data;
	(void)decoder;

	if(fseek(file_decoder->private_->file, (long)absolute_byte_offset, SEEK_SET) < 0)
		return FLAC__SEEKABLE_STREAM_DECODER_SEEK_STATUS_ERROR;
	else
		return FLAC__SEEKABLE_STREAM_DECODER_SEEK_STATUS_OK;
}

FLAC__SeekableStreamDecoderTellStatus tell_callback_(const OggFLAC__SeekableStreamDecoder *decoder, FLAC__uint64 *absolute_byte_offset, void *client_data)
{
	OggFLAC__FileDecoder *file_decoder = (OggFLAC__FileDecoder *)client_data;
	long pos;
	(void)decoder;

	if((pos = ftell(file_decoder->private_->file)) < 0)
		return FLAC__SEEKABLE_STREAM_DECODER_TELL_STATUS_ERROR;
	else {
		*absolute_byte_offset = (FLAC__uint64)pos;
		return FLAC__SEEKABLE_STREAM_DECODER_TELL_STATUS_OK;
	}
}

FLAC__SeekableStreamDecoderLengthStatus length_callback_(const OggFLAC__SeekableStreamDecoder *decoder, FLAC__uint64 *stream_length, void *client_data)
{
	OggFLAC__FileDecoder *file_decoder = (OggFLAC__FileDecoder *)client_data;
	struct stat filestats;
	(void)decoder;

	if(0 == file_decoder->private_->filename || stat(file_decoder->private_->filename, &filestats) != 0)
		return FLAC__SEEKABLE_STREAM_DECODER_LENGTH_STATUS_ERROR;
	else {
		*stream_length = (FLAC__uint64)filestats.st_size;
		return FLAC__SEEKABLE_STREAM_DECODER_LENGTH_STATUS_OK;
	}
}

FLAC__bool eof_callback_(const OggFLAC__SeekableStreamDecoder *decoder, void *client_data)
{
	OggFLAC__FileDecoder *file_decoder = (OggFLAC__FileDecoder *)client_data;
	(void)decoder;

	return feof(file_decoder->private_->file)? true : false;
}

FLAC__StreamDecoderWriteStatus write_callback_(const OggFLAC__SeekableStreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data)
{
	OggFLAC__FileDecoder *file_decoder = (OggFLAC__FileDecoder *)client_data;
	(void)decoder;

	return file_decoder->private_->write_callback(file_decoder, frame, buffer, file_decoder->private_->client_data);
}

void metadata_callback_(const OggFLAC__SeekableStreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data)
{
	OggFLAC__FileDecoder *file_decoder = (OggFLAC__FileDecoder *)client_data;
	(void)decoder;

	file_decoder->private_->metadata_callback(file_decoder, metadata, file_decoder->private_->client_data);
}

void error_callback_(const OggFLAC__SeekableStreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
	OggFLAC__FileDecoder *file_decoder = (OggFLAC__FileDecoder *)client_data;
	(void)decoder;

	file_decoder->private_->error_callback(file_decoder, status, file_decoder->private_->client_data);
}
