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

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#if defined _MSC_VER || defined __MINGW32__
#include <io.h> /* for _setmode() */
#include <fcntl.h> /* for _O_BINARY */
#endif
#if defined __CYGWIN__ || defined __EMX__
#include <io.h> /* for setmode(), O_BINARY */
#include <fcntl.h> /* for _O_BINARY */
#endif
#include <stdio.h>
#include <stdlib.h> /* for calloc() */
#include <sys/stat.h> /* for stat() */
#include <sys/types.h> /* for off_t */
#if defined _MSC_VER || defined __MINGW32__
#if _MSC_VER <= 1200 /* @@@ [2G limit] */
#define fseeko fseek
#define ftello ftell
#endif
#endif
#include "FLAC/assert.h"
#include "protected/stream_decoder.h"
#include "../libFLAC/include/private/float.h" /* @@@ ugly hack, but how else to do?  we need to reuse the float formats but don't want to expose it */

/***********************************************************************
 *
 * Private class method prototypes
 *
 ***********************************************************************/

static void set_defaults_(OggFLAC__StreamDecoder *decoder);
static FILE *get_binary_stdin_();
static FLAC__StreamDecoderReadStatus read_callback2_(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], unsigned *bytes, void *client_data);
static FLAC__StreamDecoderReadStatus read_callback_ogg_aspect_(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], unsigned *bytes, void *client_data);
static OggFLAC__OggDecoderAspectReadStatus read_callback_proxy_(const void *void_decoder, FLAC__byte buffer[], unsigned *bytes, void *client_data);
static FLAC__StreamDecoderWriteStatus write_callback_(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data);
static void metadata_callback_(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data);
static void error_callback_(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data);
static FLAC__bool seek_to_absolute_sample_(OggFLAC__StreamDecoder *decoder, FLAC__uint64 stream_length, FLAC__uint64 target_sample);
static FLAC__StreamDecoderReadStatus file_read_callback_(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], unsigned *bytes, void *client_data);
static FLAC__StreamDecoderSeekStatus file_seek_callback_(const FLAC__StreamDecoder *decoder, FLAC__uint64 absolute_byte_offset, void *client_data);
static FLAC__StreamDecoderTellStatus file_tell_callback_(const FLAC__StreamDecoder *decoder, FLAC__uint64 *absolute_byte_offset, void *client_data);
static FLAC__StreamDecoderLengthStatus file_length_callback_(const FLAC__StreamDecoder *decoder, FLAC__uint64 *stream_length, void *client_data);
static FLAC__bool file_eof_callback_(const FLAC__StreamDecoder *decoder, void *client_data);


/***********************************************************************
 *
 * Private class data
 *
 ***********************************************************************/

typedef struct OggFLAC__StreamDecoderPrivate {
	FLAC__StreamDecoderReadCallback read_callback;
	FLAC__StreamDecoderSeekCallback seek_callback;
	FLAC__StreamDecoderTellCallback tell_callback;
	FLAC__StreamDecoderLengthCallback length_callback;
	FLAC__StreamDecoderEofCallback eof_callback;
	FLAC__StreamDecoderWriteCallback write_callback;
	FLAC__StreamDecoderMetadataCallback metadata_callback;
	FLAC__StreamDecoderErrorCallback error_callback;
	void *client_data;
	FILE *file; /* only used if OggFLAC__stream_decoder_init_file()/OggFLAC__stream_decoder_init_file() called, else NULL */
	/* the rest of these are only used for seeking: */
	FLAC__bool is_seeking;
	FLAC__Frame last_frame; /* holds the info of the last frame we seeked to */
	FLAC__uint64 target_sample;
	FLAC__bool got_a_frame; /* hack needed in seek routine to check when process_single() actually writes a frame */
} OggFLAC__StreamDecoderPrivate;

/***********************************************************************
 *
 * Public static class data
 *
 ***********************************************************************/

OggFLAC_API const char * const OggFLAC__StreamDecoderStateString[] = {
	"OggFLAC__STREAM_DECODER_OK",
	"OggFLAC__STREAM_DECODER_END_OF_STREAM",
	"OggFLAC__STREAM_DECODER_OGG_ERROR",
	"OggFLAC__STREAM_DECODER_FLAC_STREAM_DECODER_ERROR",
	"OggFLAC__STREAM_DECODER_SEEK_ERROR",
	"OggFLAC__STREAM_DECODER_READ_ERROR",
	"OggFLAC__STREAM_DECODER_MEMORY_ALLOCATION_ERROR",
	"OggFLAC__STREAM_DECODER_UNINITIALIZED"
};


/***********************************************************************
 *
 * Class constructor/destructor
 *
 ***********************************************************************/
OggFLAC_API OggFLAC__StreamDecoder *OggFLAC__stream_decoder_new()
{
	OggFLAC__StreamDecoder *decoder;
	FLAC__StreamDecoder *parent;

	FLAC__ASSERT(sizeof(int) >= 4); /* we want to die right away if this is not true */

	decoder = (OggFLAC__StreamDecoder*)calloc(1, sizeof(OggFLAC__StreamDecoder));
	if(decoder == 0) {
		return 0;
	}

	decoder->protected_ = (OggFLAC__StreamDecoderProtected*)calloc(1, sizeof(OggFLAC__StreamDecoderProtected));
	if(decoder->protected_ == 0) {
		free(decoder);
		return 0;
	}

	decoder->private_ = (OggFLAC__StreamDecoderPrivate*)calloc(1, sizeof(OggFLAC__StreamDecoderPrivate));
	if(decoder->private_ == 0) {
		free(decoder->protected_);
		free(decoder);
		return 0;
	}

	parent = FLAC__stream_decoder_new();
	if(0 == parent) {
		free(decoder->private_);
		free(decoder->protected_);
		free(decoder);
		return 0;
	}
	decoder->super_ = *parent;

	decoder->private_->file = 0;

	set_defaults_(decoder);

	decoder->protected_->state = OggFLAC__STREAM_DECODER_UNINITIALIZED;

	return decoder;
}

