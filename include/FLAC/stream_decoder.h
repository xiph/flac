/* libFLAC - Free Lossless Audio Codec library
 * Copyright (C) 2000,2001,2002  Josh Coalson
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

#ifdef __cplusplus
extern "C" {
#endif


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
	FLAC__STREAM_DECODER_INVALID_CALLBACK,
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
	struct FLAC__StreamDecoderProtected *protected_; /* avoid the C++ keyword 'protected' */
	struct FLAC__StreamDecoderPrivate *private_; /* avoid the C++ keyword 'private' */
} FLAC__StreamDecoder;

/***********************************************************************
 *
 * Class constructor/destructor
 *
 ***********************************************************************/

/*
 * Any parameters that are not set before FLAC__stream_decoder_init()
 * will take on the defaults from the constructor, shown below.
 * For more on what the parameters mean, see the documentation.
 *
 *        (*read_callback)()               (DEFAULT: NULL ) The callbacks are the only values that MUST be set before FLAC__stream_decoder_init()
 *        (*write_callback)()              (DEFAULT: NULL )
 *        (*metadata_callback)()           (DEFAULT: NULL )
 *        (*error_callback)()              (DEFAULT: NULL )
 * void*    client_data                    (DEFAULT: NULL ) passed back through the callbacks
 */
FLAC__StreamDecoder *FLAC__stream_decoder_new();
void FLAC__stream_decoder_delete(FLAC__StreamDecoder *);

/***********************************************************************
 *
 * Public class method prototypes
 *
 ***********************************************************************/

/*
 * Various "set" methods.  These may only be called when the decoder
 * is in the state FLAC__STREAM_DECODER_UNINITIALIZED, i.e. after
 * FLAC__stream_decoder_new() or FLAC__stream_decoder_finish(), but
 * before FLAC__stream_decoder_init().  If this is the case they will
 * return true, otherwise false.
 *
 * NOTE that these functions do not validate the values as many are
 * interdependent.  The FLAC__stream_decoder_init() function will do
 * this, so make sure to pay attention to the state returned by
 * FLAC__stream_decoder_init().
 *
 * Any parameters that are not set before FLAC__stream_decoder_init()
 * will take on the defaults from the constructor.  NOTE that
 * FLAC__stream_decoder_flush() or FLAC__stream_decoder_reset() do
 * NOT reset the values to the constructor defaults.
 */
FLAC__bool FLAC__stream_decoder_set_read_callback(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderReadStatus (*value)(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], unsigned *bytes, void *client_data));
FLAC__bool FLAC__stream_decoder_set_write_callback(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderWriteStatus (*value)(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 *buffer[], void *client_data));
FLAC__bool FLAC__stream_decoder_set_metadata_callback(const FLAC__StreamDecoder *decoder, void (*value)(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetaData *metadata, void *client_data));
FLAC__bool FLAC__stream_decoder_set_error_callback(const FLAC__StreamDecoder *decoder, void (*value)(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data));
FLAC__bool FLAC__stream_decoder_set_client_data(const FLAC__StreamDecoder *decoder, void *value);
/*
 * These deserve special attention.  By default, the decoder only calls the
 * metadata_callback for the STREAMINFO block.  These functions allow you to
 * tell the decoder explicitly which blocks to parse and return via the
 * metadata_callback and/or which to skip.  Use a _respond_all(), _ignore() ...
 * or _ignore_all(), _respond() ... sequence to exactly specify which blocks
 * to return.  Remember that some metadata blocks can be big so filtering out
 * the ones you don't use can reduce the memory requirements of the decoder.
 * Also note the special forms _respond/_ignore_application(id) for filtering
 * APPLICATION blocks based on the application ID.
 *
 * STREAMINFO and SEEKTABLE blocks are always parsed and used internally, but
 * they still can legally be filtered from the metadata_callback here.
 */
FLAC__bool FLAC__stream_decoder_set_metadata_respond(const FLAC__StreamDecoder *decoder, FLAC__MetaDataType type);
FLAC__bool FLAC__stream_decoder_set_metadata_respond_application(const FLAC__StreamDecoder *decoder, FLAC__byte id[4]);
FLAC__bool FLAC__stream_decoder_set_metadata_respond_all(const FLAC__StreamDecoder *decoder);
FLAC__bool FLAC__stream_decoder_set_metadata_ignore(const FLAC__StreamDecoder *decoder, FLAC__MetaDataType type);
FLAC__bool FLAC__stream_decoder_set_metadata_ignore_application(const FLAC__StreamDecoder *decoder, FLAC__byte id[4]);
FLAC__bool FLAC__stream_decoder_set_metadata_ignore_all(const FLAC__StreamDecoder *decoder);

/*
 * Methods to return the current stream decoder state, number
 * of channels, channel assignment, bits-per-sample, sample
 * rate in Hz, and blocksize in samples.  All but the decoder
 * state will only be valid after decoding has started.
 */
FLAC__StreamDecoderState FLAC__stream_decoder_get_state(const FLAC__StreamDecoder *decoder);
unsigned FLAC__stream_decoder_get_channels(const FLAC__StreamDecoder *decoder);
FLAC__ChannelAssignment FLAC__stream_decoder_get_channel_assignment(const FLAC__StreamDecoder *decoder);
unsigned FLAC__stream_decoder_get_bits_per_sample(const FLAC__StreamDecoder *decoder);
unsigned FLAC__stream_decoder_get_sample_rate(const FLAC__StreamDecoder *decoder);
unsigned FLAC__stream_decoder_get_blocksize(const FLAC__StreamDecoder *decoder);

/*
 * Initialize the instance; should be called after construction and
 * 'set' calls but before any of the 'process' calls.  Will set and
 * return the decoder state, which will be
 * FLAC__STREAM_DECODER_SEARCH_FOR_METADATA if initialization
 * succeeded.
 */
FLAC__StreamDecoderState FLAC__stream_decoder_init(FLAC__StreamDecoder *decoder);

/*
 * Flush the decoding buffer, release resources, and return the decoder
 * state to FLAC__STREAM_DECODER_UNINITIALIZED.
 */
void FLAC__stream_decoder_finish(FLAC__StreamDecoder *decoder);

/*
 * state control methods
 */
FLAC__bool FLAC__stream_decoder_flush(FLAC__StreamDecoder *decoder);
FLAC__bool FLAC__stream_decoder_reset(FLAC__StreamDecoder *decoder);

/*
 * Methods for decoding the data
 */
FLAC__bool FLAC__stream_decoder_process_whole_stream(FLAC__StreamDecoder *decoder);
FLAC__bool FLAC__stream_decoder_process_metadata(FLAC__StreamDecoder *decoder);
FLAC__bool FLAC__stream_decoder_process_one_frame(FLAC__StreamDecoder *decoder);
FLAC__bool FLAC__stream_decoder_process_remaining_frames(FLAC__StreamDecoder *decoder);

#ifdef __cplusplus
}
#endif

#endif
