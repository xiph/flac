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

#ifndef FLAC__SEEKABLE_STREAM_DECODER_H
#define FLAC__SEEKABLE_STREAM_DECODER_H

#include "stream_decoder.h"

typedef enum {
    FLAC__SEEKABLE_STREAM_DECODER_OK = 0,
	FLAC__SEEKABLE_STREAM_DECODER_SEEKING,
	FLAC__SEEKABLE_STREAM_DECODER_END_OF_STREAM,
	FLAC__SEEKABLE_STREAM_DECODER_MEMORY_ALLOCATION_ERROR,
	FLAC__SEEKABLE_STREAM_DECODER_STREAM_ERROR,
	FLAC__SEEKABLE_STREAM_DECODER_READ_ERROR,
	FLAC__SEEKABLE_STREAM_DECODER_SEEK_ERROR,
    FLAC__SEEKABLE_STREAM_DECODER_ALREADY_INITIALIZED,
    FLAC__SEEKABLE_STREAM_DECODER_INVALID_CALLBACK,
    FLAC__SEEKABLE_STREAM_DECODER_UNINITIALIZED
} FLAC__SeekableStreamDecoderState;
extern const char *FLAC__SeekableStreamDecoderStateString[];

typedef enum {
	FLAC__SEEKABLE_STREAM_DECODER_READ_STATUS_OK,
	FLAC__SEEKABLE_STREAM_DECODER_READ_STATUS_ERROR
} FLAC__SeekableStreamDecoderReadStatus;
extern const char *FLAC__SeekableStreamDecoderReadStatusString[];

typedef enum {
	FLAC__SEEKABLE_STREAM_DECODER_SEEK_STATUS_OK,
	FLAC__SEEKABLE_STREAM_DECODER_SEEK_STATUS_ERROR
} FLAC__SeekableStreamDecoderSeekStatus;
extern const char *FLAC__SeekableStreamDecoderSeekStatusString[];

typedef enum {
	FLAC__SEEKABLE_STREAM_DECODER_TELL_STATUS_OK,
	FLAC__SEEKABLE_STREAM_DECODER_TELL_STATUS_ERROR
} FLAC__SeekableStreamDecoderTellStatus;
extern const char *FLAC__SeekableStreamDecoderTellStatusString[];

typedef enum {
	FLAC__SEEKABLE_STREAM_DECODER_LENGTH_STATUS_OK,
	FLAC__SEEKABLE_STREAM_DECODER_LENGTH_STATUS_ERROR
} FLAC__SeekableStreamDecoderLengthStatus;
extern const char *FLAC__SeekableStreamDecoderLengthStatusString[];

/***********************************************************************
 *
 * class FLAC__SeekableStreamDecoder : public FLAC__StreamDecoder
 *
 ***********************************************************************/

struct FLAC__SeekableStreamDecoderProtected;
struct FLAC__SeekableStreamDecoderPrivate;
typedef struct {
	struct FLAC__SeekableStreamDecoderProtected *protected_; /* avoid the C++ keyword 'protected' */
	struct FLAC__SeekableStreamDecoderPrivate *private_; /* avoid the C++ keyword 'private' */
} FLAC__SeekableStreamDecoder;

/***********************************************************************
 *
 * Class constructor/destructor
 *
 ***********************************************************************/

/*
 * Any parameters that are not set before FLAC__seekable_stream_decoder_init()
 * will take on the defaults from the constructor, shown below.
 * For more on what the parameters mean, see the documentation.
 *
 * FLAC__bool  md5_checking                   (DEFAULT: false) MD5 checking will be turned off if a seek is requested
 *           (*read_callback)()               (DEFAULT: NULL ) The callbacks are the only values that MUST be set before FLAC__seekable_stream_decoder_init()
 *           (*seek_callback)()               (DEFAULT: NULL )
 *           (*tell_callback)()               (DEFAULT: NULL )
 *           (*length_callback)()             (DEFAULT: NULL )
 *           (*eof_callback)()                (DEFAULT: NULL )
 *           (*write_callback)()              (DEFAULT: NULL )
 *           (*metadata_callback)()           (DEFAULT: NULL )
 *           (*error_callback)()              (DEFAULT: NULL )
 * void*       client_data                    (DEFAULT: NULL ) passed back through the callbacks
 */
FLAC__SeekableStreamDecoder *FLAC__seekable_stream_decoder_new();
void FLAC__seekable_stream_decoder_delete(FLAC__SeekableStreamDecoder *);

/***********************************************************************
 *
 * Public class method prototypes
 *
 ***********************************************************************/

