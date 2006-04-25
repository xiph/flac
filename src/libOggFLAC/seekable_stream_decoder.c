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

#include <stdio.h>
#include <stdlib.h> /* for calloc() */
#include <string.h> /* for memcpy()/memcmp() */
#include "FLAC/assert.h"
#include "protected/seekable_stream_decoder.h"
#include "protected/stream_decoder.h"
#include "../libFLAC/include/private/float.h" /* @@@ ugly hack, but how else to do?  we need to reuse the float formats but don't want to expose it */
#include "../libFLAC/include/private/md5.h" /* @@@ ugly hack, but how else to do?  we need to reuse the md5 code but don't want to expose it */

/***********************************************************************
 *
 * Private class method prototypes
 *
 ***********************************************************************/

static void set_defaults_(OggFLAC__SeekableStreamDecoder *decoder);
static FLAC__StreamDecoderReadStatus read_callback_(const OggFLAC__StreamDecoder *decoder, FLAC__byte buffer[], unsigned *bytes, void *client_data);
static FLAC__StreamDecoderWriteStatus write_callback_(const OggFLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data);
static void metadata_callback_(const OggFLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data);
static void error_callback_(const OggFLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data);
static FLAC__bool seek_to_absolute_sample_(OggFLAC__SeekableStreamDecoder *decoder, FLAC__uint64 stream_length, FLAC__uint64 target_sample);

/***********************************************************************
 *
 * Private class data
 *
 ***********************************************************************/

typedef struct OggFLAC__SeekableStreamDecoderPrivate {
	OggFLAC__SeekableStreamDecoderReadCallback read_callback;
	OggFLAC__SeekableStreamDecoderSeekCallback seek_callback;
	OggFLAC__SeekableStreamDecoderTellCallback tell_callback;
	OggFLAC__SeekableStreamDecoderLengthCallback length_callback;
	OggFLAC__SeekableStreamDecoderEofCallback eof_callback;
	OggFLAC__SeekableStreamDecoderWriteCallback write_callback;
	OggFLAC__SeekableStreamDecoderMetadataCallback metadata_callback;
	OggFLAC__SeekableStreamDecoderErrorCallback error_callback;
	void *client_data;
	OggFLAC__StreamDecoder *stream_decoder;
	FLAC__bool do_md5_checking; /* initially gets protected_->md5_checking but is turned off after a seek */
	struct FLAC__MD5Context md5context;
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
	FLAC__bool got_a_frame; /* hack needed in seek routine to check when process_single() actually writes a frame */
} OggFLAC__SeekableStreamDecoderPrivate;

/***********************************************************************
 *
 * Public static class data
 *
 ***********************************************************************/

OggFLAC_API const char * const OggFLAC__SeekableStreamDecoderStateString[] = {
	"OggFLAC__SEEKABLE_STREAM_DECODER_OK",
	"OggFLAC__SEEKABLE_STREAM_DECODER_SEEKING",
	"OggFLAC__SEEKABLE_STREAM_DECODER_END_OF_STREAM",
	"OggFLAC__SEEKABLE_STREAM_DECODER_MEMORY_ALLOCATION_ERROR",
	"OggFLAC__SEEKABLE_STREAM_DECODER_STREAM_DECODER_ERROR",
	"OggFLAC__SEEKABLE_STREAM_DECODER_READ_ERROR",
	"OggFLAC__SEEKABLE_STREAM_DECODER_SEEK_ERROR",
	"OggFLAC__SEEKABLE_STREAM_DECODER_ALREADY_INITIALIZED",
	"OggFLAC__SEEKABLE_STREAM_DECODER_INVALID_CALLBACK",
	"OggFLAC__SEEKABLE_STREAM_DECODER_UNINITIALIZED"
};

OggFLAC_API const char * const OggFLAC__SeekableStreamDecoderReadStatusString[] = {
	"OggFLAC__SEEKABLE_STREAM_DECODER_READ_STATUS_OK",
	"OggFLAC__SEEKABLE_STREAM_DECODER_READ_STATUS_ERROR"
};

OggFLAC_API const char * const OggFLAC__SeekableStreamDecoderSeekStatusString[] = {
	"OggFLAC__SEEKABLE_STREAM_DECODER_SEEK_STATUS_OK",
	"OggFLAC__SEEKABLE_STREAM_DECODER_SEEK_STATUS_ERROR"
};

OggFLAC_API const char * const OggFLAC__SeekableStreamDecoderTellStatusString[] = {
	"OggFLAC__SEEKABLE_STREAM_DECODER_TELL_STATUS_OK",
	"OggFLAC__SEEKABLE_STREAM_DECODER_TELL_STATUS_ERROR"
};

