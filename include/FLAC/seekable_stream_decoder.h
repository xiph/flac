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

#ifndef FLAC__SEEKABLE_STREAM_DECODER_H
#define FLAC__SEEKABLE_STREAM_DECODER_H

#include "stream_decoder.h"

#ifdef __cplusplus
extern "C" {
#endif


/** \file include/FLAC/seekable_stream_decoder.h
 *
 *  \brief
 *  This module contains the functions which implement the seekable stream
 *  decoder.
 *
 *  See the detailed documentation in the
 *  \link flac_seekable_stream_decoder seekable stream decoder \endlink module.
 */

/** \defgroup flac_seekable_stream_decoder FLAC/seekable_stream_decoder.h: seekable stream decoder interface
 *  \ingroup flac_decoder
 *
 *  \brief
 *  This module contains the functions which implement the seekable stream
 *  decoder.
 *
 * The basic usage of this decoder is as follows:
 * - The program creates an instance of a decoder using
 *   FLAC__seekable_stream_decoder_new().
 * - The program overrides the default settings and sets callbacks for
 *   reading, writing, seeking, error reporting, and metadata reporting
 *   using FLAC__seekable_stream_decoder_set_*() functions.
 * - The program initializes the instance to validate the settings and
 *   prepare for decoding using FLAC__seekable_stream_decoder_init().
 * - The program calls the FLAC__seekable_stream_decoder_process_*()
 *   functions to decode data, which subsequently calls the callbacks.
 * - The program finishes the decoding with
 *   FLAC__seekable_stream_decoder_finish(), which flushes the input and
 *   output and resets the decoder to the uninitialized state.
 * - The instance may be used again or deleted with
 *   FLAC__seekable_stream_decoder_delete().
 *
 * The seekable stream decoder is a wrapper around the
 * \link flac_stream_decoder stream decoder \endlink which also provides
 * seeking capability.  In addition to the Read/Write/Metadata/Error
 * callbacks of the stream decoder, the user must also provide the following:
 *
 * - Seek callback - This function will be called when the decoder wants to
 *   seek to an absolute position in the stream.
 * - Tell callback - This function will be called when the decoder wants to
 *   know the current absolute position of the stream.
 * - Length callback - This function will be called when the decoder wants
 *   to know length of the stream.  The seeking algorithm currently requires
 *   that the overall stream length be known.
 * - EOF callback - This function will be called when the decoder wants to
 *   know if it is at the end of the stream.  This could be synthesized from
 *   the tell and length callbacks but it may be more expensive that way, so
 *   there is a separate callback for it.
 *
 * Seeking is exposed through the
 * FLAC__seekable_stream_decoder_seek_absolute() method.  At any point after
 * the seekable stream decoder has been initialized, the user can call this
 * function to seek to an exact sample within the stream.  Subsequently, the
 * first time the write callback is called it will be passed a (possibly
 * partial) block starting at that sample.
 *
 * The seekable stream decoder also provides MD5 signature checking.  If
 * this is turned on before initialization,
 * FLAC__seekable_stream_decoder_finish() will report when the decoded MD5
 * signature does not match the one stored in the STREAMINFO block.  MD5
 * checking is automatically turned off (until the next
 * FLAC__seekable_stream_decoder_reset()) if there is no signature in the
 * STREAMINFO block or when a seek is attempted.
 *
 * \note
 * The "set" functions may only be called when the decoder is in the
 * state FLAC__SEEKABLE_STREAM_DECODER_UNINITIALIZED, i.e. after
 * FLAC__seekable_stream_decoder_new() or
 * FLAC__seekable_stream_decoder_finish(), but before
 * FLAC__seekable_stream_decoder_init().  If this is the case they will
 * return \c true, otherwise \c false.
 *
 * \note
 * FLAC__stream_decoder_finish() resets all settings to the constructor
 * defaults, including the callbacks.
 *
 * \{
 */


/** State values for a FLAC__SeekableStreamDecoder
 *
 *  The decoder's state can be obtained by calling FLAC__seekable_tream_decoder_get_state().
 */
typedef enum {
    FLAC__SEEKABLE_STREAM_DECODER_OK = 0,
	FLAC__SEEKABLE_STREAM_DECODER_SEEKING,
	FLAC__SEEKABLE_STREAM_DECODER_END_OF_STREAM,
	FLAC__SEEKABLE_STREAM_DECODER_MEMORY_ALLOCATION_ERROR,
	FLAC__SEEKABLE_STREAM_DECODER_STREAM_DECODER_ERROR,
	FLAC__SEEKABLE_STREAM_DECODER_READ_ERROR,
	FLAC__SEEKABLE_STREAM_DECODER_SEEK_ERROR,
    FLAC__SEEKABLE_STREAM_DECODER_ALREADY_INITIALIZED,
    FLAC__SEEKABLE_STREAM_DECODER_INVALID_CALLBACK,
    FLAC__SEEKABLE_STREAM_DECODER_UNINITIALIZED
} FLAC__SeekableStreamDecoderState;
extern const char * const FLAC__SeekableStreamDecoderStateString[];

typedef enum {
	FLAC__SEEKABLE_STREAM_DECODER_READ_STATUS_OK,
	FLAC__SEEKABLE_STREAM_DECODER_READ_STATUS_ERROR
} FLAC__SeekableStreamDecoderReadStatus;
extern const char * const FLAC__SeekableStreamDecoderReadStatusString[];

typedef enum {
	FLAC__SEEKABLE_STREAM_DECODER_SEEK_STATUS_OK,
	FLAC__SEEKABLE_STREAM_DECODER_SEEK_STATUS_ERROR
} FLAC__SeekableStreamDecoderSeekStatus;
extern const char * const FLAC__SeekableStreamDecoderSeekStatusString[];

typedef enum {
	FLAC__SEEKABLE_STREAM_DECODER_TELL_STATUS_OK,
	FLAC__SEEKABLE_STREAM_DECODER_TELL_STATUS_ERROR
} FLAC__SeekableStreamDecoderTellStatus;
extern const char * const FLAC__SeekableStreamDecoderTellStatusString[];

typedef enum {
	FLAC__SEEKABLE_STREAM_DECODER_LENGTH_STATUS_OK,
	FLAC__SEEKABLE_STREAM_DECODER_LENGTH_STATUS_ERROR
} FLAC__SeekableStreamDecoderLengthStatus;
extern const char * const FLAC__SeekableStreamDecoderLengthStatusString[];

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
 *          metadata_respond/ignore        By default, only the STREAMINFO block is returned via metadata_callback()
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
FLAC__bool FLAC__seekable_stream_decoder_set_md5_checking(FLAC__SeekableStreamDecoder *decoder, FLAC__bool value);
FLAC__bool FLAC__seekable_stream_decoder_set_read_callback(FLAC__SeekableStreamDecoder *decoder, FLAC__SeekableStreamDecoderReadStatus (*value)(const FLAC__SeekableStreamDecoder *decoder, FLAC__byte buffer[], unsigned *bytes, void *client_data));
FLAC__bool FLAC__seekable_stream_decoder_set_seek_callback(FLAC__SeekableStreamDecoder *decoder, FLAC__SeekableStreamDecoderSeekStatus (*value)(const FLAC__SeekableStreamDecoder *decoder, FLAC__uint64 absolute_byte_offset, void *client_data));
FLAC__bool FLAC__seekable_stream_decoder_set_tell_callback(FLAC__SeekableStreamDecoder *decoder, FLAC__SeekableStreamDecoderTellStatus (*value)(const FLAC__SeekableStreamDecoder *decoder, FLAC__uint64 *absolute_byte_offset, void *client_data));
FLAC__bool FLAC__seekable_stream_decoder_set_length_callback(FLAC__SeekableStreamDecoder *decoder, FLAC__SeekableStreamDecoderLengthStatus (*value)(const FLAC__SeekableStreamDecoder *decoder, FLAC__uint64 *stream_length, void *client_data));
FLAC__bool FLAC__seekable_stream_decoder_set_eof_callback(FLAC__SeekableStreamDecoder *decoder, FLAC__bool (*value)(const FLAC__SeekableStreamDecoder *decoder, void *client_data));
FLAC__bool FLAC__seekable_stream_decoder_set_write_callback(FLAC__SeekableStreamDecoder *decoder, FLAC__StreamDecoderWriteStatus (*value)(const FLAC__SeekableStreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data));
FLAC__bool FLAC__seekable_stream_decoder_set_metadata_callback(FLAC__SeekableStreamDecoder *decoder, void (*value)(const FLAC__SeekableStreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data));
FLAC__bool FLAC__seekable_stream_decoder_set_error_callback(FLAC__SeekableStreamDecoder *decoder, void (*value)(const FLAC__SeekableStreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data));
FLAC__bool FLAC__seekable_stream_decoder_set_client_data(FLAC__SeekableStreamDecoder *decoder, void *value);
/*
 * See the comments for the equivalent functions in stream_decoder.h
 */
FLAC__bool FLAC__seekable_stream_decoder_set_metadata_respond(FLAC__SeekableStreamDecoder *decoder, FLAC__MetadataType type);
FLAC__bool FLAC__seekable_stream_decoder_set_metadata_respond_application(FLAC__SeekableStreamDecoder *decoder, const FLAC__byte id[4]);
FLAC__bool FLAC__seekable_stream_decoder_set_metadata_respond_all(FLAC__SeekableStreamDecoder *decoder);
FLAC__bool FLAC__seekable_stream_decoder_set_metadata_ignore(FLAC__SeekableStreamDecoder *decoder, FLAC__MetadataType type);
FLAC__bool FLAC__seekable_stream_decoder_set_metadata_ignore_application(FLAC__SeekableStreamDecoder *decoder, const FLAC__byte id[4]);
FLAC__bool FLAC__seekable_stream_decoder_set_metadata_ignore_all(FLAC__SeekableStreamDecoder *decoder);

/*
 * Various "get" methods
 */
FLAC__SeekableStreamDecoderState FLAC__seekable_stream_decoder_get_state(const FLAC__SeekableStreamDecoder *decoder);
FLAC__bool FLAC__seekable_stream_decoder_get_md5_checking(const FLAC__SeekableStreamDecoder *decoder);
/*
 * Methods to return the current number of channels, channel assignment
 * bits-per-sample, sample rate in Hz, and blocksize in samples.  These
 * will only be valid after decoding has started.
 */
unsigned FLAC__seekable_stream_decoder_get_channels(const FLAC__SeekableStreamDecoder *decoder);
FLAC__ChannelAssignment FLAC__seekable_stream_decoder_get_channel_assignment(const FLAC__SeekableStreamDecoder *decoder);
unsigned FLAC__seekable_stream_decoder_get_bits_per_sample(const FLAC__SeekableStreamDecoder *decoder);
unsigned FLAC__seekable_stream_decoder_get_sample_rate(const FLAC__SeekableStreamDecoder *decoder);
unsigned FLAC__seekable_stream_decoder_get_blocksize(const FLAC__SeekableStreamDecoder *decoder);

/*
 * Initialize the instance; should be called after construction and
 * 'set' calls but before any of the 'process' or 'seek' calls.  Will
 * set and return the decoder state, which will be
 *  FLAC__SEEKABLE_STREAM_DECODER_OK if initialization succeeded.
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
 * state control methods
 */
FLAC__bool FLAC__seekable_stream_decoder_flush(FLAC__SeekableStreamDecoder *decoder);
FLAC__bool FLAC__seekable_stream_decoder_reset(FLAC__SeekableStreamDecoder *decoder);

/*
 * Methods for decoding the data
 */
FLAC__bool FLAC__seekable_stream_decoder_process_whole_stream(FLAC__SeekableStreamDecoder *decoder);
FLAC__bool FLAC__seekable_stream_decoder_process_metadata(FLAC__SeekableStreamDecoder *decoder);
FLAC__bool FLAC__seekable_stream_decoder_process_one_frame(FLAC__SeekableStreamDecoder *decoder);
FLAC__bool FLAC__seekable_stream_decoder_process_remaining_frames(FLAC__SeekableStreamDecoder *decoder);

FLAC__bool FLAC__seekable_stream_decoder_seek_absolute(FLAC__SeekableStreamDecoder *decoder, FLAC__uint64 sample);

/* \} */

#ifdef __cplusplus
}
#endif

#endif
