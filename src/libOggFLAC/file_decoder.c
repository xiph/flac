/* libOggFLAC - Free Lossless Audio Codec + Ogg library
 * Copyright (C) 2002,2003  Josh Coalson
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

#include <stdlib.h> /* for calloc() */
#include "FLAC/assert.h"
#include "protected/file_decoder.h"

/***********************************************************************
 *
 * Private class method prototypes
 *
 ***********************************************************************/

static void set_defaults_(OggFLAC__FileDecoder *decoder);
static FLAC__StreamDecoderWriteStatus write_callback_(const FLAC__FileDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data);
static void metadata_callback_(const FLAC__FileDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data);
static void error_callback_(const FLAC__FileDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data);


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
	FLAC__FileDecoder *FLAC_file_decoder;
} OggFLAC__FileDecoderPrivate;

/***********************************************************************
 *
 * Public static class data
 *
 ***********************************************************************/

OggFLAC_API const char * const OggFLAC__FileDecoderStateString[] = {
	"OggFLAC__FILE_DECODER_OK",
	"OggFLAC__FILE_DECODER_END_OF_FILE",
	"OggFLAC__FILE_DECODER_OGG_ERROR",
	"OggFLAC__FILE_DECODER_FLAC_FILE_DECODER_ERROR",
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

	decoder->private_->FLAC_file_decoder = FLAC__file_decoder_new();
	if(0 == decoder->private_->FLAC_file_decoder) {
		free(decoder->private_);
		free(decoder->protected_);
		free(decoder);
		return 0;
	}

	set_defaults_(decoder);

	decoder->protected_->state = OggFLAC__FILE_DECODER_UNINITIALIZED;

	return decoder;
}

OggFLAC_API void OggFLAC__file_decoder_delete(OggFLAC__FileDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->protected_);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->private_->FLAC_file_decoder);

	OggFLAC__file_decoder_finish(decoder);

	FLAC__file_decoder_delete(decoder->private_->FLAC_file_decoder);

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

	if(!OggFLAC__ogg_decoder_aspect_init(&decoder->protected_->ogg_decoder_aspect))
		return decoder->protected_->state = OggFLAC__FILE_DECODER_OGG_ERROR;

	FLAC__file_decoder_set_write_callback(decoder->private_->FLAC_file_decoder, write_callback_);
	FLAC__file_decoder_set_metadata_callback(decoder->private_->FLAC_file_decoder, metadata_callback_);
	FLAC__file_decoder_set_error_callback(decoder->private_->FLAC_file_decoder, error_callback_);
	FLAC__file_decoder_set_client_data(decoder->private_->FLAC_file_decoder, decoder);

	if(FLAC__file_decoder_init(decoder->private_->FLAC_file_decoder) != FLAC__FILE_DECODER_OK)
		return decoder->protected_->state = OggFLAC__FILE_DECODER_FLAC_FILE_DECODER_ERROR;

	return decoder->protected_->state = OggFLAC__FILE_DECODER_OK;
}

OggFLAC_API FLAC__bool OggFLAC__file_decoder_finish(OggFLAC__FileDecoder *decoder)
{
	FLAC__bool ok;

	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);

	if(decoder->protected_->state == OggFLAC__FILE_DECODER_UNINITIALIZED)
		return true;

	FLAC__ASSERT(0 != decoder->private_->FLAC_file_decoder);

	ok = FLAC__file_decoder_finish(decoder->private_->FLAC_file_decoder);

	OggFLAC__ogg_decoder_aspect_finish(&decoder->protected_->ogg_decoder_aspect);

	set_defaults_(decoder);

	decoder->protected_->state = OggFLAC__FILE_DECODER_UNINITIALIZED;

	return ok;
}

OggFLAC_API FLAC__bool OggFLAC__file_decoder_set_md5_checking(OggFLAC__FileDecoder *decoder, FLAC__bool value)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	if(decoder->protected_->state != OggFLAC__FILE_DECODER_UNINITIALIZED)
		return false;
	return FLAC__file_decoder_set_md5_checking(decoder->private_->FLAC_file_decoder, value);
}

OggFLAC_API FLAC__bool OggFLAC__file_decoder_set_filename(OggFLAC__FileDecoder *decoder, const char *value)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	if(decoder->protected_->state != OggFLAC__FILE_DECODER_UNINITIALIZED)
		return false;
	return FLAC__file_decoder_set_filename(decoder->private_->FLAC_file_decoder, value);
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
	OggFLAC__ogg_decoder_aspect_set_serial_number(&decoder->protected_->ogg_decoder_aspect, value);
	return true;
}

OggFLAC_API FLAC__bool OggFLAC__file_decoder_set_metadata_respond(OggFLAC__FileDecoder *decoder, FLAC__MetadataType type)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	if(decoder->protected_->state != OggFLAC__FILE_DECODER_UNINITIALIZED)
		return false;
	return FLAC__file_decoder_set_metadata_respond(decoder->private_->FLAC_file_decoder, type);
}

