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

#include <stdlib.h> /* for calloc() */
#include "FLAC/assert.h"
#include "protected/stream_decoder.h"

/***********************************************************************
 *
 * Private class method prototypes
 *
 ***********************************************************************/

static void set_defaults_(OggFLAC__StreamDecoder *decoder);
static FLAC__StreamDecoderReadStatus read_callback_(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], unsigned *bytes, void *client_data);
static FLAC__StreamDecoderWriteStatus write_callback_(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data);
static void metadata_callback_(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data);
static void error_callback_(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data);
static OggFLAC__OggDecoderAspectReadStatus read_callback_proxy_(const void *void_decoder, FLAC__byte buffer[], unsigned *bytes, void *client_data);


/***********************************************************************
 *
 * Private class data
 *
 ***********************************************************************/

typedef struct OggFLAC__StreamDecoderPrivate {
	OggFLAC__StreamDecoderReadCallback read_callback;
	OggFLAC__StreamDecoderWriteCallback write_callback;
	OggFLAC__StreamDecoderMetadataCallback metadata_callback;
	OggFLAC__StreamDecoderErrorCallback error_callback;
	void *client_data;
	FLAC__StreamDecoder *FLAC_stream_decoder;
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
	"OggFLAC__STREAM_DECODER_READ_ERROR",
	"OggFLAC__STREAM_DECODER_FLAC_STREAM_DECODER_ERROR",
	"OggFLAC__STREAM_DECODER_MEMORY_ALLOCATION_ERROR",
	"OggFLAC__STREAM_DECODER_ALREADY_INITIALIZED",
	"OggFLAC__STREAM_DECODER_INVALID_CALLBACK",
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

	decoder->private_->FLAC_stream_decoder = FLAC__stream_decoder_new();
	if(0 == decoder->private_->FLAC_stream_decoder) {
		free(decoder->private_);
		free(decoder->protected_);
		free(decoder);
		return 0;
	}

	set_defaults_(decoder);

	decoder->protected_->state = OggFLAC__STREAM_DECODER_UNINITIALIZED;

	return decoder;
}

OggFLAC_API void OggFLAC__stream_decoder_delete(OggFLAC__StreamDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->protected_);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->private_->FLAC_stream_decoder);

	OggFLAC__stream_decoder_finish(decoder);

	FLAC__stream_decoder_delete(decoder->private_->FLAC_stream_decoder);

	free(decoder->private_);
	free(decoder->protected_);
	free(decoder);
}

/***********************************************************************
 *
 * Public class methods
 *
 ***********************************************************************/

OggFLAC_API OggFLAC__StreamDecoderState OggFLAC__stream_decoder_init(OggFLAC__StreamDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);

	if(decoder->protected_->state != OggFLAC__STREAM_DECODER_UNINITIALIZED)
		return decoder->protected_->state = OggFLAC__STREAM_DECODER_ALREADY_INITIALIZED;

	if(0 == decoder->private_->read_callback || 0 == decoder->private_->write_callback || 0 == decoder->private_->metadata_callback || 0 == decoder->private_->error_callback)
		return decoder->protected_->state = OggFLAC__STREAM_DECODER_INVALID_CALLBACK;

	if(!OggFLAC__ogg_decoder_aspect_init(&decoder->protected_->ogg_decoder_aspect))
		return decoder->protected_->state = OggFLAC__STREAM_DECODER_OGG_ERROR;

	FLAC__stream_decoder_set_read_callback(decoder->private_->FLAC_stream_decoder, read_callback_);
	FLAC__stream_decoder_set_write_callback(decoder->private_->FLAC_stream_decoder, write_callback_);
	FLAC__stream_decoder_set_metadata_callback(decoder->private_->FLAC_stream_decoder, metadata_callback_);
	FLAC__stream_decoder_set_error_callback(decoder->private_->FLAC_stream_decoder, error_callback_);
	FLAC__stream_decoder_set_client_data(decoder->private_->FLAC_stream_decoder, decoder);

	if(FLAC__stream_decoder_init(decoder->private_->FLAC_stream_decoder) != FLAC__STREAM_DECODER_SEARCH_FOR_METADATA)
		return decoder->protected_->state = OggFLAC__STREAM_DECODER_FLAC_STREAM_DECODER_ERROR;

	return decoder->protected_->state = OggFLAC__STREAM_DECODER_OK;
}