OggFLAC_API const char * const OggFLAC__SeekableStreamDecoderLengthStatusString[] = {
	"OggFLAC__SEEKABLE_STREAM_DECODER_LENGTH_STATUS_OK",
	"OggFLAC__SEEKABLE_STREAM_DECODER_LENGTH_STATUS_ERROR"
};


/***********************************************************************
 *
 * Class constructor/destructor
 *
 ***********************************************************************/

OggFLAC_API OggFLAC__SeekableStreamDecoder *OggFLAC__seekable_stream_decoder_new()
{
	OggFLAC__SeekableStreamDecoder *decoder;

	FLAC__ASSERT(sizeof(int) >= 4); /* we want to die right away if this is not true */

	decoder = (OggFLAC__SeekableStreamDecoder*)calloc(1, sizeof(OggFLAC__SeekableStreamDecoder));
	if(decoder == 0) {
		return 0;
	}

	decoder->protected_ = (OggFLAC__SeekableStreamDecoderProtected*)calloc(1, sizeof(OggFLAC__SeekableStreamDecoderProtected));
	if(decoder->protected_ == 0) {
		free(decoder);
		return 0;
	}

	decoder->private_ = (OggFLAC__SeekableStreamDecoderPrivate*)calloc(1, sizeof(OggFLAC__SeekableStreamDecoderPrivate));
	if(decoder->private_ == 0) {
		free(decoder->protected_);
		free(decoder);
		return 0;
	}

	decoder->private_->stream_decoder = OggFLAC__stream_decoder_new();
	if(0 == decoder->private_->stream_decoder) {
		free(decoder->private_);
		free(decoder->protected_);
		free(decoder);
		return 0;
	}

	set_defaults_(decoder);

	decoder->protected_->state = OggFLAC__SEEKABLE_STREAM_DECODER_UNINITIALIZED;

	return decoder;
}

OggFLAC_API void OggFLAC__seekable_stream_decoder_delete(OggFLAC__SeekableStreamDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->protected_);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->private_->stream_decoder);

	(void)OggFLAC__seekable_stream_decoder_finish(decoder);

	OggFLAC__stream_decoder_delete(decoder->private_->stream_decoder);

	free(decoder->private_);
	free(decoder->protected_);
	free(decoder);
}

/***********************************************************************
 *
 * Public class methods
 *
 ***********************************************************************/

OggFLAC_API OggFLAC__SeekableStreamDecoderState OggFLAC__seekable_stream_decoder_init(OggFLAC__SeekableStreamDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);

	if(decoder->protected_->state != OggFLAC__SEEKABLE_STREAM_DECODER_UNINITIALIZED)
		return decoder->protected_->state = OggFLAC__SEEKABLE_STREAM_DECODER_ALREADY_INITIALIZED;

	if(0 == decoder->private_->read_callback || 0 == decoder->private_->seek_callback || 0 == decoder->private_->tell_callback || 0 == decoder->private_->length_callback || 0 == decoder->private_->eof_callback)
		return decoder->protected_->state = OggFLAC__SEEKABLE_STREAM_DECODER_INVALID_CALLBACK;

	if(0 == decoder->private_->write_callback || 0 == decoder->private_->metadata_callback || 0 == decoder->private_->error_callback)
		return decoder->protected_->state = OggFLAC__SEEKABLE_STREAM_DECODER_INVALID_CALLBACK;

	decoder->private_->seek_table = 0;

	decoder->private_->do_md5_checking = decoder->protected_->md5_checking;

	/* We initialize the FLAC__MD5Context even though we may never use it.  This
	 * is because md5 checking may be turned on to start and then turned off if
	 * a seek occurs.  So we always init the context here and finalize it in
	 * OggFLAC__seekable_stream_decoder_finish() to make sure things are always
	 * cleaned up properly.
	 */
	FLAC__MD5Init(&decoder->private_->md5context);

	OggFLAC__stream_decoder_set_read_callback(decoder->private_->stream_decoder, read_callback_);
	OggFLAC__stream_decoder_set_write_callback(decoder->private_->stream_decoder, write_callback_);
	OggFLAC__stream_decoder_set_metadata_callback(decoder->private_->stream_decoder, metadata_callback_);
	OggFLAC__stream_decoder_set_error_callback(decoder->private_->stream_decoder, error_callback_);
	OggFLAC__stream_decoder_set_client_data(decoder->private_->stream_decoder, decoder);

	/* We always want to see these blocks.  Whether or not we pass them up
	 * through the metadata callback will be determined by flags set in our
	 * implementation of ..._set_metadata_respond/ignore...()
	 */
	OggFLAC__stream_decoder_set_metadata_respond(decoder->private_->stream_decoder, FLAC__METADATA_TYPE_STREAMINFO);
	OggFLAC__stream_decoder_set_metadata_respond(decoder->private_->stream_decoder, FLAC__METADATA_TYPE_SEEKTABLE);

	if(OggFLAC__stream_decoder_init(decoder->private_->stream_decoder) != OggFLAC__STREAM_DECODER_OK)
		return decoder->protected_->state = OggFLAC__SEEKABLE_STREAM_DECODER_STREAM_DECODER_ERROR;

	return decoder->protected_->state = OggFLAC__SEEKABLE_STREAM_DECODER_OK;
}

