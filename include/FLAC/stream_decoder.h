/* libFLAC - Free Lossless Audio Coder library
 * Copyright (C) 2000  Josh Coalson
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

#ifndef FLAC__STREAM_DECODER_H
#define FLAC__STREAM_DECODER_H

#include "format.h"

typedef enum {
	FLAC__STREAM_DECODER_SEARCH_FOR_METADATA,
	FLAC__STREAM_DECODER_READ_METADATA,
	FLAC__STREAM_DECODER_SEARCH_FOR_FRAME_SYNC,
	FLAC__STREAM_DECODER_READ_FRAME,
	FLAC__STREAM_DECODER_RESYNC_IN_HEADER,
	FLAC__STREAM_DECODER_END_OF_STREAM,
	FLAC__STREAM_DECODER_ABORTED,
	FLAC__STREAM_DECODER_UNPARSEABLE_STREAM,
	FLAC__STREAM_DECODER_MEMORY_ALLOCATION_ERROR,
	FLAC__STREAM_DECODER_UNINITIALIZED
} FLAC__StreamDecoderState;

typedef enum {
	FLAC__STREAM_DECODER_READ_CONTINUE,
	FLAC__STREAM_DECODER_READ_END_OF_STREAM,
	FLAC__STREAM_DECODER_READ_ABORT
} FLAC__StreamDecoderReadStatus;

typedef enum {
	FLAC__STREAM_DECODER_WRITE_CONTINUE,
	FLAC__STREAM_DECODER_WRITE_ABORT
} FLAC__StreamDecoderWriteStatus;

typedef enum {
	FLAC__STREAM_DECODER_ERROR_LOST_SYNC
} FLAC__StreamDecoderErrorStatus;

struct FLAC__StreamDecoderPrivate;
typedef struct {
	/* these fields are read-only and valid as of the last write_callback() */
	unsigned channels;
	FLAC__ChannelAssignment channel_assignment;
	unsigned bits_per_sample;
	unsigned sample_rate; /* in Hz */
	unsigned blocksize; /* in samples (per channel) */
	FLAC__StreamDecoderState state; /* must be FLAC__STREAM_DECODER_UNINITIALIZED when passed to FLAC__stream_decoder_init() */
	struct FLAC__StreamDecoderPrivate *guts; /* must be 0 when passed to FLAC__stream_decoder_init() */
} FLAC__StreamDecoder;

FLAC__StreamDecoder *FLAC__stream_decoder_get_new_instance();
void FLAC__stream_decoder_free_instance(FLAC__StreamDecoder *decoder);
FLAC__StreamDecoderState FLAC__stream_decoder_init(
	FLAC__StreamDecoder *decoder,
	FLAC__StreamDecoderReadStatus (*read_callback)(const FLAC__StreamDecoder *decoder, byte buffer[], unsigned *bytes, void *client_data),
	FLAC__StreamDecoderWriteStatus (*write_callback)(const FLAC__StreamDecoder *decoder, const FLAC__FrameHeader *header, const int32 *buffer[], void *client_data),
	void (*metadata_callback)(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetaData *metadata, void *client_data),
	void (*error_callback)(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data),
	void *client_data
);
void FLAC__stream_decoder_finish(FLAC__StreamDecoder *decoder);
bool FLAC__stream_decoder_flush(FLAC__StreamDecoder *decoder);
bool FLAC__stream_decoder_reset(FLAC__StreamDecoder *decoder);
bool FLAC__stream_decoder_process_whole_stream(FLAC__StreamDecoder *decoder);
bool FLAC__stream_decoder_process_metadata(FLAC__StreamDecoder *decoder);
bool FLAC__stream_decoder_process_one_frame(FLAC__StreamDecoder *decoder);
bool FLAC__stream_decoder_process_remaining_frames(FLAC__StreamDecoder *decoder);

#endif
