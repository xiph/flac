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

#include <stdlib.h>
#include "FLAC/assert.h"
#include "protected/stream_decoder.h"

/***********************************************************************
 *
 * Private class method prototypes
 *
 ***********************************************************************/

static void set_defaults_(FLAC__StreamDecoder *decoder);


/***********************************************************************
 *
 * Private class data
 *
 ***********************************************************************/

typedef struct FLAC__StreamDecoderPrivate {
	FLAC__StreamDecoderReadCallback read_callback;
	FLAC__StreamDecoderWriteCallback write_callback;
	FLAC__StreamDecoderMetadataCallback metadata_callback;
	FLAC__StreamDecoderErrorCallback error_callback;
	void *client_data;
} FLAC__StreamDecoderPrivate;

/***********************************************************************
 *
 * Public static class data
 *
 ***********************************************************************/

const char * const FLAC__StreamDecoderStateString[] = {
	"OggFLAC__STREAM_DECODER_OK",
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
FLAC__StreamDecoder *FLAC__stream_decoder_new()
{
	FLAC__StreamDecoder *decoder;
	unsigned i;

	FLAC__ASSERT(sizeof(int) >= 4); /* we want to die right away if this is not true */

	decoder = (FLAC__StreamDecoder*)malloc(sizeof(FLAC__StreamDecoder));
	if(decoder == 0) {
		return 0;
	}
	memset(decoder, 0, sizeof(FLAC__StreamDecoder));

	decoder->protected_ = (FLAC__StreamDecoderProtected*)malloc(sizeof(FLAC__StreamDecoderProtected));
	if(decoder->protected_ == 0) {
		free(decoder);
		return 0;
	}
	memset(decoder->protected_, 0, sizeof(FLAC__StreamDecoderProtected));

	decoder->private_ = (FLAC__StreamDecoderPrivate*)malloc(sizeof(FLAC__StreamDecoderPrivate));
	if(decoder->private_ == 0) {
		free(decoder->protected_);
		free(decoder);
		return 0;
	}
	memset(decoder->private_, 0, sizeof(FLAC__StreamDecoderPrivate));

	decoder->private_->input = FLAC__bitbuffer_new();
	if(decoder->private_->input == 0) {
		free(decoder->private_);
		free(decoder->protected_);
		free(decoder);
		return 0;
	}

	decoder->private_->metadata_filter_ids_capacity = 16;
	if(0 == (decoder->private_->metadata_filter_ids = malloc((FLAC__STREAM_METADATA_APPLICATION_ID_LEN/8) * decoder->private_->metadata_filter_ids_capacity))) {
		FLAC__bitbuffer_delete(decoder->private_->input);
		free(decoder->private_);
		free(decoder->protected_);
		free(decoder);
		return 0;
	}

	for(i = 0; i < FLAC__MAX_CHANNELS; i++) {
		decoder->private_->output[i] = 0;
		decoder->private_->residual[i] = 0;
	}

	decoder->private_->output_capacity = 0;
	decoder->private_->output_channels = 0;
	decoder->private_->has_seek_table = false;

	set_defaults_(decoder);

	decoder->protected_->state = FLAC__STREAM_DECODER_UNINITIALIZED;

	return decoder;
}

void FLAC__stream_decoder_delete(FLAC__StreamDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->protected_);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->private_->input);

	FLAC__stream_decoder_finish(decoder);

	if(0 != decoder->private_->metadata_filter_ids)
		free(decoder->private_->metadata_filter_ids);

	FLAC__bitbuffer_delete(decoder->private_->input);
	free(decoder->private_);
	free(decoder->protected_);
	free(decoder);
}

/***********************************************************************
 *
 * Public class methods
 *
 ***********************************************************************/