OggFLAC_API FLAC__bool OggFLAC__seekable_stream_decoder_finish(OggFLAC__SeekableStreamDecoder *decoder)
{
	FLAC__bool md5_failed = false;

	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);

	if(decoder->protected_->state == OggFLAC__SEEKABLE_STREAM_DECODER_UNINITIALIZED)
		return true;

	FLAC__ASSERT(0 != decoder->private_->stream_decoder);

	/* see the comment in OggFLAC__seekable_stream_decoder_init() as to why we
	 * always call FLAC__MD5Final()
	 */
	FLAC__MD5Final(decoder->private_->computed_md5sum, &decoder->private_->md5context);

	OggFLAC__stream_decoder_finish(decoder->private_->stream_decoder);

	if(decoder->private_->do_md5_checking) {
		if(memcmp(decoder->private_->stored_md5sum, decoder->private_->computed_md5sum, 16))
			md5_failed = true;
	}

	set_defaults_(decoder);

	decoder->protected_->state = OggFLAC__SEEKABLE_STREAM_DECODER_UNINITIALIZED;

	return !md5_failed;
}

OggFLAC_API FLAC__bool OggFLAC__seekable_stream_decoder_set_md5_checking(OggFLAC__SeekableStreamDecoder *decoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->protected_);
	if(decoder->protected_->state != OggFLAC__SEEKABLE_STREAM_DECODER_UNINITIALIZED)
		return false;
	decoder->protected_->md5_checking = value;
	return true;
}

OggFLAC_API FLAC__bool OggFLAC__seekable_stream_decoder_set_read_callback(OggFLAC__SeekableStreamDecoder *decoder, OggFLAC__SeekableStreamDecoderReadCallback value)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	if(decoder->protected_->state != OggFLAC__SEEKABLE_STREAM_DECODER_UNINITIALIZED)
		return false;
	decoder->private_->read_callback = value;
	return true;
}

OggFLAC_API FLAC__bool OggFLAC__seekable_stream_decoder_set_seek_callback(OggFLAC__SeekableStreamDecoder *decoder, OggFLAC__SeekableStreamDecoderSeekCallback value)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	if(decoder->protected_->state != OggFLAC__SEEKABLE_STREAM_DECODER_UNINITIALIZED)
		return false;
	decoder->private_->seek_callback = value;
	return true;
}

OggFLAC_API FLAC__bool OggFLAC__seekable_stream_decoder_set_tell_callback(OggFLAC__SeekableStreamDecoder *decoder, OggFLAC__SeekableStreamDecoderTellCallback value)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	if(decoder->protected_->state != OggFLAC__SEEKABLE_STREAM_DECODER_UNINITIALIZED)
		return false;
	decoder->private_->tell_callback = value;
	return true;
}

OggFLAC_API FLAC__bool OggFLAC__seekable_stream_decoder_set_length_callback(OggFLAC__SeekableStreamDecoder *decoder, OggFLAC__SeekableStreamDecoderLengthCallback value)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	if(decoder->protected_->state != OggFLAC__SEEKABLE_STREAM_DECODER_UNINITIALIZED)
		return false;
	decoder->private_->length_callback = value;
	return true;
}

OggFLAC_API FLAC__bool OggFLAC__seekable_stream_decoder_set_eof_callback(OggFLAC__SeekableStreamDecoder *decoder, OggFLAC__SeekableStreamDecoderEofCallback value)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	if(decoder->protected_->state != OggFLAC__SEEKABLE_STREAM_DECODER_UNINITIALIZED)
		return false;
	decoder->private_->eof_callback = value;
	return true;
}

OggFLAC_API FLAC__bool OggFLAC__seekable_stream_decoder_set_write_callback(OggFLAC__SeekableStreamDecoder *decoder, OggFLAC__SeekableStreamDecoderWriteCallback value)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	if(decoder->protected_->state != OggFLAC__SEEKABLE_STREAM_DECODER_UNINITIALIZED)
		return false;
	decoder->private_->write_callback = value;
	return true;
}

