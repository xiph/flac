/* libOggFLAC - Free Lossless Audio Codec + Ogg library
 * Copyright (C) 2002  Josh Coalson
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

#include <stdlib.h> /* for calloc() */
#include <string.h> /* for memset() */
#include "ogg/ogg.h"
#include "FLAC/assert.h"
#include "protected/stream_decoder.h"

#ifdef min
#undef min
#endif
#define min(x,y) ((x)<(y)?(x):(y))

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
	struct {
		ogg_stream_state stream_state;
		ogg_sync_state sync_state;
		FLAC__bool need_serial_number;
	} ogg;
} OggFLAC__StreamDecoderPrivate;

/***********************************************************************
 *
 * Public static class data
 *
 ***********************************************************************/

const OggFLAC_API char * const OggFLAC__StreamDecoderStateString[] = {
	"OggFLAC__STREAM_DECODER_OK",
	"OggFLAC__STREAM_DECODER_OGG_ERROR",
	"OggFLAC__STREAM_DECODER_READ_ERROR",
	"OggFLAC__STREAM_DECODER_FLAC_STREAM_DECODER_ERROR",
	"OggFLAC__STREAM_DECODER_INVALID_CALLBACK",
	"OggFLAC__STREAM_DECODER_MEMORY_ALLOCATION_ERROR",
	"OggFLAC__STREAM_DECODER_ALREADY_INITIALIZED",
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

	decoder->private_->ogg.need_serial_number = decoder->protected_->use_first_serial_number;
	/* we will determine the serial number later if necessary */
	if(ogg_stream_init(&decoder->private_->ogg.stream_state, decoder->protected_->serial_number) != 0)
		return decoder->protected_->state = OggFLAC__STREAM_DECODER_OGG_ERROR;

	if(ogg_sync_init(&decoder->private_->ogg.sync_state) != 0)
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

	(void)ogg_sync_clear(&decoder->private_->ogg.sync_state);
	(void)ogg_stream_clear(&decoder->private_->ogg.stream_state);

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
	decoder->protected_->use_first_serial_number = false;
	decoder->protected_->serial_number = value;
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

	(void)ogg_sync_clear(&decoder->private_->ogg.sync_state);

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
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	return FLAC__stream_decoder_process_single(decoder->private_->FLAC_stream_decoder);
}

OggFLAC_API FLAC__bool OggFLAC__stream_decoder_process_until_end_of_metadata(OggFLAC__StreamDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	return FLAC__stream_decoder_process_until_end_of_metadata(decoder->private_->FLAC_stream_decoder);
}

OggFLAC_API FLAC__bool OggFLAC__stream_decoder_process_until_end_of_stream(OggFLAC__StreamDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	return FLAC__stream_decoder_process_until_end_of_stream(decoder->private_->FLAC_stream_decoder);
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
	decoder->protected_->use_first_serial_number = true;
}

FLAC__StreamDecoderReadStatus read_callback_(const FLAC__StreamDecoder *unused, FLAC__byte buffer[], unsigned *bytes, void *client_data)
{
	static const unsigned OGG_BYTES_CHUNK = 8192;
	OggFLAC__StreamDecoder *decoder = (OggFLAC__StreamDecoder*)client_data;
	unsigned ogg_bytes_to_read, ogg_bytes_read;
	ogg_page page;
	char *oggbuf;

	(void)unused;

	/*
	 * We have to be careful not to read in more than the
	 * FLAC__StreamDecoder says it has room for.  We know
	 * that the size of the decoded data must be no more
	 * than the encoded data we will read.
	 */
	ogg_bytes_to_read = min(*bytes, OGG_BYTES_CHUNK);
	oggbuf = ogg_sync_buffer(&decoder->private_->ogg.sync_state, ogg_bytes_to_read);

	if(decoder->private_->read_callback(decoder, oggbuf, &ogg_bytes_to_read, decoder->private_->client_data) != FLAC__STREAM_DECODER_READ_STATUS_CONTINUE) {
		decoder->protected_->state = OggFLAC__STREAM_DECODER_READ_ERROR;
		return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
	}
	ogg_bytes_read = ogg_bytes_to_read;

	if(ogg_sync_wrote(&decoder->private_->ogg.sync_state, ogg_bytes_read) < 0) {
		decoder->protected_->state = OggFLAC__STREAM_DECODER_READ_ERROR;
		return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
	}

	*bytes = 0;
	while(ogg_sync_pageout(&decoder->private_->ogg.sync_state, &page) == 1) {
		/* grab the serial number if necessary */
		if(decoder->private_->ogg.need_serial_number) {
			decoder->private_->ogg.stream_state.serialno = decoder->protected_->serial_number = ogg_page_serialno(&page);
			decoder->private_->ogg.need_serial_number = false;
		}
		if(ogg_stream_pagein(&decoder->private_->ogg.stream_state, &page) == 0) {
			ogg_packet packet;

			while(ogg_stream_packetout(&decoder->private_->ogg.stream_state, &packet) == 1) {
				memcpy(buffer, packet.packet, packet.bytes);
				*bytes += packet.bytes;
				buffer += packet.bytes;
			}
		} else {
			decoder->protected_->state = OggFLAC__STREAM_DECODER_READ_ERROR;
			return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
		}
	}

	return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
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