FLAC__StreamDecoderState FLAC__stream_decoder_init(FLAC__StreamDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);

	if(decoder->protected_->state != FLAC__STREAM_DECODER_UNINITIALIZED)
		return decoder->protected_->state = FLAC__STREAM_DECODER_ALREADY_INITIALIZED;

	if(0 == decoder->private_->read_callback || 0 == decoder->private_->write_callback || 0 == decoder->private_->metadata_callback || 0 == decoder->private_->error_callback)
		return decoder->protected_->state = FLAC__STREAM_DECODER_INVALID_CALLBACK;

	if(!FLAC__bitbuffer_init(decoder->private_->input))
		return decoder->protected_->state = FLAC__STREAM_DECODER_MEMORY_ALLOCATION_ERROR;

	decoder->private_->last_frame_number = 0;
	decoder->private_->samples_decoded = 0;
	decoder->private_->has_stream_info = false;
	decoder->private_->cached = false;

	/*
	 * get the CPU info and set the function pointers
	 */
	FLAC__cpu_info(&decoder->private_->cpuinfo);
	/* first default to the non-asm routines */
	decoder->private_->local_lpc_restore_signal = FLAC__lpc_restore_signal;
	decoder->private_->local_lpc_restore_signal_16bit = FLAC__lpc_restore_signal;
	/* now override with asm where appropriate */
#ifndef FLAC__NO_ASM
	if(decoder->private_->cpuinfo.use_asm) {
#ifdef FLAC__CPU_IA32
		FLAC__ASSERT(decoder->private_->cpuinfo.type == FLAC__CPUINFO_TYPE_IA32);
#ifdef FLAC__HAS_NASM
		if(decoder->private_->cpuinfo.data.ia32.mmx) {
			decoder->private_->local_lpc_restore_signal = FLAC__lpc_restore_signal_asm_ia32;
			decoder->private_->local_lpc_restore_signal_16bit = FLAC__lpc_restore_signal_asm_ia32_mmx;
		}
		else {
			decoder->private_->local_lpc_restore_signal = FLAC__lpc_restore_signal_asm_ia32;
			decoder->private_->local_lpc_restore_signal_16bit = FLAC__lpc_restore_signal_asm_ia32;
		}
#endif
#endif
	}
#endif

	if(!FLAC__stream_decoder_reset(decoder))
		return decoder->protected_->state = FLAC__STREAM_DECODER_MEMORY_ALLOCATION_ERROR;

	return decoder->protected_->state;
}

void FLAC__stream_decoder_finish(FLAC__StreamDecoder *decoder)
{
	unsigned i;
	FLAC__ASSERT(0 != decoder);
	if(decoder->protected_->state == FLAC__STREAM_DECODER_UNINITIALIZED)
		return;
	if(decoder->private_->has_seek_table) {
		FLAC__ASSERT(0 != decoder->private_->seek_table.data.seek_table.points);
		free(decoder->private_->seek_table.data.seek_table.points);
		decoder->private_->seek_table.data.seek_table.points = 0;
		decoder->private_->has_seek_table = false;
	}
	FLAC__bitbuffer_free(decoder->private_->input);
	for(i = 0; i < FLAC__MAX_CHANNELS; i++) {
		/* WATCHOUT: FLAC__lpc_restore_signal_asm_ia32_mmx() requires that the output arrays have a buffer of up to 3 zeroes in front (at negative indices) for alignment purposes; we use 4 to keep the data well-aligned. */
		if(0 != decoder->private_->output[i]) {
			free(decoder->private_->output[i]-4);
			decoder->private_->output[i] = 0;
		}
		if(0 != decoder->private_->residual[i]) {
			free(decoder->private_->residual[i]);
			decoder->private_->residual[i] = 0;
		}
	}
	decoder->private_->output_capacity = 0;
	decoder->private_->output_channels = 0;

	set_defaults_(decoder);

	decoder->protected_->state = FLAC__STREAM_DECODER_UNINITIALIZED;
}

FLAC__bool FLAC__stream_decoder_set_read_callback(FLAC__StreamDecoder *decoder, FLAC__StreamDecoderReadCallback value)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	if(decoder->protected_->state != FLAC__STREAM_DECODER_UNINITIALIZED)
		return false;
	decoder->private_->read_callback = value;
	return true;
}