OggFLAC_API FLAC__bool OggFLAC__seekable_stream_decoder_set_metadata_callback(OggFLAC__SeekableStreamDecoder *decoder, OggFLAC__SeekableStreamDecoderMetadataCallback value)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	if(decoder->protected_->state != OggFLAC__SEEKABLE_STREAM_DECODER_UNINITIALIZED)
		return false;
	decoder->private_->metadata_callback = value;
	return true;
}

OggFLAC_API FLAC__bool OggFLAC__seekable_stream_decoder_set_error_callback(OggFLAC__SeekableStreamDecoder *decoder, OggFLAC__SeekableStreamDecoderErrorCallback value)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	if(decoder->protected_->state != OggFLAC__SEEKABLE_STREAM_DECODER_UNINITIALIZED)
		return false;
	decoder->private_->error_callback = value;
	return true;
}

OggFLAC_API FLAC__bool OggFLAC__seekable_stream_decoder_set_client_data(OggFLAC__SeekableStreamDecoder *decoder, void *value)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	if(decoder->protected_->state != OggFLAC__SEEKABLE_STREAM_DECODER_UNINITIALIZED)
		return false;
	decoder->private_->client_data = value;
	return true;
}

OggFLAC_API FLAC__bool OggFLAC__seekable_stream_decoder_set_serial_number(OggFLAC__SeekableStreamDecoder *decoder, long value)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	if(decoder->protected_->state != OggFLAC__SEEKABLE_STREAM_DECODER_UNINITIALIZED)
		return false;
	OggFLAC__stream_decoder_set_serial_number(decoder->private_->stream_decoder, value);
	return true;
}

OggFLAC_API FLAC__bool OggFLAC__seekable_stream_decoder_set_metadata_respond(OggFLAC__SeekableStreamDecoder *decoder, FLAC__MetadataType type)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	FLAC__ASSERT(0 != decoder->private_->stream_decoder);
	if(decoder->protected_->state != OggFLAC__SEEKABLE_STREAM_DECODER_UNINITIALIZED)
		return false;
	if(type == FLAC__METADATA_TYPE_STREAMINFO)
		decoder->private_->ignore_stream_info_block = false;
	else if(type == FLAC__METADATA_TYPE_SEEKTABLE)
		decoder->private_->ignore_seek_table_block = false;
	return OggFLAC__stream_decoder_set_metadata_respond(decoder->private_->stream_decoder, type);
}

OggFLAC_API FLAC__bool OggFLAC__seekable_stream_decoder_set_metadata_respond_application(OggFLAC__SeekableStreamDecoder *decoder, const FLAC__byte id[4])
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	FLAC__ASSERT(0 != decoder->private_->stream_decoder);
	if(decoder->protected_->state != OggFLAC__SEEKABLE_STREAM_DECODER_UNINITIALIZED)
		return false;
	return OggFLAC__stream_decoder_set_metadata_respond_application(decoder->private_->stream_decoder, id);
}

OggFLAC_API FLAC__bool OggFLAC__seekable_stream_decoder_set_metadata_respond_all(OggFLAC__SeekableStreamDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	FLAC__ASSERT(0 != decoder->private_->stream_decoder);
	if(decoder->protected_->state != OggFLAC__SEEKABLE_STREAM_DECODER_UNINITIALIZED)
		return false;
	decoder->private_->ignore_stream_info_block = false;
	decoder->private_->ignore_seek_table_block = false;
	return OggFLAC__stream_decoder_set_metadata_respond_all(decoder->private_->stream_decoder);
}

OggFLAC_API FLAC__bool OggFLAC__seekable_stream_decoder_set_metadata_ignore(OggFLAC__SeekableStreamDecoder *decoder, FLAC__MetadataType type)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	FLAC__ASSERT(0 != decoder->private_->stream_decoder);
	if(decoder->protected_->state != OggFLAC__SEEKABLE_STREAM_DECODER_UNINITIALIZED)
		return false;
	if(type == FLAC__METADATA_TYPE_STREAMINFO) {
		decoder->private_->ignore_stream_info_block = true;
		return true;
	}
	else if(type == FLAC__METADATA_TYPE_SEEKTABLE) {
		decoder->private_->ignore_seek_table_block = true;
		return true;
	}
	else
		return OggFLAC__stream_decoder_set_metadata_ignore(decoder->private_->stream_decoder, type);
}

OggFLAC_API FLAC__bool OggFLAC__seekable_stream_decoder_set_metadata_ignore_application(OggFLAC__SeekableStreamDecoder *decoder, const FLAC__byte id[4])
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	FLAC__ASSERT(0 != decoder->private_->stream_decoder);
	if(decoder->protected_->state != OggFLAC__SEEKABLE_STREAM_DECODER_UNINITIALIZED)
		return false;
	return OggFLAC__stream_decoder_set_metadata_ignore_application(decoder->private_->stream_decoder, id);
}