OggFLAC_API void OggFLAC__stream_decoder_delete(OggFLAC__StreamDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->protected_);
	FLAC__ASSERT(0 != decoder->private_);

	OggFLAC__stream_decoder_finish(decoder);

	free(decoder->private_);
	free(decoder->protected_);
	/* don't free(decoder) because FLAC__stream_decoder_delete() will do it */

	/* call superclass destructor last */
	FLAC__stream_decoder_delete((FLAC__StreamDecoder*)decoder);
}

/***********************************************************************
 *
 * Public class methods
 *
 ***********************************************************************/

OggFLAC_API FLAC__StreamDecoderInitStatus OggFLAC__stream_decoder_init_stream(
	OggFLAC__StreamDecoder *decoder,
	FLAC__StreamDecoderReadCallback read_callback,
	FLAC__StreamDecoderSeekCallback seek_callback,
	FLAC__StreamDecoderTellCallback tell_callback,
	FLAC__StreamDecoderLengthCallback length_callback,
	FLAC__StreamDecoderEofCallback eof_callback,
	FLAC__StreamDecoderWriteCallback write_callback,
	FLAC__StreamDecoderMetadataCallback metadata_callback,
	FLAC__StreamDecoderErrorCallback error_callback,
	void *client_data
)
{
	FLAC__ASSERT(0 != decoder);

	if(decoder->protected_->state != OggFLAC__STREAM_DECODER_UNINITIALIZED)
		return decoder->protected_->state = FLAC__STREAM_DECODER_INIT_STATUS_ALREADY_INITIALIZED;

	if(
		0 == read_callback ||
		0 == write_callback ||
		0 == error_callback ||
		(seek_callback && (0 == tell_callback || 0 == length_callback || 0 == eof_callback))
	)
		return FLAC__STREAM_DECODER_INIT_STATUS_INVALID_CALLBACKS;

	if(!OggFLAC__ogg_decoder_aspect_init(&decoder->protected_->ogg_decoder_aspect))
		return decoder->protected_->state = OggFLAC__STREAM_DECODER_OGG_ERROR;

	/* from here on, errors are fatal */

	decoder->private_->read_callback = read_callback;
	decoder->private_->seek_callback = seek_callback;
	decoder->private_->tell_callback = tell_callback;
	decoder->private_->length_callback = length_callback;
	decoder->private_->eof_callback = eof_callback;
	decoder->private_->write_callback = write_callback;
	decoder->private_->metadata_callback = metadata_callback;
	decoder->private_->error_callback = error_callback;
	decoder->private_->client_data = client_data;

	decoder->private_->is_seeking = false;

	if(FLAC__stream_decoder_init_stream((FLAC__StreamDecoder*)decoder, read_callback_ogg_aspect_, /*seek_callback=*/0, /*tell_callback=*/0, /*length_callback=*/0, /*eof_callback=*/0, write_callback_, metadata_callback_, error_callback_, /*client_data=*/decoder) != FLAC__STREAM_DECODER_INIT_STATUS_OK) \
		return decoder->protected_->state = OggFLAC__STREAM_DECODER_FLAC_STREAM_DECODER_ERROR;

	decoder->protected_->state = OggFLAC__STREAM_DECODER_OK;
	return FLAC__STREAM_DECODER_INIT_STATUS_OK;
}

OggFLAC_API FLAC__StreamDecoderInitStatus OggFLAC__stream_decoder_init_FILE(
	OggFLAC__StreamDecoder *decoder,
	FILE *file,
	FLAC__StreamDecoderWriteCallback write_callback,
	FLAC__StreamDecoderMetadataCallback metadata_callback,
	FLAC__StreamDecoderErrorCallback error_callback,
	void *client_data
)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != file);

	if(decoder->protected_->state != OggFLAC__STREAM_DECODER_UNINITIALIZED)
		return decoder->protected_->state = FLAC__STREAM_DECODER_INIT_STATUS_ALREADY_INITIALIZED;

	if(0 == write_callback || 0 == error_callback)
		return decoder->protected_->state = FLAC__STREAM_DECODER_INIT_STATUS_INVALID_CALLBACKS;

	/*
	 * To make sure that our file does not go unclosed after an error, we
	 * must assign the FILE pointer before any further error can occur in
	 * this routine.
	 */
	if(file == stdin)
		file = get_binary_stdin_(); /* just to be safe */

	decoder->private_->file = file;

	return OggFLAC__stream_decoder_init_stream(
		decoder,
		file_read_callback_,
		decoder->private_->file == stdin? 0: file_seek_callback_,
		decoder->private_->file == stdin? 0: file_tell_callback_,
		decoder->private_->file == stdin? 0: file_length_callback_,
		file_eof_callback_,
		write_callback,
		metadata_callback,
		error_callback,
		client_data
	);
}