OggFLAC_API void OggFLAC__stream_decoder_finish(OggFLAC__StreamDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);

	if(decoder->protected_->state == OggFLAC__STREAM_DECODER_UNINITIALIZED)
		return;

	FLAC__ASSERT(0 != decoder->private_->FLAC_stream_decoder);

	FLAC__stream_decoder_finish(decoder->private_->FLAC_stream_decoder);

	OggFLAC__ogg_decoder_aspect_finish(&decoder->protected_->ogg_decoder_aspect);

	set_defaults_(decoder);

	decoder->protected_->state = OggFLAC__STREAM_DECODER_UNINITIALIZED;
}

OggFLAC_API FLAC__bool OggFLAC__stream_decoder_set_read_callback(OggFLAC__StreamDecoder *decoder, OggFLAC__StreamDecoderReadCallback value)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	if(decoder->protected_->state != OggFLAC__STREAM_DECODER_UNINITIALIZED)
		return false;
	decoder->private_->read_callback = value;
	return true;
}

OggFLAC_API FLAC__bool OggFLAC__stream_decoder_set_write_callback(OggFLAC__StreamDecoder *decoder, OggFLAC__StreamDecoderWriteCallback value)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	if(decoder->protected_->state != OggFLAC__STREAM_DECODER_UNINITIALIZED)
		return false;
	decoder->private_->write_callback = value;
	return true;
}

OggFLAC_API FLAC__bool OggFLAC__stream_decoder_set_metadata_callback(OggFLAC__StreamDecoder *decoder, OggFLAC__StreamDecoderMetadataCallback value)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	if(decoder->protected_->state != OggFLAC__STREAM_DECODER_UNINITIALIZED)
		return false;
	decoder->private_->metadata_callback = value;
	return true;
}

OggFLAC_API FLAC__bool OggFLAC__stream_decoder_set_error_callback(OggFLAC__StreamDecoder *decoder, OggFLAC__StreamDecoderErrorCallback value)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	if(decoder->protected_->state != OggFLAC__STREAM_DECODER_UNINITIALIZED)
		return false;
	decoder->private_->error_callback = value;
	return true;
}

OggFLAC_API FLAC__bool OggFLAC__stream_decoder_set_client_data(OggFLAC__StreamDecoder *decoder, void *value)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	if(decoder->protected_->state != OggFLAC__STREAM_DECODER_UNINITIALIZED)
		return false;
	decoder->private_->client_data = value;
	return true;
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
	return FLAC__stream_decoder_set_metadata_respond(decoder->private_->FLAC_stream_decoder, type);
}

OggFLAC_API FLAC__bool OggFLAC__stream_decoder_set_metadata_respond_application(OggFLAC__StreamDecoder *decoder, const FLAC__byte id[4])
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	if(decoder->protected_->state != OggFLAC__STREAM_DECODER_UNINITIALIZED)
		return false;
	return FLAC__stream_decoder_set_metadata_respond_application(decoder->private_->FLAC_stream_decoder, id);
}

OggFLAC_API FLAC__bool OggFLAC__stream_decoder_set_metadata_respond_all(OggFLAC__StreamDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	if(decoder->protected_->state != OggFLAC__STREAM_DECODER_UNINITIALIZED)
		return false;
	return FLAC__stream_decoder_set_metadata_respond_all(decoder->private_->FLAC_stream_decoder);
}

OggFLAC_API FLAC__bool OggFLAC__stream_decoder_set_metadata_ignore(OggFLAC__StreamDecoder *decoder, FLAC__MetadataType type)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	if(decoder->protected_->state != OggFLAC__STREAM_DECODER_UNINITIALIZED)
		return false;
	return FLAC__stream_decoder_set_metadata_ignore(decoder->private_->FLAC_stream_decoder, type);
}

