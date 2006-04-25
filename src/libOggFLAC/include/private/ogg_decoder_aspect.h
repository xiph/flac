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

#ifndef OggFLAC__PRIVATE__OGG_DECODER_ASPECT_H
#define OggFLAC__PRIVATE__OGG_DECODER_ASPECT_H

#include <ogg/ogg.h>

#include "FLAC/ordinals.h"
#include "FLAC/stream_decoder.h" /* for FLAC__StreamDecoderReadStatus */

typedef struct OggFLAC__OggDecoderAspect {
	/* these are storage for values that can be set through the API */
	FLAC__bool use_first_serial_number;
	long serial_number;

	/* these are for internal state related to Ogg decoding */
	ogg_stream_state stream_state;
	ogg_sync_state sync_state;
	unsigned version_major, version_minor;
	FLAC__bool need_serial_number;
	FLAC__bool end_of_stream;
	FLAC__bool have_working_page; /* only if true will the following vars be valid */
	ogg_page working_page;
	FLAC__bool have_working_packet; /* only if true will the following vars be valid */
	ogg_packet working_packet; /* as we work through the packet we will move working_packet.packet forward and working_packet.bytes down */
} OggFLAC__OggDecoderAspect;

void OggFLAC__ogg_decoder_aspect_set_serial_number(OggFLAC__OggDecoderAspect *aspect, long value);
void OggFLAC__ogg_decoder_aspect_set_defaults(OggFLAC__OggDecoderAspect *aspect);
FLAC__bool OggFLAC__ogg_decoder_aspect_init(OggFLAC__OggDecoderAspect *aspect);
void OggFLAC__ogg_decoder_aspect_finish(OggFLAC__OggDecoderAspect *aspect);
void OggFLAC__ogg_decoder_aspect_flush(OggFLAC__OggDecoderAspect *aspect);
void OggFLAC__ogg_decoder_aspect_reset(OggFLAC__OggDecoderAspect *aspect);

typedef enum {
	OggFLAC__OGG_DECODER_ASPECT_READ_STATUS_OK = 0,
	OggFLAC__OGG_DECODER_ASPECT_READ_STATUS_END_OF_STREAM,
	OggFLAC__OGG_DECODER_ASPECT_READ_STATUS_LOST_SYNC,
	OggFLAC__OGG_DECODER_ASPECT_READ_STATUS_NOT_FLAC,
	OggFLAC__OGG_DECODER_ASPECT_READ_STATUS_UNSUPPORTED_MAPPING_VERSION,
	OggFLAC__OGG_DECODER_ASPECT_READ_STATUS_ABORT,
	OggFLAC__OGG_DECODER_ASPECT_READ_STATUS_ERROR,
	OggFLAC__OGG_DECODER_ASPECT_READ_STATUS_MEMORY_ALLOCATION_ERROR
} OggFLAC__OggDecoderAspectReadStatus;

typedef OggFLAC__OggDecoderAspectReadStatus (*OggFLAC__OggDecoderAspectReadCallbackProxy)(const void *decoder, FLAC__byte buffer[], unsigned *bytes, void *client_data);

OggFLAC__OggDecoderAspectReadStatus OggFLAC__ogg_decoder_aspect_read_callback_wrapper(OggFLAC__OggDecoderAspect *aspect, FLAC__byte buffer[], unsigned *bytes, OggFLAC__OggDecoderAspectReadCallbackProxy read_callback, void *decoder, void *client_data);

#endif
