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

#include <string.h> /* for memcpy() */
#include "FLAC/assert.h"
#include "private/ogg_decoder_aspect.h"

#ifdef min
#undef min
#endif
#define min(x,y) ((x)<(y)?(x):(y))

/***********************************************************************
 *
 * Public class methods
 *
 ***********************************************************************/

FLAC__bool OggFLAC__ogg_decoder_aspect_init(OggFLAC__OggDecoderAspect *aspect)
{
	aspect->need_serial_number = aspect->use_first_serial_number;

	/* we will determine the serial number later if necessary */
	if(ogg_stream_init(&aspect->stream_state, aspect->serial_number) != 0)
		return false;

	if(ogg_sync_init(&aspect->sync_state) != 0)
		return false;

	return true;
}

void OggFLAC__ogg_decoder_aspect_finish(OggFLAC__OggDecoderAspect *aspect)
{
	(void)ogg_sync_clear(&aspect->sync_state);
	(void)ogg_stream_clear(&aspect->stream_state);
}

void OggFLAC__ogg_decoder_aspect_set_serial_number(OggFLAC__OggDecoderAspect *aspect, long value)
{
	aspect->use_first_serial_number = false;
	aspect->serial_number = value;
}

void OggFLAC__ogg_decoder_aspect_set_defaults(OggFLAC__OggDecoderAspect *aspect)
{
	aspect->use_first_serial_number = true;
}

void OggFLAC__ogg_decoder_aspect_flush(OggFLAC__OggDecoderAspect *aspect)
{
	OggFLAC__ogg_decoder_aspect_reset(aspect);
}

void OggFLAC__ogg_decoder_aspect_reset(OggFLAC__OggDecoderAspect *aspect)
{
	(void)ogg_stream_reset(&aspect->sync_state);
	(void)ogg_sync_reset(&aspect->sync_state);
}

OggFLAC__OggDecoderAspectReadStatus OggFLAC__ogg_decoder_aspect_read_callback_wrapper(OggFLAC__OggDecoderAspect *aspect, FLAC__byte buffer[], unsigned *bytes, OggFLAC__OggDecoderAspectReadCallbackProxy read_callback, void *decoder, void *client_data)
{
	static const unsigned OGG_BYTES_CHUNK = 8192;
	unsigned ogg_bytes_to_read, ogg_bytes_read;
	ogg_page page;
	char *oggbuf;

	/*
	 * We have to be careful not to read in more than the
	 * FLAC__StreamDecoder says it has room for.  We know
	 * that the size of the decoded data must be no more
	 * than the encoded data we will read.
	 */
	ogg_bytes_to_read = min(*bytes, OGG_BYTES_CHUNK);
	oggbuf = ogg_sync_buffer(&aspect->sync_state, ogg_bytes_to_read);

	if(0 == oggbuf)
		return OggFLAC__OGG_DECODER_ASPECT_READ_STATUS_MEMORY_ALLOCATION_ERROR;

	ogg_bytes_read = ogg_bytes_to_read;

	switch(read_callback(decoder, (FLAC__byte*)oggbuf, &ogg_bytes_read, client_data)) {
		case FLAC__STREAM_DECODER_READ_STATUS_CONTINUE:
			break;
		case FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM:
			return OggFLAC__OGG_DECODER_ASPECT_READ_STATUS_END_OF_STREAM;
		case FLAC__STREAM_DECODER_READ_STATUS_ABORT:
			return OggFLAC__OGG_DECODER_ASPECT_READ_STATUS_ABORT;
		default:
			FLAC__ASSERT(0);
	}

	if(ogg_sync_wrote(&aspect->sync_state, ogg_bytes_read) < 0)
		return OggFLAC__OGG_DECODER_ASPECT_READ_STATUS_ERROR;

	*bytes = 0;
	while(ogg_sync_pageout(&aspect->sync_state, &page) == 1) {
		/* grab the serial number if necessary */
		if(aspect->need_serial_number) {
			aspect->stream_state.serialno = aspect->serial_number = ogg_page_serialno(&page);
			aspect->need_serial_number = false;
		}
		if(ogg_stream_pagein(&aspect->stream_state, &page) == 0) {
			ogg_packet packet;

			while(ogg_stream_packetout(&aspect->stream_state, &packet) == 1) {
				memcpy(buffer, packet.packet, packet.bytes);
				*bytes += packet.bytes;
				buffer += packet.bytes;
			}
		} else {
			return OggFLAC__OGG_DECODER_ASPECT_READ_STATUS_ERROR;
		}
	}

	return OggFLAC__OGG_DECODER_ASPECT_READ_STATUS_OK;
}