OggFLAC_API FLAC__bool OggFLAC__file_decoder_set_metadata_respond_application(OggFLAC__FileDecoder *decoder, const FLAC__byte id[4])
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	if(decoder->protected_->state != OggFLAC__FILE_DECODER_UNINITIALIZED)
		return false;
	return FLAC__file_decoder_set_metadata_respond_application(decoder->private_->FLAC_file_decoder, id);
}

OggFLAC_API FLAC__bool OggFLAC__file_decoder_set_metadata_respond_all(OggFLAC__FileDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	if(decoder->protected_->state != OggFLAC__FILE_DECODER_UNINITIALIZED)
		return false;
	return FLAC__file_decoder_set_metadata_respond_all(decoder->private_->FLAC_file_decoder);
}

OggFLAC_API FLAC__bool OggFLAC__file_decoder_set_metadata_ignore(OggFLAC__FileDecoder *decoder, FLAC__MetadataType type)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	if(decoder->protected_->state != OggFLAC__FILE_DECODER_UNINITIALIZED)
		return false;
	return FLAC__file_decoder_set_metadata_ignore(decoder->private_->FLAC_file_decoder, type);
}

OggFLAC_API FLAC__bool OggFLAC__file_decoder_set_metadata_ignore_application(OggFLAC__FileDecoder *decoder, const FLAC__byte id[4])
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	if(decoder->protected_->state != OggFLAC__FILE_DECODER_UNINITIALIZED)
		return false;
	return FLAC__file_decoder_set_metadata_ignore_application(decoder->private_->FLAC_file_decoder, id);
}

OggFLAC_API FLAC__bool OggFLAC__file_decoder_set_metadata_ignore_all(OggFLAC__FileDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	if(decoder->protected_->state != OggFLAC__FILE_DECODER_UNINITIALIZED)
		return false;
	return FLAC__file_decoder_set_metadata_ignore_all(decoder->private_->FLAC_file_decoder);
}

OggFLAC_API OggFLAC__FileDecoderState OggFLAC__file_decoder_get_state(const OggFLAC__FileDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->protected_);
	return decoder->protected_->state;
}

OggFLAC_API FLAC__FileDecoderState OggFLAC__file_decoder_get_FLAC_file_decoder_state(const OggFLAC__FileDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	return FLAC__file_decoder_get_state(decoder->private_->FLAC_file_decoder);
}

OggFLAC_API FLAC__SeekableStreamDecoderState OggFLAC__file_decoder_get_FLAC_seekable_stream_decoder_state(const OggFLAC__FileDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	return FLAC__file_decoder_get_seekable_stream_decoder_state(decoder->private_->FLAC_file_decoder);
}

OggFLAC_API FLAC__StreamDecoderState OggFLAC__file_decoder_get_FLAC_stream_decoder_state(const OggFLAC__FileDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	return FLAC__file_decoder_get_stream_decoder_state(decoder->private_->FLAC_file_decoder);
}

OggFLAC_API const char *OggFLAC__file_decoder_get_resolved_state_string(const OggFLAC__FileDecoder *decoder)
{
	if(decoder->protected_->state != OggFLAC__FILE_DECODER_FLAC_FILE_DECODER_ERROR)
		return OggFLAC__FileDecoderStateString[decoder->protected_->state];
	else
		return FLAC__file_decoder_get_resolved_state_string(decoder->private_->FLAC_file_decoder);
}

OggFLAC_API FLAC__bool OggFLAC__file_decoder_get_md5_checking(const OggFLAC__FileDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	return FLAC__file_decoder_get_md5_checking(decoder->private_->FLAC_file_decoder);
}

OggFLAC_API unsigned OggFLAC__file_decoder_get_channels(const OggFLAC__FileDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	return FLAC__file_decoder_get_channels(decoder->private_->FLAC_file_decoder);
}

OggFLAC_API FLAC__ChannelAssignment OggFLAC__file_decoder_get_channel_assignment(const OggFLAC__FileDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	return FLAC__file_decoder_get_channel_assignment(decoder->private_->FLAC_file_decoder);
}

OggFLAC_API unsigned OggFLAC__file_decoder_get_bits_per_sample(const OggFLAC__FileDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	return FLAC__file_decoder_get_bits_per_sample(decoder->private_->FLAC_file_decoder);
}

OggFLAC_API unsigned OggFLAC__file_decoder_get_sample_rate(const OggFLAC__FileDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	return FLAC__file_decoder_get_sample_rate(decoder->private_->FLAC_file_decoder);
}

OggFLAC_API unsigned OggFLAC__file_decoder_get_blocksize(const OggFLAC__FileDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	return FLAC__file_decoder_get_blocksize(decoder->private_->FLAC_file_decoder);
}