FLAC__bool FLAC__stream_decoder_set_write_callback(FLAC__StreamDecoder *decoder, FLAC__StreamDecoderWriteCallback value)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	if(decoder->protected_->state != FLAC__STREAM_DECODER_UNINITIALIZED)
		return false;
	decoder->private_->write_callback = value;
	return true;
}

FLAC__bool FLAC__stream_decoder_set_metadata_callback(FLAC__StreamDecoder *decoder, FLAC__StreamDecoderMetadataCallback value)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	if(decoder->protected_->state != FLAC__STREAM_DECODER_UNINITIALIZED)
		return false;
	decoder->private_->metadata_callback = value;
	return true;
}

FLAC__bool FLAC__stream_decoder_set_error_callback(FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorCallback value)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	if(decoder->protected_->state != FLAC__STREAM_DECODER_UNINITIALIZED)
		return false;
	decoder->private_->error_callback = value;
	return true;
}

FLAC__bool FLAC__stream_decoder_set_client_data(FLAC__StreamDecoder *decoder, void *value)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	if(decoder->protected_->state != FLAC__STREAM_DECODER_UNINITIALIZED)
		return false;
	decoder->private_->client_data = value;
	return true;
}

FLAC__bool FLAC__stream_decoder_set_metadata_respond(FLAC__StreamDecoder *decoder, FLAC__MetadataType type)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	FLAC__ASSERT(type <= FLAC__METADATA_TYPE_VORBIS_COMMENT);
	if(decoder->protected_->state != FLAC__STREAM_DECODER_UNINITIALIZED)
		return false;
	decoder->private_->metadata_filter[type] = true;
	if(type == FLAC__METADATA_TYPE_APPLICATION)
		decoder->private_->metadata_filter_ids_count = 0;
	return true;
}

FLAC__bool FLAC__stream_decoder_set_metadata_respond_application(FLAC__StreamDecoder *decoder, const FLAC__byte id[4])
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	FLAC__ASSERT(0 != id);
	if(decoder->protected_->state != FLAC__STREAM_DECODER_UNINITIALIZED)
		return false;

	if(decoder->private_->metadata_filter[FLAC__METADATA_TYPE_APPLICATION])
		return true;

	FLAC__ASSERT(0 != decoder->private_->metadata_filter_ids);

	if(decoder->private_->metadata_filter_ids_count == decoder->private_->metadata_filter_ids_capacity) {
		if(0 == (decoder->private_->metadata_filter_ids = realloc(decoder->private_->metadata_filter_ids, decoder->private_->metadata_filter_ids_capacity * 2)))
			return decoder->protected_->state = FLAC__STREAM_DECODER_MEMORY_ALLOCATION_ERROR;
		decoder->private_->metadata_filter_ids_capacity *= 2;
	}

	memcpy(decoder->private_->metadata_filter_ids + decoder->private_->metadata_filter_ids_count * (FLAC__STREAM_METADATA_APPLICATION_ID_LEN/8), id, (FLAC__STREAM_METADATA_APPLICATION_ID_LEN/8));
	decoder->private_->metadata_filter_ids_count++;

	return true;
}

FLAC__bool FLAC__stream_decoder_set_metadata_respond_all(FLAC__StreamDecoder *decoder)
{
	unsigned i;
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	if(decoder->protected_->state != FLAC__STREAM_DECODER_UNINITIALIZED)
		return false;
	for(i = 0; i < sizeof(decoder->private_->metadata_filter) / sizeof(decoder->private_->metadata_filter[0]); i++)
		decoder->private_->metadata_filter[i] = true;
	decoder->private_->metadata_filter_ids_count = 0;
	return true;
}

FLAC__bool FLAC__stream_decoder_set_metadata_ignore(FLAC__StreamDecoder *decoder, FLAC__MetadataType type)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	FLAC__ASSERT(type <= FLAC__METADATA_TYPE_VORBIS_COMMENT);
	if(decoder->protected_->state != FLAC__STREAM_DECODER_UNINITIALIZED)
		return false;
	decoder->private_->metadata_filter[type] = false;
	if(type == FLAC__METADATA_TYPE_APPLICATION)
		decoder->private_->metadata_filter_ids_count = 0;
	return true;
}

