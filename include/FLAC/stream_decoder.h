/* libFLAC - Free Lossless Audio Codec library
 * Copyright (C) 2000,2001  Josh Coalson
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
	FLAC__STREAM_DECODER_SEARCH_FOR_METADATA = 0,
	FLAC__STREAM_DECODER_READ_METADATA,
	FLAC__STREAM_DECODER_SEARCH_FOR_FRAME_SYNC,
	FLAC__STREAM_DECODER_READ_FRAME,
	FLAC__STREAM_DECODER_END_OF_STREAM,
	FLAC__STREAM_DECODER_ABORTED,
	FLAC__STREAM_DECODER_UNPARSEABLE_STREAM,
	FLAC__STREAM_DECODER_MEMORY_ALLOCATION_ERROR,
	FLAC__STREAM_DECODER_ALREADY_INITIALIZED,
	FLAC__STREAM_DECODER_UNINITIALIZED
} FLAC__StreamDecoderState;
extern const char *FLAC__StreamDecoderStateString[];

typedef enum {
	FLAC__STREAM_DECODER_READ_CONTINUE,
	FLAC__STREAM_DECODER_READ_END_OF_STREAM,
	FLAC__STREAM_DECODER_READ_ABORT
} FLAC__StreamDecoderReadStatus;
extern const char *FLAC__StreamDecoderReadStatusString[];

typedef enum {
	FLAC__STREAM_DECODER_WRITE_CONTINUE,
	FLAC__STREAM_DECODER_WRITE_ABORT
} FLAC__StreamDecoderWriteStatus;
extern const char *FLAC__StreamDecoderWriteStatusString[];

typedef enum {
	FLAC__STREAM_DECODER_ERROR_LOST_SYNC,
	FLAC__STREAM_DECODER_ERROR_BAD_HEADER,
	FLAC__STREAM_DECODER_ERROR_FRAME_CRC_MISMATCH
} FLAC__StreamDecoderErrorStatus;
extern const char *FLAC__StreamDecoderErrorStatusString[];

/***********************************************************************
 *
 * class FLAC__StreamDecoder
 *
 ***********************************************************************/

struct FLAC__StreamDecoderProtected;
struct FLAC__StreamDecoderPrivate;
typedef struct {
	struct FLAC__StreamDecoderProtected *protected;
	struct FLAC__StreamDecoderPrivate *private;
} FLAC__StreamDecoder;

/***********************************************************************
 *
 * Class constructor/destructor
 *
 ***********************************************************************/

FLAC__StreamDecoder *FLAC__stream_decoder_new();
void FLAC__stream_decoder_delete(FLAC__StreamDecoder *);

/***********************************************************************
 *
 * Public class method prototypes
 *
 ***********************************************************************/

/*
 * Initialize the instance; should be called after construction and
 * before any other calls.  Will set and return the decoder state which
 * will be FLAC__STREAM_DECODER_SEARCH_FOR_METADATA if initialization
 * succeeded.
 */
FLAC__StreamDecoderState FLAC__stream_decoder_init(
	FLAC__StreamDecoder *decoder,
	FLAC__StreamDecoderReadStatus (*read_callback)(const FLAC__StreamDecoder *decoder, byte buffer[], unsigned *bytes, void *client_data),
	FLAC__StreamDecoderWriteStatus (*write_callback)(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const int32 *buffer[], void *client_data),
	void (*metadata_callback)(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetaData *metadata, void *client_data),
	void (*error_callback)(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data),
	void *client_data
);
void FLAC__stream_decoder_finish(FLAC__StreamDecoder *decoder);

/*
 * methods to return the stream decoder state, number of channels,
 * channel assignment, bits-per-sample, sample rate in Hz, and
 * blocksize in samples.
 */
FLAC__StreamDecoderState FLAC__stream_decoder_state(const FLAC__StreamDecoder *decoder);
unsigned FLAC__stream_decoder_channels(const FLAC__StreamDecoder *decoder);
FLAC__ChannelAssignment FLAC__stream_decoder_channel_assignment(const FLAC__StreamDecoder *decoder);
unsigned FLAC__stream_decoder_bits_per_sample(const FLAC__StreamDecoder *decoder);
unsigned FLAC__stream_decoder_sample_rate(const FLAC__StreamDecoder *decoder);
unsigned FLAC__stream_decoder_blocksize(const FLAC__StreamDecoder *decoder);

/*
 * state control methods
 */
bool FLAC__stream_decoder_flush(FLAC__StreamDecoder *decoder);
bool FLAC__stream_decoder_reset(FLAC__StreamDecoder *decoder);

/*
 * methods for decoding the data
 */
bool FLAC__stream_decoder_process_whole_stream(FLAC__StreamDecoder *decoder);
bool FLAC__stream_decoder_process_metadata(FLAC__StreamDecoder *decoder);
bool FLAC__stream_decoder_process_one_frame(FLAC__StreamDecoder *decoder);
bool FLAC__stream_decoder_process_remaining_frames(FLAC__StreamDecoder *decoder);

#endif