OggFLAC_API FLAC__bool OggFLAC__stream_decoder_set_metadata_ignore_application(OggFLAC__StreamDecoder *decoder, const FLAC__byte id[4])
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	if(decoder->protected_->state != OggFLAC__STREAM_DECODER_UNINITIALIZED)
		return false;
	return FLAC__stream_decoder_set_metadata_ignore_application(decoder->private_->FLAC_stream_decoder, id);
}

OggFLAC_API FLAC__bool OggFLAC__stream_decoder_set_metadata_ignore_all(OggFLAC__StreamDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	if(decoder->protected_->state != OggFLAC__STREAM_DECODER_UNINITIALIZED)
		return false;
	return FLAC__stream_decoder_set_metadata_ignore_all(decoder->private_->FLAC_stream_decoder);
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
	return FLAC__stream_decoder_get_state(decoder->private_->FLAC_stream_decoder);
}

OggFLAC_API const char *OggFLAC__stream_decoder_get_resolved_state_string(const OggFLAC__StreamDecoder *decoder)
{
	if(decoder->protected_->state != OggFLAC__STREAM_DECODER_FLAC_STREAM_DECODER_ERROR)
		return OggFLAC__StreamDecoderStateString[decoder->protected_->state];
	else
		return FLAC__stream_decoder_get_resolved_state_string(decoder->private_->FLAC_stream_decoder);
}

OggFLAC_API unsigned OggFLAC__stream_decoder_get_channels(const OggFLAC__StreamDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	return FLAC__stream_decoder_get_channels(decoder->private_->FLAC_stream_decoder);
}

OggFLAC_API FLAC__ChannelAssignment OggFLAC__stream_decoder_get_channel_assignment(const OggFLAC__StreamDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	return FLAC__stream_decoder_get_channel_assignment(decoder->private_->FLAC_stream_decoder);
}

OggFLAC_API unsigned OggFLAC__stream_decoder_get_bits_per_sample(const OggFLAC__StreamDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	return FLAC__stream_decoder_get_bits_per_sample(decoder->private_->FLAC_stream_decoder);
}

OggFLAC_API unsigned OggFLAC__stream_decoder_get_sample_rate(const OggFLAC__StreamDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	return FLAC__stream_decoder_get_sample_rate(decoder->private_->FLAC_stream_decoder);
}

OggFLAC_API unsigned OggFLAC__stream_decoder_get_blocksize(const OggFLAC__StreamDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	return FLAC__stream_decoder_get_blocksize(decoder->private_->FLAC_stream_decoder);
}