OggFLAC_API FLAC__StreamDecoderInitStatus OggFLAC__stream_decoder_init_file(
	OggFLAC__StreamDecoder *decoder,
	const char *filename,
	FLAC__StreamDecoderWriteCallback write_callback,
	FLAC__StreamDecoderMetadataCallback metadata_callback,
	FLAC__StreamDecoderErrorCallback error_callback,
	void *client_data
)
{
	FILE *file;

	FLAC__ASSERT(0 != decoder);

	/*
	 * To make sure that our file does not go unclosed after an error, we
	 * have to do the same entrance checks here that are later performed
	 * in OggFLAC__stream_decoder_init_FILE() before the FILE* is assigned.
	 */
	if(decoder->protected_->state != OggFLAC__STREAM_DECODER_UNINITIALIZED)
		return decoder->protected_->state = FLAC__STREAM_DECODER_INIT_STATUS_ALREADY_INITIALIZED;

	if(0 == write_callback || 0 == error_callback)
		return decoder->protected_->state = FLAC__STREAM_DECODER_INIT_STATUS_INVALID_CALLBACKS;

	file = filename? fopen(filename, "rb") : stdin;

	if(0 == file)
		return FLAC__STREAM_DECODER_INIT_STATUS_ERROR_OPENING_FILE;

	return OggFLAC__stream_decoder_init_FILE(decoder, file, write_callback, metadata_callback, error_callback, client_data);
}

OggFLAC_API FLAC__bool OggFLAC__stream_decoder_finish(OggFLAC__StreamDecoder *decoder)
{
	FLAC__bool md5_ok;

	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);

	if(decoder->protected_->state == OggFLAC__STREAM_DECODER_UNINITIALIZED)
		return true;

	md5_ok = FLAC__stream_decoder_finish((FLAC__StreamDecoder*)decoder);

	OggFLAC__ogg_decoder_aspect_finish(&decoder->protected_->ogg_decoder_aspect);

	if(0 != decoder->private_->file) {
		if(decoder->private_->file != stdin)
			fclose(decoder->private_->file);
		decoder->private_->file = 0;
	}

	decoder->private_->is_seeking = false;

	set_defaults_(decoder);

	decoder->protected_->state = OggFLAC__STREAM_DECODER_UNINITIALIZED;

	return md5_ok;
}

OggFLAC_API FLAC__bool OggFLAC__stream_decoder_set_md5_checking(OggFLAC__StreamDecoder *decoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->protected_);
	if(decoder->protected_->state != OggFLAC__STREAM_DECODER_UNINITIALIZED)
		return false;
	return FLAC__stream_decoder_set_md5_checking((FLAC__StreamDecoder*)decoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__stream_decoder_set_serial_number(OggFLAC__StreamDecoder *decoder, long value)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	if(decoder->protected_->state != OggFLAC__STREAM_DECODER_UNINITIALIZED)
		return false;
	OggFLAC__ogg_decoder_aspect_set_serial_number(&decoder->protected_->ogg_decoder_aspect, value);
	return true;
}

OggFLAC_API FLAC__bool OggFLAC__stream_decoder_set_metadata_respond(OggFLAC__StreamDecoder *decoder, FLAC__MetadataType type)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	if(decoder->protected_->state != OggFLAC__STREAM_DECODER_UNINITIALIZED)
		return false;
	return FLAC__stream_decoder_set_metadata_respond((FLAC__StreamDecoder*)decoder, type);
}

OggFLAC_API FLAC__bool OggFLAC__stream_decoder_set_metadata_respond_application(OggFLAC__StreamDecoder *decoder, const FLAC__byte id[4])
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	if(decoder->protected_->state != OggFLAC__STREAM_DECODER_UNINITIALIZED)
		return false;
	return FLAC__stream_decoder_set_metadata_respond_application((FLAC__StreamDecoder*)decoder, id);
}

OggFLAC_API FLAC__bool OggFLAC__stream_decoder_set_metadata_respond_all(OggFLAC__StreamDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	if(decoder->protected_->state != OggFLAC__STREAM_DECODER_UNINITIALIZED)
		return false;
	return FLAC__stream_decoder_set_metadata_respond_all((FLAC__StreamDecoder*)decoder);
}

OggFLAC_API FLAC__bool OggFLAC__stream_decoder_set_metadata_ignore(OggFLAC__StreamDecoder *decoder, FLAC__MetadataType type)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	if(decoder->protected_->state != OggFLAC__STREAM_DECODER_UNINITIALIZED)
		return false;
	return FLAC__stream_decoder_set_metadata_ignore((FLAC__StreamDecoder*)decoder, type);
}

OggFLAC_API FLAC__bool OggFLAC__stream_decoder_set_metadata_ignore_application(OggFLAC__StreamDecoder *decoder, const FLAC__byte id[4])
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	if(decoder->protected_->state != OggFLAC__STREAM_DECODER_UNINITIALIZED)
		return false;
	return FLAC__stream_decoder_set_metadata_ignore_application((FLAC__StreamDecoder*)decoder, id);
}

OggFLAC_API FLAC__bool OggFLAC__stream_decoder_set_metadata_ignore_all(OggFLAC__StreamDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	if(decoder->protected_->state != OggFLAC__STREAM_DECODER_UNINITIALIZED)
		return false;
	return FLAC__stream_decoder_set_metadata_ignore_all((FLAC__StreamDecoder*)decoder);
}

OggFLAC_API OggFLAC__StreamDecoderState OggFLAC__stream_decoder_get_state(const OggFLAC__StreamDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->protected_);
	return decoder->protected_->state;
}