OggFLAC_API FLAC__bool OggFLAC__seekable_stream_decoder_set_metadata_ignore_all(OggFLAC__SeekableStreamDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	FLAC__ASSERT(0 != decoder->private_->stream_decoder);
	if(decoder->protected_->state != OggFLAC__SEEKABLE_STREAM_DECODER_UNINITIALIZED)
		return false;
	decoder->private_->ignore_stream_info_block = true;
	decoder->private_->ignore_seek_table_block = true;
	return
		OggFLAC__stream_decoder_set_metadata_ignore_all(decoder->private_->stream_decoder) &&
		OggFLAC__stream_decoder_set_metadata_respond(decoder->private_->stream_decoder, FLAC__METADATA_TYPE_STREAMINFO) &&
		OggFLAC__stream_decoder_set_metadata_respond(decoder->private_->stream_decoder, FLAC__METADATA_TYPE_SEEKTABLE);
}

OggFLAC_API OggFLAC__SeekableStreamDecoderState OggFLAC__seekable_stream_decoder_get_state(const OggFLAC__SeekableStreamDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->protected_);
	return decoder->protected_->state;
}

OggFLAC_API OggFLAC__StreamDecoderState OggFLAC__seekable_stream_decoder_get_stream_decoder_state(const OggFLAC__SeekableStreamDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	return OggFLAC__stream_decoder_get_state(decoder->private_->stream_decoder);
}

OggFLAC_API FLAC__StreamDecoderState OggFLAC__seekable_stream_decoder_get_FLAC_stream_decoder_state(const OggFLAC__SeekableStreamDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	return OggFLAC__stream_decoder_get_FLAC_stream_decoder_state(decoder->private_->stream_decoder);
}

OggFLAC_API const char *OggFLAC__seekable_stream_decoder_get_resolved_state_string(const OggFLAC__SeekableStreamDecoder *decoder)
{
	if(decoder->protected_->state != OggFLAC__SEEKABLE_STREAM_DECODER_STREAM_DECODER_ERROR)
		return OggFLAC__SeekableStreamDecoderStateString[decoder->protected_->state];
	else
		return OggFLAC__stream_decoder_get_resolved_state_string(decoder->private_->stream_decoder);
}

OggFLAC_API FLAC__bool OggFLAC__seekable_stream_decoder_get_md5_checking(const OggFLAC__SeekableStreamDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->protected_);
	return decoder->protected_->md5_checking;
}

OggFLAC_API unsigned OggFLAC__seekable_stream_decoder_get_channels(const OggFLAC__SeekableStreamDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	return OggFLAC__stream_decoder_get_channels(decoder->private_->stream_decoder);
}

OggFLAC_API FLAC__ChannelAssignment OggFLAC__seekable_stream_decoder_get_channel_assignment(const OggFLAC__SeekableStreamDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	return OggFLAC__stream_decoder_get_channel_assignment(decoder->private_->stream_decoder);
}

OggFLAC_API unsigned OggFLAC__seekable_stream_decoder_get_bits_per_sample(const OggFLAC__SeekableStreamDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	return OggFLAC__stream_decoder_get_bits_per_sample(decoder->private_->stream_decoder);
}

OggFLAC_API unsigned OggFLAC__seekable_stream_decoder_get_sample_rate(const OggFLAC__SeekableStreamDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	return OggFLAC__stream_decoder_get_sample_rate(decoder->private_->stream_decoder);
}

OggFLAC_API unsigned OggFLAC__seekable_stream_decoder_get_blocksize(const OggFLAC__SeekableStreamDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	return OggFLAC__stream_decoder_get_blocksize(decoder->private_->stream_decoder);
}

OggFLAC_API FLAC__bool OggFLAC__seekable_stream_decoder_flush(OggFLAC__SeekableStreamDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);

	decoder->private_->do_md5_checking = false;

	if(!OggFLAC__stream_decoder_flush(decoder->private_->stream_decoder)) {
		decoder->protected_->state = OggFLAC__SEEKABLE_STREAM_DECODER_STREAM_DECODER_ERROR;
		return false;
	}

	decoder->protected_->state = OggFLAC__SEEKABLE_STREAM_DECODER_OK;

	return true;
}