OggFLAC_API FLAC__bool OggFLAC__stream_decoder_flush(OggFLAC__StreamDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);

	OggFLAC__ogg_decoder_aspect_flush(&decoder->protected_->ogg_decoder_aspect);

	if(!FLAC__stream_decoder_flush(decoder->private_->FLAC_stream_decoder)) {
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

	if(!FLAC__stream_decoder_reset(decoder->private_->FLAC_stream_decoder)) {
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

	if(FLAC__stream_decoder_get_state(decoder->private_->FLAC_stream_decoder) == FLAC__STREAM_DECODER_END_OF_STREAM)
		decoder->protected_->state = OggFLAC__STREAM_DECODER_END_OF_STREAM;

	if(decoder->protected_->state == OggFLAC__STREAM_DECODER_END_OF_STREAM)
		return true;

	FLAC__ASSERT(decoder->protected_->state == OggFLAC__STREAM_DECODER_OK);

	ret = FLAC__stream_decoder_process_single(decoder->private_->FLAC_stream_decoder);
	if(!ret)
		decoder->protected_->state = OggFLAC__STREAM_DECODER_FLAC_STREAM_DECODER_ERROR;

	return ret;
}

OggFLAC_API FLAC__bool OggFLAC__stream_decoder_process_until_end_of_metadata(OggFLAC__StreamDecoder *decoder)
{
	FLAC__bool ret;
	FLAC__ASSERT(0 != decoder);

	if(FLAC__stream_decoder_get_state(decoder->private_->FLAC_stream_decoder) == FLAC__STREAM_DECODER_END_OF_STREAM)
		decoder->protected_->state = OggFLAC__STREAM_DECODER_END_OF_STREAM;

	if(decoder->protected_->state == OggFLAC__STREAM_DECODER_END_OF_STREAM)
		return true;

	FLAC__ASSERT(decoder->protected_->state == OggFLAC__STREAM_DECODER_OK);

	ret = FLAC__stream_decoder_process_until_end_of_metadata(decoder->private_->FLAC_stream_decoder);
	if(!ret)
		decoder->protected_->state = OggFLAC__STREAM_DECODER_FLAC_STREAM_DECODER_ERROR;

	return ret;
}

OggFLAC_API FLAC__bool OggFLAC__stream_decoder_process_until_end_of_stream(OggFLAC__StreamDecoder *decoder)
{
	FLAC__bool ret;
	FLAC__ASSERT(0 != decoder);

	if(FLAC__stream_decoder_get_state(decoder->private_->FLAC_stream_decoder) == FLAC__STREAM_DECODER_END_OF_STREAM)
		decoder->protected_->state = OggFLAC__STREAM_DECODER_END_OF_STREAM;

	if(decoder->protected_->state == OggFLAC__STREAM_DECODER_END_OF_STREAM)
		return true;

	FLAC__ASSERT(decoder->protected_->state == OggFLAC__STREAM_DECODER_OK);

	ret = FLAC__stream_decoder_process_until_end_of_stream(decoder->private_->FLAC_stream_decoder);
	if(!ret)
		decoder->protected_->state = OggFLAC__STREAM_DECODER_FLAC_STREAM_DECODER_ERROR;

	return ret;
}


/***********************************************************************
 *
 * Private class methods
 *
 ***********************************************************************/

void set_defaults_(OggFLAC__StreamDecoder *decoder)
{
	decoder->private_->read_callback = 0;
	decoder->private_->write_callback = 0;
	decoder->private_->metadata_callback = 0;
	decoder->private_->error_callback = 0;
	decoder->private_->client_data = 0;
	OggFLAC__ogg_decoder_aspect_set_defaults(&decoder->protected_->ogg_decoder_aspect);
}

FLAC__StreamDecoderReadStatus read_callback_(const FLAC__StreamDecoder *unused, FLAC__byte buffer[], unsigned *bytes, void *client_data)
{
	OggFLAC__StreamDecoder *decoder = (OggFLAC__StreamDecoder*)client_data;

	(void)unused;

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

FLAC__StreamDecoderWriteStatus write_callback_(const FLAC__StreamDecoder *unused, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data)
{
	OggFLAC__StreamDecoder *decoder = (OggFLAC__StreamDecoder*)client_data;
	(void)unused;
	return decoder->private_->write_callback(decoder, frame, buffer, decoder->private_->client_data);
}

void metadata_callback_(const FLAC__StreamDecoder *unused, const FLAC__StreamMetadata *metadata, void *client_data)
{
	OggFLAC__StreamDecoder *decoder = (OggFLAC__StreamDecoder*)client_data;
	(void)unused;
	decoder->private_->metadata_callback(decoder, metadata, decoder->private_->client_data);
}

void error_callback_(const FLAC__StreamDecoder *unused, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
	OggFLAC__StreamDecoder *decoder = (OggFLAC__StreamDecoder*)client_data;
	(void)unused;
	decoder->private_->error_callback(decoder, status, decoder->private_->client_data);
}

OggFLAC__OggDecoderAspectReadStatus read_callback_proxy_(const void *void_decoder, FLAC__byte buffer[], unsigned *bytes, void *client_data)
{
	OggFLAC__StreamDecoder *decoder = (OggFLAC__StreamDecoder*)void_decoder;

	switch(decoder->private_->read_callback(decoder, buffer, bytes, client_data)) {
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