OggFLAC_API FLAC__StreamDecoderState OggFLAC__stream_decoder_get_FLAC_stream_decoder_state(const OggFLAC__StreamDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	return FLAC__stream_decoder_get_state((FLAC__StreamDecoder*)decoder);
}

OggFLAC_API const char *OggFLAC__stream_decoder_get_resolved_state_string(const OggFLAC__StreamDecoder *decoder)
{
	if(decoder->protected_->state != OggFLAC__STREAM_DECODER_FLAC_STREAM_DECODER_ERROR)
		return OggFLAC__StreamDecoderStateString[decoder->protected_->state];
	else
		return FLAC__stream_decoder_get_resolved_state_string((FLAC__StreamDecoder*)decoder);
}

OggFLAC_API FLAC__bool OggFLAC__stream_decoder_get_md5_checking(const OggFLAC__StreamDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	return FLAC__stream_decoder_get_md5_checking((FLAC__StreamDecoder*)decoder);
}

OggFLAC_API FLAC__uint64 OggFLAC__stream_decoder_get_total_samples(const OggFLAC__StreamDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	return FLAC__stream_decoder_get_total_samples((FLAC__StreamDecoder*)decoder);
}

OggFLAC_API unsigned OggFLAC__stream_decoder_get_channels(const OggFLAC__StreamDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	return FLAC__stream_decoder_get_channels((FLAC__StreamDecoder*)decoder);
}

OggFLAC_API FLAC__ChannelAssignment OggFLAC__stream_decoder_get_channel_assignment(const OggFLAC__StreamDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	return FLAC__stream_decoder_get_channel_assignment((FLAC__StreamDecoder*)decoder);
}

OggFLAC_API unsigned OggFLAC__stream_decoder_get_bits_per_sample(const OggFLAC__StreamDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	return FLAC__stream_decoder_get_bits_per_sample((FLAC__StreamDecoder*)decoder);
}

OggFLAC_API unsigned OggFLAC__stream_decoder_get_sample_rate(const OggFLAC__StreamDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	return FLAC__stream_decoder_get_sample_rate((FLAC__StreamDecoder*)decoder);
}

OggFLAC_API unsigned OggFLAC__stream_decoder_get_blocksize(const OggFLAC__StreamDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	return FLAC__stream_decoder_get_blocksize((FLAC__StreamDecoder*)decoder);
}

OggFLAC_API FLAC__bool OggFLAC__stream_decoder_flush(OggFLAC__StreamDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);

	OggFLAC__ogg_decoder_aspect_flush(&decoder->protected_->ogg_decoder_aspect);

	if(!FLAC__stream_decoder_flush((FLAC__StreamDecoder*)decoder)) {
		decoder->protected_->state = OggFLAC__STREAM_DECODER_FLAC_STREAM_DECODER_ERROR;
		return false;
	}

	decoder->protected_->state = OggFLAC__STREAM_DECODER_OK;

	return true;
}

OggFLAC_API FLAC__bool OggFLAC__stream_decoder_reset(OggFLAC__StreamDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);

	if(!OggFLAC__stream_decoder_flush(decoder)) {
		decoder->protected_->state = OggFLAC__STREAM_DECODER_MEMORY_ALLOCATION_ERROR;
		return false;
	}

	OggFLAC__ogg_decoder_aspect_reset(&decoder->protected_->ogg_decoder_aspect);

	if(!FLAC__stream_decoder_reset((FLAC__StreamDecoder*)decoder)) {
		decoder->protected_->state = OggFLAC__STREAM_DECODER_FLAC_STREAM_DECODER_ERROR;
		return false;
	}

	decoder->protected_->state = OggFLAC__STREAM_DECODER_OK;

	return true;
}

OggFLAC_API FLAC__bool OggFLAC__stream_decoder_process_single(OggFLAC__StreamDecoder *decoder)
{
	FLAC__bool ret;
	FLAC__ASSERT(0 != decoder);

	if(FLAC__stream_decoder_get_state((FLAC__StreamDecoder*)decoder) == FLAC__STREAM_DECODER_END_OF_STREAM)
		decoder->protected_->state = OggFLAC__STREAM_DECODER_END_OF_STREAM;

	if(decoder->protected_->state == OggFLAC__STREAM_DECODER_END_OF_STREAM)
		return true;

	FLAC__ASSERT(decoder->protected_->state == OggFLAC__STREAM_DECODER_OK);

	ret = FLAC__stream_decoder_process_single((FLAC__StreamDecoder*)decoder);
	if(!ret)
		decoder->protected_->state = OggFLAC__STREAM_DECODER_FLAC_STREAM_DECODER_ERROR;

	return ret;
}

OggFLAC_API FLAC__bool OggFLAC__stream_decoder_process_until_end_of_metadata(OggFLAC__StreamDecoder *decoder)
{
	FLAC__bool ret;
	FLAC__ASSERT(0 != decoder);

	if(FLAC__stream_decoder_get_state((FLAC__StreamDecoder*)decoder) == FLAC__STREAM_DECODER_END_OF_STREAM)
		decoder->protected_->state = OggFLAC__STREAM_DECODER_END_OF_STREAM;

	if(decoder->protected_->state == OggFLAC__STREAM_DECODER_END_OF_STREAM)
		return true;

	FLAC__ASSERT(decoder->protected_->state == OggFLAC__STREAM_DECODER_OK);

	ret = FLAC__stream_decoder_process_until_end_of_metadata((FLAC__StreamDecoder*)decoder);
	if(!ret)
		decoder->protected_->state = OggFLAC__STREAM_DECODER_FLAC_STREAM_DECODER_ERROR;

	return ret;
}