FLAC__bool FLAC__stream_decoder_set_metadata_ignore_application(FLAC__StreamDecoder *decoder, const FLAC__byte id[4])
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	FLAC__ASSERT(0 != id);
	if(decoder->protected_->state != FLAC__STREAM_DECODER_UNINITIALIZED)
		return false;

	if(!decoder->private_->metadata_filter[FLAC__METADATA_TYPE_APPLICATION])
		return true;

	FLAC__ASSERT(0 != decoder->private_->metadata_filter_ids);

	if(decoder->private_->metadata_filter_ids_count == decoder->private_->metadata_filter_ids_capacity) {
		if(0 == (decoder->private_->metadata_filter_ids = realloc(decoder->private_->metadata_filter_ids, decoder->private_->metadata_filter_ids_capacity * 2)))
			return decoder->protected_->state = FLAC__STREAM_DECODER_MEMORY_ALLOCATION_ERROR;
		decoder->private_->metadata_filter_ids_capacity *= 2;
	}

	memcpy(decoder->private_->metadata_filter_ids + decoder->private_->metadata_filter_ids_count * (FLAC__STREAM_METADATA_APPLICATION_ID_LEN/8), id, (FLAC__STREAM_METADATA_APPLICATION_ID_LEN/8));
	decoder->private_->metadata_filter_ids_count++;

	return true;
}

FLAC__bool FLAC__stream_decoder_set_metadata_ignore_all(FLAC__StreamDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);
	if(decoder->protected_->state != FLAC__STREAM_DECODER_UNINITIALIZED)
		return false;
	memset(decoder->private_->metadata_filter, 0, sizeof(decoder->private_->metadata_filter));
	decoder->private_->metadata_filter_ids_count = 0;
	return true;
}

FLAC__StreamDecoderState FLAC__stream_decoder_get_state(const FLAC__StreamDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->protected_);
	return decoder->protected_->state;
}

unsigned FLAC__stream_decoder_get_channels(const FLAC__StreamDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->protected_);
	return decoder->protected_->channels;
}

FLAC__ChannelAssignment FLAC__stream_decoder_get_channel_assignment(const FLAC__StreamDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->protected_);
	return decoder->protected_->channel_assignment;
}

unsigned FLAC__stream_decoder_get_bits_per_sample(const FLAC__StreamDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->protected_);
	return decoder->protected_->bits_per_sample;
}

unsigned FLAC__stream_decoder_get_sample_rate(const FLAC__StreamDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->protected_);
	return decoder->protected_->sample_rate;
}

unsigned FLAC__stream_decoder_get_blocksize(const FLAC__StreamDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->protected_);
	return decoder->protected_->blocksize;
}

FLAC__bool FLAC__stream_decoder_flush(FLAC__StreamDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);

	if(!FLAC__bitbuffer_clear(decoder->private_->input)) {
		decoder->protected_->state = FLAC__STREAM_DECODER_MEMORY_ALLOCATION_ERROR;
		return false;
	}
	decoder->protected_->state = FLAC__STREAM_DECODER_SEARCH_FOR_FRAME_SYNC;

	return true;
}

FLAC__bool FLAC__stream_decoder_reset(FLAC__StreamDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->private_);
	FLAC__ASSERT(0 != decoder->protected_);

	if(!FLAC__stream_decoder_flush(decoder)) {
		decoder->protected_->state = FLAC__STREAM_DECODER_MEMORY_ALLOCATION_ERROR;
		return false;
	}
	decoder->protected_->state = FLAC__STREAM_DECODER_SEARCH_FOR_METADATA;

	decoder->private_->samples_decoded = 0;

	return true;
}