OggFLAC_API FLAC__bool OggFLAC__seekable_stream_decoder_reset(OggFLAC__SeekableStreamDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);

	if(!OggFLAC__seekable_stream_decoder_flush(decoder)) {
		decoder->protected_->state = OggFLAC__SEEKABLE_STREAM_DECODER_STREAM_DECODER_ERROR;
		return false;
	}

	if(!OggFLAC__stream_decoder_reset(decoder->private_->stream_decoder)) {
		decoder->protected_->state = OggFLAC__SEEKABLE_STREAM_DECODER_STREAM_DECODER_ERROR;
		return false;
	}

	decoder->private_->seek_table = 0;

	decoder->private_->do_md5_checking = decoder->protected_->md5_checking;

	/* We initialize the FLAC__MD5Context even though we may never use it.  This
	 * is because md5 checking may be turned on to start and then turned off if
	 * a seek occurs.  So we always init the context here and finalize it in
	 * OggFLAC__seekable_stream_decoder_finish() to make sure things are always
	 * cleaned up properly.
	 */
	FLAC__MD5Init(&decoder->private_->md5context);

	decoder->protected_->state = OggFLAC__SEEKABLE_STREAM_DECODER_OK;

	return true;
}

OggFLAC_API FLAC__bool OggFLAC__seekable_stream_decoder_process_single(OggFLAC__SeekableStreamDecoder *decoder)
{
	FLAC__bool ret;
	FLAC__ASSERT(0 != decoder);

	if(decoder->private_->stream_decoder->protected_->state == OggFLAC__STREAM_DECODER_END_OF_STREAM)
		decoder->protected_->state = OggFLAC__SEEKABLE_STREAM_DECODER_END_OF_STREAM;

	if(decoder->protected_->state == OggFLAC__SEEKABLE_STREAM_DECODER_END_OF_STREAM)
		return true;

	FLAC__ASSERT(decoder->protected_->state == OggFLAC__SEEKABLE_STREAM_DECODER_OK);

	ret = OggFLAC__stream_decoder_process_single(decoder->private_->stream_decoder);
	if(!ret)
		decoder->protected_->state = OggFLAC__SEEKABLE_STREAM_DECODER_STREAM_DECODER_ERROR;

	return ret;
}

OggFLAC_API FLAC__bool OggFLAC__seekable_stream_decoder_process_until_end_of_metadata(OggFLAC__SeekableStreamDecoder *decoder)
{
	FLAC__bool ret;
	FLAC__ASSERT(0 != decoder);

	if(decoder->private_->stream_decoder->protected_->state == OggFLAC__STREAM_DECODER_END_OF_STREAM)
		decoder->protected_->state = OggFLAC__SEEKABLE_STREAM_DECODER_END_OF_STREAM;

	if(decoder->protected_->state == OggFLAC__SEEKABLE_STREAM_DECODER_END_OF_STREAM)
		return true;

	FLAC__ASSERT(decoder->protected_->state == OggFLAC__SEEKABLE_STREAM_DECODER_OK);

	ret = OggFLAC__stream_decoder_process_until_end_of_metadata(decoder->private_->stream_decoder);
	if(!ret)
		decoder->protected_->state = OggFLAC__SEEKABLE_STREAM_DECODER_STREAM_DECODER_ERROR;

	return ret;
}

OggFLAC_API FLAC__bool OggFLAC__seekable_stream_decoder_process_until_end_of_stream(OggFLAC__SeekableStreamDecoder *decoder)
{
	FLAC__bool ret;
	FLAC__ASSERT(0 != decoder);

	if(decoder->private_->stream_decoder->protected_->state == OggFLAC__STREAM_DECODER_END_OF_STREAM)
		decoder->protected_->state = OggFLAC__SEEKABLE_STREAM_DECODER_END_OF_STREAM;

	if(decoder->protected_->state == OggFLAC__SEEKABLE_STREAM_DECODER_END_OF_STREAM)
		return true;

	FLAC__ASSERT(decoder->protected_->state == OggFLAC__SEEKABLE_STREAM_DECODER_OK);

	ret = OggFLAC__stream_decoder_process_until_end_of_stream(decoder->private_->stream_decoder);
	if(!ret)
		decoder->protected_->state = OggFLAC__SEEKABLE_STREAM_DECODER_STREAM_DECODER_ERROR;

	return ret;
}