OggFLAC_API FLAC__bool OggFLAC__stream_decoder_process_until_end_of_stream(OggFLAC__StreamDecoder *decoder)
{
	FLAC__bool ret;
	FLAC__ASSERT(0 != decoder);

	if(FLAC__stream_decoder_get_state((FLAC__StreamDecoder*)decoder) == FLAC__STREAM_DECODER_END_OF_STREAM)
		decoder->protected_->state = OggFLAC__STREAM_DECODER_END_OF_STREAM;

	if(decoder->protected_->state == OggFLAC__STREAM_DECODER_END_OF_STREAM)
		return true;

	FLAC__ASSERT(decoder->protected_->state == OggFLAC__STREAM_DECODER_OK);

	ret = FLAC__stream_decoder_process_until_end_of_stream((FLAC__StreamDecoder*)decoder);
	if(!ret)
		decoder->protected_->state = OggFLAC__STREAM_DECODER_FLAC_STREAM_DECODER_ERROR;

	return ret;
}

OggFLAC_API FLAC__bool OggFLAC__stream_decoder_skip_single_frame(OggFLAC__StreamDecoder *decoder)
{
	FLAC__bool ret;
	FLAC__ASSERT(0 != decoder);

	if(FLAC__stream_decoder_get_state((FLAC__StreamDecoder*)decoder) == FLAC__STREAM_DECODER_END_OF_STREAM)
		decoder->protected_->state = OggFLAC__STREAM_DECODER_END_OF_STREAM;

	if(decoder->protected_->state == OggFLAC__STREAM_DECODER_END_OF_STREAM)
		return true;

	FLAC__ASSERT(decoder->protected_->state == OggFLAC__STREAM_DECODER_OK);

	ret = FLAC__stream_decoder_skip_single_frame((FLAC__StreamDecoder*)decoder);
	if(!ret)
		decoder->protected_->state = OggFLAC__STREAM_DECODER_FLAC_STREAM_DECODER_ERROR;

	return ret;
}

OggFLAC_API FLAC__bool OggFLAC__stream_decoder_seek_absolute(OggFLAC__StreamDecoder *decoder, FLAC__uint64 sample)
{
	FLAC__uint64 length;

	FLAC__ASSERT(0 != decoder);

	if(decoder->protected_->state != OggFLAC__STREAM_DECODER_OK && decoder->protected_->state != OggFLAC__STREAM_DECODER_END_OF_STREAM)
		return false;

	if(0 == decoder->private_->seek_callback)
		return false;

	FLAC__ASSERT(decoder->private_->seek_callback);
	FLAC__ASSERT(decoder->private_->tell_callback);
	FLAC__ASSERT(decoder->private_->length_callback);
	FLAC__ASSERT(decoder->private_->eof_callback);

	if(OggFLAC__stream_decoder_get_total_samples(decoder) > 0 && sample >= OggFLAC__stream_decoder_get_total_samples(decoder))
		return false;

	decoder->private_->is_seeking = true;

	/* get the file length (currently our algorithm needs to know the length so it's also an error to get FLAC__STREAM_DECODER_LENGTH_STATUS_UNSUPPORTED) */
	if(decoder->private_->length_callback((FLAC__StreamDecoder*)decoder, &length, decoder->private_->client_data) != FLAC__STREAM_DECODER_LENGTH_STATUS_OK) {
		decoder->private_->is_seeking = false;
		return false;
	}

	/* if we haven't finished processing the metadata yet, do that so we have the STREAMINFO */
	if(
		FLAC__stream_decoder_get_state((FLAC__StreamDecoder*)decoder) == FLAC__STREAM_DECODER_SEARCH_FOR_METADATA ||
		FLAC__stream_decoder_get_state((FLAC__StreamDecoder*)decoder) == FLAC__STREAM_DECODER_READ_METADATA
	) {
		if(!OggFLAC__stream_decoder_process_until_end_of_metadata(decoder)) {
			/* above call sets the state for us */
			decoder->private_->is_seeking = false;
			return false;
		}
		/* check this again in case we didn't know total_samples the first time */
		if(OggFLAC__stream_decoder_get_total_samples(decoder) > 0 && sample >= OggFLAC__stream_decoder_get_total_samples(decoder)) {
			decoder->private_->is_seeking = false;
			return false;
		}
	}

	{
		FLAC__bool ok = seek_to_absolute_sample_(decoder, length, sample);
		decoder->private_->is_seeking = false;
		return ok;
	}
}


/***********************************************************************
 *
 * Private class methods
 *
 ***********************************************************************/

void set_defaults_(OggFLAC__StreamDecoder *decoder)
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
	OggFLAC__ogg_decoder_aspect_set_defaults(&decoder->protected_->ogg_decoder_aspect);
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
#elif defined __CYGWIN__ || defined __EMX__
	/* almost certainly not needed for any modern Cygwin, but let's be safe... */
	setmode(_fileno(stdin), _O_BINARY);
#endif

	return stdin;
}