/*
 * Various "set" methods.  These may only be called when the decoder
 * is in the state FLAC__SEEKABLE_STREAM_DECODER_UNINITIALIZED, i.e. after
 * FLAC__seekable_stream_decoder_new() or FLAC__seekable_stream_decoder_finish(),
 * but before FLAC__seekable_stream_decoder_init().  If this is the case they
 * will return true, otherwise false.
 *
 * NOTE that these functions do not validate the values as many are
 * interdependent.  The FLAC__seekable_stream_decoder_init() function will
 * do this, so make sure to pay attention to the state returned by
 * FLAC__seekable_stream_decoder_init().
 *
 * Any parameters that are not set before FLAC__seekable_stream_decoder_init()
 * will take on the defaults from the constructor.  NOTE that
 * FLAC__seekable_stream_decoder_flush() or FLAC__seekable_stream_decoder_reset()
 * do NOT reset the values to the constructor defaults.
 */
FLAC__bool FLAC__seekable_stream_decoder_set_md5_checking(const FLAC__SeekableStreamDecoder *decoder, FLAC__bool value);
FLAC__bool FLAC__seekable_stream_decoder_set_read_callback(const FLAC__SeekableStreamDecoder *decoder, FLAC__SeekableStreamDecoderReadStatus (*value)(const FLAC__SeekableStreamDecoder *decoder, FLAC__byte buffer[], unsigned *bytes, void *client_data));
FLAC__bool FLAC__seekable_stream_decoder_set_seek_callback(const FLAC__SeekableStreamDecoder *decoder, FLAC__SeekableStreamDecoderSeekStatus (*value)(const FLAC__SeekableStreamDecoder *decoder, FLAC__uint64 absolute_byte_offset, void *client_data));
FLAC__bool FLAC__seekable_stream_decoder_set_tell_callback(const FLAC__SeekableStreamDecoder *decoder, FLAC__SeekableStreamDecoderTellStatus (*value)(const FLAC__SeekableStreamDecoder *decoder, FLAC__uint64 *absolute_byte_offset, void *client_data));
FLAC__bool FLAC__seekable_stream_decoder_set_length_callback(const FLAC__SeekableStreamDecoder *decoder, FLAC__SeekableStreamDecoderLengthStatus (*value)(const FLAC__SeekableStreamDecoder *decoder, FLAC__uint64 *stream_length, void *client_data));
FLAC__bool FLAC__seekable_stream_decoder_set_eof_callback(const FLAC__SeekableStreamDecoder *decoder, FLAC__bool (*value)(const FLAC__SeekableStreamDecoder *decoder, void *client_data));
FLAC__bool FLAC__seekable_stream_decoder_set_write_callback(const FLAC__SeekableStreamDecoder *decoder, FLAC__StreamDecoderWriteStatus (*value)(const FLAC__SeekableStreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 *buffer[], void *client_data));
FLAC__bool FLAC__seekable_stream_decoder_set_metadata_callback(const FLAC__SeekableStreamDecoder *decoder, void (*value)(const FLAC__SeekableStreamDecoder *decoder, const FLAC__StreamMetaData *metadata, void *client_data));
FLAC__bool FLAC__seekable_stream_decoder_set_error_callback(const FLAC__SeekableStreamDecoder *decoder, void (*value)(const FLAC__SeekableStreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data));
FLAC__bool FLAC__seekable_stream_decoder_set_client_data(const FLAC__SeekableStreamDecoder *decoder, void *value);

/*
 * Various "get" methods
 */
FLAC__SeekableStreamDecoderState FLAC__seekable_stream_decoder_get_state(const FLAC__SeekableStreamDecoder *decoder);
FLAC__bool FLAC__seekable_stream_decoder_get_md5_checking(const FLAC__SeekableStreamDecoder *decoder);

/*
 * Initialize the instance; should be called after construction and
 * 'set' calls but before any of the 'process' or 'seek' calls.  Will
 * set and return the decoder state, which will be FLAC__SEEKABLE_STREAM_DECODER_OK
 * if initialization succeeded.
 */
FLAC__SeekableStreamDecoderState FLAC__seekable_stream_decoder_init(FLAC__SeekableStreamDecoder *decoder);

/*
 * Flush the decoding buffer, release resources, and return the decoder
 * state to FLAC__SEEKABLE_STREAM_DECODER_UNINITIALIZED.  Only returns false if
 * md5_checking is set AND the stored MD5 sum is non-zero AND the stored
 * MD5 sum and computed MD5 sum do not match.
 */
FLAC__bool FLAC__seekable_stream_decoder_finish(FLAC__SeekableStreamDecoder *decoder);

/*
 * Methods for decoding the data
 */
FLAC__bool FLAC__seekable_stream_decoder_process_whole_stream(FLAC__SeekableStreamDecoder *decoder);
FLAC__bool FLAC__seekable_stream_decoder_process_metadata(FLAC__SeekableStreamDecoder *decoder);
FLAC__bool FLAC__seekable_stream_decoder_process_one_frame(FLAC__SeekableStreamDecoder *decoder);
FLAC__bool FLAC__seekable_stream_decoder_process_remaining_frames(FLAC__SeekableStreamDecoder *decoder);

FLAC__bool FLAC__seekable_stream_decoder_seek_absolute(FLAC__SeekableStreamDecoder *decoder, FLAC__uint64 sample);

#endif