FLAC__bool FLAC__stream_decoder_process_single(FLAC__StreamDecoder *decoder)
{
	FLAC__bool got_a_frame;
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->protected_);

	while(1) {
		switch(decoder->protected_->state) {
			case FLAC__STREAM_DECODER_SEARCH_FOR_METADATA:
				if(!find_metadata_(decoder))
					return false; /* above function sets the status for us */
				break;
			case FLAC__STREAM_DECODER_READ_METADATA:
				if(!read_metadata_(decoder))
					return false; /* above function sets the status for us */
				else
					return true;
			case FLAC__STREAM_DECODER_SEARCH_FOR_FRAME_SYNC:
				if(!frame_sync_(decoder))
					return true; /* above function sets the status for us */
				break;
			case FLAC__STREAM_DECODER_READ_FRAME:
				if(!read_frame_(decoder, &got_a_frame))
					return false; /* above function sets the status for us */
				if(got_a_frame)
					return true; /* above function sets the status for us */
				break;
			case FLAC__STREAM_DECODER_END_OF_STREAM:
			case FLAC__STREAM_DECODER_ABORTED:
				return true;
			default:
				FLAC__ASSERT(0);
				return false;
		}
	}
}

FLAC__bool FLAC__stream_decoder_process_until_end_of_metadata(FLAC__StreamDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->protected_);

	while(1) {
		switch(decoder->protected_->state) {
			case FLAC__STREAM_DECODER_SEARCH_FOR_METADATA:
				if(!find_metadata_(decoder))
					return false; /* above function sets the status for us */
				break;
			case FLAC__STREAM_DECODER_READ_METADATA:
				if(!read_metadata_(decoder))
					return false; /* above function sets the status for us */
				break;
			case FLAC__STREAM_DECODER_SEARCH_FOR_FRAME_SYNC:
			case FLAC__STREAM_DECODER_READ_FRAME:
			case FLAC__STREAM_DECODER_END_OF_STREAM:
			case FLAC__STREAM_DECODER_ABORTED:
				return true;
			default:
				FLAC__ASSERT(0);
				return false;
		}
	}
}

FLAC__bool FLAC__stream_decoder_process_until_end_of_stream(FLAC__StreamDecoder *decoder)
{
	FLAC__bool dummy;
	FLAC__ASSERT(0 != decoder);
	FLAC__ASSERT(0 != decoder->protected_);

	while(1) {
		switch(decoder->protected_->state) {
			case FLAC__STREAM_DECODER_SEARCH_FOR_METADATA:
				if(!find_metadata_(decoder))
					return false; /* above function sets the status for us */
				break;
			case FLAC__STREAM_DECODER_READ_METADATA:
				if(!read_metadata_(decoder))
					return false; /* above function sets the status for us */
				break;
			case FLAC__STREAM_DECODER_SEARCH_FOR_FRAME_SYNC:
				if(!frame_sync_(decoder))
					return true; /* above function sets the status for us */
				break;
			case FLAC__STREAM_DECODER_READ_FRAME:
				if(!read_frame_(decoder, &dummy))
					return false; /* above function sets the status for us */
				break;
			case FLAC__STREAM_DECODER_END_OF_STREAM:
			case FLAC__STREAM_DECODER_ABORTED:
				return true;
			default:
				FLAC__ASSERT(0);
				return false;
		}
	}
}

/***********************************************************************
 *
 * Protected class methods
 *
 ***********************************************************************/

unsigned FLAC__stream_decoder_get_input_bytes_unconsumed(const FLAC__StreamDecoder *decoder)
{
	FLAC__ASSERT(0 != decoder);
	return FLAC__bitbuffer_get_input_bytes_unconsumed(decoder->private_->input);
}

/***********************************************************************
 *
 * Private class methods
 *
 ***********************************************************************/

void set_defaults_(FLAC__StreamDecoder *decoder)
{
	decoder->private_->read_callback = 0;
	decoder->private_->write_callback = 0;
	decoder->private_->metadata_callback = 0;
	decoder->private_->error_callback = 0;
	decoder->private_->client_data = 0;

	memset(decoder->private_->metadata_filter, 0, sizeof(decoder->private_->metadata_filter));
	decoder->private_->metadata_filter[FLAC__METADATA_TYPE_STREAMINFO] = true;
	decoder->private_->metadata_filter_ids_count = 0;
}