FLAC__StreamDecoderReadStatus read_callback2_(const FLAC__StreamDecoder *super, FLAC__byte buffer[], unsigned *bytes, void *client_data)
{
	const OggFLAC__StreamDecoder *decoder = (const OggFLAC__StreamDecoder *)super;
	(void)client_data;
	if(decoder->private_->eof_callback && decoder->private_->eof_callback((FLAC__StreamDecoder*)decoder, decoder->private_->client_data)) {
		*bytes = 0;
#if 0
		/*@@@@@@ we used to do this: */
		stream_decoder->protected_->state = OggFLAC__STREAM_DECODER_END_OF_STREAM;
		/* but it causes a problem because the Ogg decoding layer reads as much as it can to get pages, so the state will get to end-of-stream before the bitbuffer does */
#endif
		return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
	}
	else if(*bytes > 0) {
		const FLAC__StreamDecoderReadStatus status = decoder->private_->read_callback(super, buffer, bytes, decoder->private_->client_data);
		if(status == FLAC__STREAM_DECODER_READ_STATUS_ABORT) {
			decoder->protected_->state = OggFLAC__STREAM_DECODER_READ_ERROR;
			return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
		}
		else if(*bytes == 0) {
			if(status == FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM || (decoder->private_->eof_callback && decoder->private_->eof_callback((FLAC__StreamDecoder*)decoder, decoder->private_->client_data))) {
#if 0
				/*@@@@@@ we used to do this: */
				stream_decoder->protected_->state = OggFLAC__STREAM_DECODER_END_OF_STREAM;
				/* but it causes a problem because the Ogg decoding layer reads as much as it can to get pages, so the state will get to end-of-stream before the bitbuffer does */
#endif
				return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
			}
			else
				return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
		}
		else
			return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
	}
	else
		return FLAC__STREAM_DECODER_READ_STATUS_ABORT; /* abort to avoid a deadlock */
}

FLAC__StreamDecoderReadStatus read_callback_ogg_aspect_(const FLAC__StreamDecoder *super, FLAC__byte buffer[], unsigned *bytes, void *client_data)
{
	const OggFLAC__StreamDecoder *decoder = (const OggFLAC__StreamDecoder *)super;
	(void)client_data;

	switch(OggFLAC__ogg_decoder_aspect_read_callback_wrapper(&decoder->protected_->ogg_decoder_aspect, buffer, bytes, read_callback_proxy_, decoder, decoder->private_->client_data)) {
		case OggFLAC__OGG_DECODER_ASPECT_READ_STATUS_OK:
			return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
		/* we don't really have a way to handle lost sync via read
		 * callback so we'll let it pass and let the underlying
		 * FLAC decoder catch the error
		 */
		case OggFLAC__OGG_DECODER_ASPECT_READ_STATUS_LOST_SYNC:
			return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
		case OggFLAC__OGG_DECODER_ASPECT_READ_STATUS_END_OF_STREAM:
			return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
		case OggFLAC__OGG_DECODER_ASPECT_READ_STATUS_NOT_FLAC:
		case OggFLAC__OGG_DECODER_ASPECT_READ_STATUS_UNSUPPORTED_MAPPING_VERSION:
		case OggFLAC__OGG_DECODER_ASPECT_READ_STATUS_ABORT:
		case OggFLAC__OGG_DECODER_ASPECT_READ_STATUS_ERROR:
			decoder->protected_->state = OggFLAC__STREAM_DECODER_READ_ERROR;
			return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
		case OggFLAC__OGG_DECODER_ASPECT_READ_STATUS_MEMORY_ALLOCATION_ERROR:
			decoder->protected_->state = OggFLAC__STREAM_DECODER_MEMORY_ALLOCATION_ERROR;
			return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
		default:
			FLAC__ASSERT(0);
			/* double protection */
			return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
	}
}

OggFLAC__OggDecoderAspectReadStatus read_callback_proxy_(const void *void_decoder, FLAC__byte buffer[], unsigned *bytes, void *client_data)
{
	OggFLAC__StreamDecoder *decoder = (OggFLAC__StreamDecoder*)void_decoder;

	switch(read_callback2_((FLAC__StreamDecoder*)decoder, buffer, bytes, client_data)) {
		case FLAC__STREAM_DECODER_READ_STATUS_CONTINUE:
			return OggFLAC__OGG_DECODER_ASPECT_READ_STATUS_OK;
		case FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM:
			return OggFLAC__OGG_DECODER_ASPECT_READ_STATUS_END_OF_STREAM;
		case FLAC__STREAM_DECODER_READ_STATUS_ABORT:
			return OggFLAC__OGG_DECODER_ASPECT_READ_STATUS_ABORT;
		default:
			/* double protection: */
			FLAC__ASSERT(0);
			return OggFLAC__OGG_DECODER_ASPECT_READ_STATUS_ABORT;
	}
}

FLAC__StreamDecoderWriteStatus write_callback_(const FLAC__StreamDecoder *super, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data)
{
	OggFLAC__StreamDecoder *decoder = (OggFLAC__StreamDecoder *)super;
	(void)client_data;

	if(decoder->private_->is_seeking) {
		FLAC__uint64 this_frame_sample = frame->header.number.sample_number;
		FLAC__uint64 next_frame_sample = this_frame_sample + (FLAC__uint64)frame->header.blocksize;
		FLAC__uint64 target_sample = decoder->private_->target_sample;

		FLAC__ASSERT(frame->header.number_type == FLAC__FRAME_NUMBER_TYPE_SAMPLE_NUMBER);

		decoder->private_->got_a_frame = true;
		decoder->private_->last_frame = *frame; /* save the frame */
		if(this_frame_sample <= target_sample && target_sample < next_frame_sample) { /* we hit our target frame */
			unsigned delta = (unsigned)(target_sample - this_frame_sample);
			/* kick out of seek mode */
			decoder->private_->is_seeking = false;
			/* shift out the samples before target_sample */
			if(delta > 0) {
				unsigned channel;
				const FLAC__int32 *newbuffer[FLAC__MAX_CHANNELS];
				for(channel = 0; channel < frame->header.channels; channel++)
					newbuffer[channel] = buffer[channel] + delta;
				decoder->private_->last_frame.header.blocksize -= delta;
				decoder->private_->last_frame.header.number.sample_number += (FLAC__uint64)delta;
				/* write the relevant samples */
				return decoder->private_->write_callback((FLAC__StreamDecoder*)decoder, &decoder->private_->last_frame, newbuffer, decoder->private_->client_data);
			}
			else {
				/* write the relevant samples */
				return decoder->private_->write_callback((FLAC__StreamDecoder*)decoder, frame, buffer, decoder->private_->client_data);
			}
		}
		else {
			return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
		}
	}
	else
		return decoder->private_->write_callback((FLAC__StreamDecoder*)decoder, frame, buffer, decoder->private_->client_data);
}