OggFLAC_API FLAC__bool OggFLAC__seekable_stream_decoder_seek_absolute(OggFLAC__SeekableStreamDecoder *decoder, FLAC__uint64 sample)
{
	FLAC__uint64 length;

	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(decoder->protected_->state == OggFLAC__SEEKABLE_STREAM_DECODER_OK || decoder->protected_->state == OggFLAC__SEEKABLE_STREAM_DECODER_END_OF_STREAM);

	decoder->protected_->state = OggFLAC__SEEKABLE_STREAM_DECODER_SEEKING;

	/* turn off md5 checking if a seek is attempted */
	decoder->private_->do_md5_checking = false;

	if(!OggFLAC__stream_decoder_reset(decoder->private_->stream_decoder)) {
		decoder->protected_->state = OggFLAC__SEEKABLE_STREAM_DECODER_STREAM_DECODER_ERROR;
		return false;
	}
	/* get the file length */
	if(decoder->private_->length_callback(decoder, &length, decoder->private_->client_data) != OggFLAC__SEEKABLE_STREAM_DECODER_LENGTH_STATUS_OK) {
		decoder->protected_->state = OggFLAC__SEEKABLE_STREAM_DECODER_SEEK_ERROR;
		return false;
	}
	/* rewind */
	if(decoder->private_->seek_callback(decoder, 0, decoder->private_->client_data) != OggFLAC__SEEKABLE_STREAM_DECODER_SEEK_STATUS_OK) {
		decoder->protected_->state = OggFLAC__SEEKABLE_STREAM_DECODER_SEEK_ERROR;
		return false;
	}
	if(!OggFLAC__stream_decoder_process_until_end_of_metadata(decoder->private_->stream_decoder)) {
		decoder->protected_->state = OggFLAC__SEEKABLE_STREAM_DECODER_STREAM_DECODER_ERROR;
		return false;
	}
	if(decoder->private_->stream_info.total_samples > 0 && sample >= decoder->private_->stream_info.total_samples) {
		decoder->protected_->state = OggFLAC__SEEKABLE_STREAM_DECODER_SEEK_ERROR;
		return false;
	}

	return seek_to_absolute_sample_(decoder, length, sample);
}

/***********************************************************************
 *
 * Private class methods
 *
 ***********************************************************************/

void set_defaults_(OggFLAC__SeekableStreamDecoder *decoder)
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
	/* WATCHOUT: these should match the default behavior of OggFLAC__StreamDecoder */
	decoder->private_->ignore_stream_info_block = false;
	decoder->private_->ignore_seek_table_block = true;

	decoder->protected_->md5_checking = false;
}

FLAC__StreamDecoderReadStatus read_callback_(const OggFLAC__StreamDecoder *decoder, FLAC__byte buffer[], unsigned *bytes, void *client_data)
{
	OggFLAC__SeekableStreamDecoder *seekable_stream_decoder = (OggFLAC__SeekableStreamDecoder *)client_data;
	(void)decoder;
	if(seekable_stream_decoder->private_->eof_callback(seekable_stream_decoder, seekable_stream_decoder->private_->client_data)) {
		*bytes = 0;
#if 0
		/*@@@@@@ we used to do this: */
		seekable_stream_decoder->protected_->state = OggFLAC__SEEKABLE_STREAM_DECODER_END_OF_STREAM;
		/* but it causes a problem because the Ogg decoding layer reads as much as it can to get pages, so the state will get to end-of-stream before the bitbuffer does */
#endif
		return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
	}
	else if(*bytes > 0) {
		if(seekable_stream_decoder->private_->read_callback(seekable_stream_decoder, buffer, bytes, seekable_stream_decoder->private_->client_data) != OggFLAC__SEEKABLE_STREAM_DECODER_READ_STATUS_OK) {
			seekable_stream_decoder->protected_->state = OggFLAC__SEEKABLE_STREAM_DECODER_READ_ERROR;
			return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
		}
		if(*bytes == 0) {
			if(seekable_stream_decoder->private_->eof_callback(seekable_stream_decoder, seekable_stream_decoder->private_->client_data)) {
#if 0
				/*@@@@@@ we used to do this: */
				seekable_stream_decoder->protected_->state = OggFLAC__SEEKABLE_STREAM_DECODER_END_OF_STREAM;
				/* but it causes a problem because the Ogg decoding layer reads as much as it can to get pages, so the state will get to end-of-stream before the bitbuffer does */
#endif
				return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
			}
			else
				return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
		}
		else {
			return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
		}
	}
	else
		return FLAC__STREAM_DECODER_READ_STATUS_ABORT; /* abort to avoid a deadlock */
}