OggFLAC_API FLAC__bool OggFLAC__file_decoder_get_decode_position(const OggFLAC__FileDecoder *decoder, FLAC__uint64 *position)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	return FLAC__file_decoder_get_decode_position(decoder->private_->FLAC_file_decoder, position);
}

OggFLAC_API FLAC__bool OggFLAC__file_decoder_process_single(OggFLAC__FileDecoder *decoder)
{
	FLAC__bool ret;
	FLAC__ASSERT(0 != decoder);

	if(FLAC__file_decoder_get_state(decoder->private_->FLAC_file_decoder) == FLAC__FILE_DECODER_END_OF_FILE)
		decoder->protected_->state = OggFLAC__FILE_DECODER_END_OF_FILE;

	if(decoder->protected_->state == OggFLAC__FILE_DECODER_END_OF_FILE)
		return true;

	FLAC__ASSERT(decoder->protected_->state == OggFLAC__FILE_DECODER_OK);

	ret = FLAC__file_decoder_process_single(decoder->private_->FLAC_file_decoder);
	if(!ret)
		decoder->protected_->state = OggFLAC__FILE_DECODER_FLAC_FILE_DECODER_ERROR;

	return ret;
}

OggFLAC_API FLAC__bool OggFLAC__file_decoder_process_until_end_of_metadata(OggFLAC__FileDecoder *decoder)
{
	FLAC__bool ret;
	FLAC__ASSERT(0 != decoder);

	if(FLAC__file_decoder_get_state(decoder->private_->FLAC_file_decoder) == FLAC__FILE_DECODER_END_OF_FILE)
		decoder->protected_->state = OggFLAC__FILE_DECODER_END_OF_FILE;

	if(decoder->protected_->state == OggFLAC__FILE_DECODER_END_OF_FILE)
		return true;

	FLAC__ASSERT(decoder->protected_->state == OggFLAC__FILE_DECODER_OK);

	ret = FLAC__file_decoder_process_until_end_of_metadata(decoder->private_->FLAC_file_decoder);
	if(!ret)
		decoder->protected_->state = OggFLAC__FILE_DECODER_FLAC_FILE_DECODER_ERROR;

	return ret;
}

OggFLAC_API FLAC__bool OggFLAC__file_decoder_process_until_end_of_file(OggFLAC__FileDecoder *decoder)
{
	FLAC__bool ret;
	FLAC__ASSERT(0 != decoder);

	if(FLAC__file_decoder_get_state(decoder->private_->FLAC_file_decoder) == FLAC__FILE_DECODER_END_OF_FILE)
		decoder->protected_->state = OggFLAC__FILE_DECODER_END_OF_FILE;

	if(decoder->protected_->state == OggFLAC__FILE_DECODER_END_OF_FILE)
		return true;

	FLAC__ASSERT(decoder->protected_->state == OggFLAC__FILE_DECODER_OK);

	ret = FLAC__file_decoder_process_until_end_of_file(decoder->private_->FLAC_file_decoder);
	if(!ret)
		decoder->protected_->state = OggFLAC__FILE_DECODER_FLAC_FILE_DECODER_ERROR;

	return ret;
}

OggFLAC_API FLAC__bool OggFLAC__file_decoder_seek_absolute(OggFLAC__FileDecoder *decoder, FLAC__uint64 sample)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(decoder->protected_->state == OggFLAC__FILE_DECODER_OK || decoder->protected_->state == OggFLAC__FILE_DECODER_END_OF_FILE);

	if(!FLAC__file_decoder_seek_absolute(decoder->private_->FLAC_file_decoder, sample)) {
		decoder->protected_->state = OggFLAC__FILE_DECODER_FLAC_FILE_DECODER_ERROR;
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
	decoder->private_->write_callback = 0;
	decoder->private_->metadata_callback = 0;
	decoder->private_->error_callback = 0;
	decoder->private_->client_data = 0;
	OggFLAC__ogg_decoder_aspect_set_defaults(&decoder->protected_->ogg_decoder_aspect);
}

FLAC__StreamDecoderWriteStatus write_callback_(const FLAC__FileDecoder *unused, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data)
{
	OggFLAC__FileDecoder *decoder = (OggFLAC__FileDecoder*)client_data;
	(void)unused;
	return decoder->private_->write_callback(decoder, frame, buffer, decoder->private_->client_data);
}

void metadata_callback_(const FLAC__FileDecoder *unused, const FLAC__StreamMetadata *metadata, void *client_data)
{
	OggFLAC__FileDecoder *decoder = (OggFLAC__FileDecoder*)client_data;
	(void)unused;
	decoder->private_->metadata_callback(decoder, metadata, decoder->private_->client_data);
}

void error_callback_(const FLAC__FileDecoder *unused, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
	OggFLAC__FileDecoder *decoder = (OggFLAC__FileDecoder*)client_data;
	(void)unused;
	decoder->private_->error_callback(decoder, status, decoder->private_->client_data);
}