void metadata_callback_(const FLAC__StreamDecoder *super, const FLAC__StreamMetadata *metadata, void *client_data)
{
	OggFLAC__StreamDecoder *decoder = (OggFLAC__StreamDecoder *)super;
	(void)client_data;

	if(!decoder->private_->is_seeking)
		decoder->private_->metadata_callback((FLAC__StreamDecoder*)decoder, metadata, decoder->private_->client_data);
}

void error_callback_(const FLAC__StreamDecoder *super, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
	OggFLAC__StreamDecoder *decoder = (OggFLAC__StreamDecoder *)super;
	(void)client_data;

	if(!decoder->private_->is_seeking)
		decoder->private_->error_callback((FLAC__StreamDecoder*)decoder, status, decoder->private_->client_data);
}

FLAC__bool seek_to_absolute_sample_(OggFLAC__StreamDecoder *decoder, FLAC__uint64 stream_length, FLAC__uint64 target_sample)
{
	FLAC__uint64 left_pos = 0, right_pos = stream_length;
	FLAC__uint64 left_sample = 0, right_sample = OggFLAC__stream_decoder_get_total_samples(decoder);
	FLAC__uint64 this_frame_sample = 0; /* only initialized to avoid compiler warning */
	FLAC__uint64 pos = 0; /* only initialized to avoid compiler warning */
	FLAC__bool did_a_seek;
	unsigned iteration = 0;

	/* In the first iterations, we will calculate the target byte position 
	 * by the distance from the target sample to left_sample and
	 * right_sample (let's call it "proportional search").  After that, we
	 * will switch to binary search.
	 */
	unsigned BINARY_SEARCH_AFTER_ITERATION = 2;

	/* We will switch to a linear search once our current sample is less
	 * than this number of samples ahead of the target sample
	 */
	static const FLAC__uint64 LINEAR_SEARCH_WITHIN_SAMPLES = FLAC__MAX_BLOCK_SIZE * 2;

	/* If the total number of samples is unknown, use a large value, and
	 * force binary search immediately.
	 */
	if(right_sample == 0) {
		right_sample = (FLAC__uint64)(-1);
		BINARY_SEARCH_AFTER_ITERATION = 0;
	}

	decoder->private_->target_sample = target_sample;
	for( ; ; iteration++) {
		if (iteration == 0 || this_frame_sample > target_sample || target_sample - this_frame_sample > LINEAR_SEARCH_WITHIN_SAMPLES) {
			if (iteration >= BINARY_SEARCH_AFTER_ITERATION) {
				pos = (right_pos + left_pos) / 2;
			}
			else {
#ifndef FLAC__INTEGER_ONLY_LIBRARY
#if defined _MSC_VER || defined __MINGW32__
				/* with MSVC you have to spoon feed it the casting */
				pos = (FLAC__uint64)((FLAC__double)(FLAC__int64)(target_sample - left_sample) / (FLAC__double)(FLAC__int64)(right_sample - left_sample) * (FLAC__double)(FLAC__int64)(right_pos - left_pos));
#else
				pos = (FLAC__uint64)((FLAC__double)(target_sample - left_sample) / (FLAC__double)(right_sample - left_sample) * (FLAC__double)(right_pos - left_pos));
#endif
#else
				/* a little less accurate: */
				if ((target_sample-left_sample <= 0xffffffff) && (right_pos-left_pos <= 0xffffffff))
					pos = (FLAC__int64)(((target_sample-left_sample) * (right_pos-left_pos)) / (right_sample-left_sample));
				else /* @@@ WATCHOUT, ~2TB limit */
					pos = (FLAC__int64)((((target_sample-left_sample)>>8) * ((right_pos-left_pos)>>8)) / ((right_sample-left_sample)>>16));
#endif
				/* @@@ TODO: might want to limit pos to some distance
				 * before EOF, to make sure we land before the last frame,
				 * thereby getting a this_frame_sample and so having a better
				 * estimate.  @@@@@@DELETE:this would also mostly (or totally if we could
				 * be sure to land before the last frame) avoid the
				 * end-of-stream case we have to check later.
				 */
			}

			/* physical seek */
			if(decoder->private_->seek_callback((FLAC__StreamDecoder*)decoder, (FLAC__uint64)pos, decoder->private_->client_data) != FLAC__STREAM_DECODER_SEEK_STATUS_OK) {
				decoder->protected_->state = OggFLAC__STREAM_DECODER_SEEK_ERROR;
				return false;
			}
			if(!OggFLAC__stream_decoder_flush(decoder)) {
				/* above call sets the state for us */
				return false;
			}
			did_a_seek = true;
		}
		else
			did_a_seek = false;

		decoder->private_->got_a_frame = false;
		if(!OggFLAC__stream_decoder_process_single(decoder)) {
			decoder->protected_->state = OggFLAC__STREAM_DECODER_SEEK_ERROR;
			return false;
		}
		if(!decoder->private_->got_a_frame) {
			if(did_a_seek) {
				/* this can happen if we seek to a point after the last frame; we drop
				 * to binary search right away in this case to avoid any wasted
				 * iterations of proportional search.
				 */
				right_pos = pos;
				BINARY_SEARCH_AFTER_ITERATION = 0;
			}
			else {
				/* this can probably only happen if total_samples is unknown and the
				 * target_sample is past the end of the stream
				 */
				decoder->protected_->state = OggFLAC__STREAM_DECODER_SEEK_ERROR;
				return false;
			}
		}
		/* our write callback will change the state when it gets to the target frame */
		else if(!decoder->private_->is_seeking/*@@@@@@ && decoder->protected_->state != OggFLAC__STREAM_DECODER_END_OF_STREAM*/) {
			break;
		}
		else {
			this_frame_sample = decoder->private_->last_frame.header.number.sample_number;
			FLAC__ASSERT(decoder->private_->last_frame.header.number_type == FLAC__FRAME_NUMBER_TYPE_SAMPLE_NUMBER);

			if (did_a_seek) {
				if (this_frame_sample <= target_sample) {
					/* The 'equal' case should not happen, since
					 * OggFLAC__stream_decoder_process_single()
					 * should recognize that it has hit the
					 * target sample and we would exit through
					 * the 'break' above.
					 */
					FLAC__ASSERT(this_frame_sample != target_sample);

					left_sample = this_frame_sample;
					/* sanity check to avoid infinite loop */
					if (left_pos == pos) {
						decoder->protected_->state = OggFLAC__STREAM_DECODER_SEEK_ERROR;
						return false;
					}
					left_pos = pos;
				}
				else if(this_frame_sample > target_sample) {
					right_sample = this_frame_sample;
					/* sanity check to avoid infinite loop */
					if (right_pos == pos) {
						decoder->protected_->state = OggFLAC__STREAM_DECODER_SEEK_ERROR;
						return false;
					}
					right_pos = pos;
				}
			}
		}
	}

	return true;
}