FLAC__StreamDecoderWriteStatus write_callback_(const OggFLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data)
{
	OggFLAC__SeekableStreamDecoder *seekable_stream_decoder = (OggFLAC__SeekableStreamDecoder *)client_data;
	(void)decoder;

	if(seekable_stream_decoder->protected_->state == OggFLAC__SEEKABLE_STREAM_DECODER_SEEKING) {
		FLAC__uint64 this_frame_sample = frame->header.number.sample_number;
		FLAC__uint64 next_frame_sample = this_frame_sample + (FLAC__uint64)frame->header.blocksize;
		FLAC__uint64 target_sample = seekable_stream_decoder->private_->target_sample;

		FLAC__ASSERT(frame->header.number_type == FLAC__FRAME_NUMBER_TYPE_SAMPLE_NUMBER);

		seekable_stream_decoder->private_->got_a_frame = true;
		seekable_stream_decoder->private_->last_frame = *frame; /* save the frame */
		if(this_frame_sample <= target_sample && target_sample < next_frame_sample) { /* we hit our target frame */
			unsigned delta = (unsigned)(target_sample - this_frame_sample);
			/* kick out of seek mode */
			seekable_stream_decoder->protected_->state = OggFLAC__SEEKABLE_STREAM_DECODER_OK;
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

void metadata_callback_(const OggFLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data)
{
	OggFLAC__SeekableStreamDecoder *seekable_stream_decoder = (OggFLAC__SeekableStreamDecoder *)client_data;
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

	if(seekable_stream_decoder->protected_->state != OggFLAC__SEEKABLE_STREAM_DECODER_SEEKING) {
		FLAC__bool ignore_block = false;
		if(metadata->type == FLAC__METADATA_TYPE_STREAMINFO && seekable_stream_decoder->private_->ignore_stream_info_block)
			ignore_block = true;
		else if(metadata->type == FLAC__METADATA_TYPE_SEEKTABLE && seekable_stream_decoder->private_->ignore_seek_table_block)
			ignore_block = true;
		if(!ignore_block)
			seekable_stream_decoder->private_->metadata_callback(seekable_stream_decoder, metadata, seekable_stream_decoder->private_->client_data);
	}
}

void error_callback_(const OggFLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
	OggFLAC__SeekableStreamDecoder *seekable_stream_decoder = (OggFLAC__SeekableStreamDecoder *)client_data;
	(void)decoder;

	if(seekable_stream_decoder->protected_->state != OggFLAC__SEEKABLE_STREAM_DECODER_SEEKING)
		seekable_stream_decoder->private_->error_callback(seekable_stream_decoder, status, seekable_stream_decoder->private_->client_data);
}

FLAC__bool seek_to_absolute_sample_(OggFLAC__SeekableStreamDecoder *decoder, FLAC__uint64 stream_length, FLAC__uint64 target_sample)
{
	FLAC__uint64 left_pos = 0, right_pos = stream_length;
	FLAC__uint64 left_sample = 0, right_sample = decoder->private_->stream_info.total_samples;
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
	 * that this number of samples ahead of the target sample
	 */
	static const FLAC__uint64 LINEAR_SEARCH_WITHIN_SAMPLES = FLAC__MAX_BLOCK_SIZE * 2;

	/* If the total number of samples is unknown, use a large value and
	 * increase 'iteration' to force binary search immediately.
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
				 * thereby getting a this_fram_sample and so having a better
				 * estimate.  this would also mostly (or totally if we could
				 * be sure to land before the last frame) avoid the
				 * end-of-stream case we have to check later.
				 */
			}

			/* physical seek */
			if(decoder->private_->seek_callback(decoder, (FLAC__uint64)pos, decoder->private_->client_data) != OggFLAC__SEEKABLE_STREAM_DECODER_SEEK_STATUS_OK) {
				decoder->protected_->state = OggFLAC__SEEKABLE_STREAM_DECODER_SEEK_ERROR;
				return false;
			}
			if(!OggFLAC__stream_decoder_flush(decoder->private_->stream_decoder)) {
				decoder->protected_->state = OggFLAC__SEEKABLE_STREAM_DECODER_STREAM_DECODER_ERROR;
				return false;
			}
			did_a_seek = true;
		}
		else
			did_a_seek = false;

		decoder->private_->got_a_frame = false;
		if(!OggFLAC__stream_decoder_process_single(decoder->private_->stream_decoder)) {
			decoder->protected_->state = OggFLAC__SEEKABLE_STREAM_DECODER_SEEK_ERROR;
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
				decoder->protected_->state = OggFLAC__SEEKABLE_STREAM_DECODER_SEEK_ERROR;
				return false;
			}
		}
		/* our write callback will change the state when it gets to the target frame */
		else if(
			decoder->protected_->state != OggFLAC__SEEKABLE_STREAM_DECODER_SEEKING &&
			decoder->protected_->state != OggFLAC__SEEKABLE_STREAM_DECODER_END_OF_STREAM
		) {
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
						decoder->protected_->state = OggFLAC__SEEKABLE_STREAM_DECODER_SEEK_ERROR;
						return false;
					}
					left_pos = pos;
				}
				else if(this_frame_sample > target_sample) {
					right_sample = this_frame_sample;
					/* sanity check to avoid infinite loop */
					if (right_pos == pos) {
						decoder->protected_->state = OggFLAC__SEEKABLE_STREAM_DECODER_SEEK_ERROR;
						return false;
					}
					right_pos = pos;
				}
			}
		}
	}

	return true;
}