FLAC__StreamDecoderReadStatus file_read_callback_(const FLAC__StreamDecoder *super, FLAC__byte buffer[], unsigned *bytes, void *client_data)
{
	OggFLAC__StreamDecoder *decoder = (OggFLAC__StreamDecoder *)super;
	(void)client_data;

	if(*bytes > 0) {
		*bytes = (unsigned)fread(buffer, sizeof(FLAC__byte), *bytes, decoder->private_->file);
		if(ferror(decoder->private_->file))
			return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
		else if(*bytes == 0)
			return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
		else
			return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
	}
	else
		return FLAC__STREAM_DECODER_READ_STATUS_ABORT; /* abort to avoid a deadlock */
}

FLAC__StreamDecoderSeekStatus file_seek_callback_(const FLAC__StreamDecoder *super, FLAC__uint64 absolute_byte_offset, void *client_data)
{
	OggFLAC__StreamDecoder *decoder = (OggFLAC__StreamDecoder *)super;
	(void)client_data;

	if(decoder->private_->file == stdin)
		return FLAC__STREAM_DECODER_SEEK_STATUS_UNSUPPORTED;
	else if(fseeko(decoder->private_->file, (off_t)absolute_byte_offset, SEEK_SET) < 0)
		return FLAC__STREAM_DECODER_SEEK_STATUS_ERROR;
	else
		return FLAC__STREAM_DECODER_SEEK_STATUS_OK;
}

FLAC__StreamDecoderTellStatus file_tell_callback_(const FLAC__StreamDecoder *super, FLAC__uint64 *absolute_byte_offset, void *client_data)
{
	OggFLAC__StreamDecoder *decoder = (OggFLAC__StreamDecoder *)super;
	off_t pos;
	(void)client_data;

	if(decoder->private_->file == stdin)
		return FLAC__STREAM_DECODER_TELL_STATUS_UNSUPPORTED;
	else if((pos = ftello(decoder->private_->file)) < 0)
		return FLAC__STREAM_DECODER_TELL_STATUS_ERROR;
	else {
		*absolute_byte_offset = (FLAC__uint64)pos;
		return FLAC__STREAM_DECODER_TELL_STATUS_OK;
	}
}

FLAC__StreamDecoderLengthStatus file_length_callback_(const FLAC__StreamDecoder *super, FLAC__uint64 *stream_length, void *client_data)
{
	OggFLAC__StreamDecoder *decoder = (OggFLAC__StreamDecoder *)super;
	struct stat filestats;
	(void)client_data;

	if(decoder->private_->file == stdin)
		return FLAC__STREAM_DECODER_LENGTH_STATUS_UNSUPPORTED;
	else if(fstat(fileno(decoder->private_->file), &filestats) != 0)
		return FLAC__STREAM_DECODER_LENGTH_STATUS_ERROR;
	else {
		*stream_length = (FLAC__uint64)filestats.st_size;
		return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
	}
}

FLAC__bool file_eof_callback_(const FLAC__StreamDecoder *super, void *client_data)
{
	OggFLAC__StreamDecoder *decoder = (OggFLAC__StreamDecoder *)super;
	(void)client_data;

	return feof(decoder->private_->file)? true : false;
}
