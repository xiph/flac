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

#ifndef FLAC__FILE_DECODER_H
#define FLAC__FILE_DECODER_H

#include "stream_decoder.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    FLAC__FILE_DECODER_OK = 0,
	FLAC__FILE_DECODER_SEEKING, /*@@@ NOTE: unused; remove this in the next minor-version */
	FLAC__FILE_DECODER_END_OF_FILE,
    FLAC__FILE_DECODER_ERROR_OPENING_FILE,
    FLAC__FILE_DECODER_MEMORY_ALLOCATION_ERROR,
	FLAC__FILE_DECODER_SEEK_ERROR,
	FLAC__FILE_DECODER_STREAM_ERROR,
	FLAC__FILE_DECODER_MD5_ERROR, /*@@@ NOTE: unused; remove this in the next minor-version */
	FLAC__FILE_DECODER_STREAM_DECODER_ERROR,
    FLAC__FILE_DECODER_ALREADY_INITIALIZED,
    FLAC__FILE_DECODER_INVALID_CALLBACK,
    FLAC__FILE_DECODER_UNINITIALIZED,
	FLAC__FILE_DECODER_SEEKABLE_STREAM_ERROR = FLAC__FILE_DECODER_STREAM_ERROR /*@@@ NOTE: do the actual replacement in the next minor-version */
} FLAC__FileDecoderState;
extern const char *FLAC__FileDecoderStateString[];

/***********************************************************************
 *
 * class FLAC__FileDecoder : public FLAC__StreamDecoder
 *
 ***********************************************************************/

struct FLAC__FileDecoderProtected;
struct FLAC__FileDecoderPrivate;
typedef struct {
	struct FLAC__FileDecoderProtected *protected_; /* avoid the C++ keyword 'protected' */
	struct FLAC__FileDecoderPrivate *private_; /* avoid the C++ keyword 'private' */
} FLAC__FileDecoder;

/***********************************************************************
 *
 * Class constructor/destructor
 *
 ***********************************************************************/

/*
 * Any parameters that are not set before FLAC__file_decoder_init()
 * will take on the defaults from the constructor, shown below.
 * For more on what the parameters mean, see the documentation.
 *
 * FLAC__bool  md5_checking                   (DEFAULT: false) MD5 checking will be turned off if a seek is requested
 *           (*write_callback)()              (DEFAULT: NULL ) The callbacks are the only values that MUST be set before FLAC__file_decoder_init()
 *           (*metadata_callback)()           (DEFAULT: NULL )
 *           (*error_callback)()              (DEFAULT: NULL )
 * void*       client_data                    (DEFAULT: NULL ) passed back through the callbacks
 */
FLAC__FileDecoder *FLAC__file_decoder_new();
void FLAC__file_decoder_delete(FLAC__FileDecoder *);

/***********************************************************************
 *
 * Public class method prototypes
 *
 ***********************************************************************/

/*
 * Various "set" methods.  These may only be called when the decoder
 * is in the state FLAC__FILE_DECODER_UNINITIALIZED, i.e. after
 * FLAC__file_decoder_new() or FLAC__file_decoder_finish(), but
 * before FLAC__file_decoder_init().  If this is the case they will
 * return true, otherwise false.
 *
 * NOTE that these functions do not validate the values as many are
 * interdependent.  The FLAC__file_decoder_init() function will do
 * this, so make sure to pay attention to the state returned by
 * FLAC__file_decoder_init().
 *
 * Any parameters that are not set before FLAC__file_decoder_init()
 * will take on the defaults from the constructor.  NOTE that
 * FLAC__file_decoder_flush() or FLAC__file_decoder_reset() do
 * NOT reset the values to the constructor defaults.
@@@@ update so that only _set_ methods that need to return FLAC__bool, else void; update documentation.html also
@@@@ update defaults above and in documentation.html about the metadata_respond/ignore defaults
 */
FLAC__bool FLAC__file_decoder_set_md5_checking(FLAC__FileDecoder *decoder, FLAC__bool value);
FLAC__bool FLAC__file_decoder_set_filename(FLAC__FileDecoder *decoder, const char *value); /* 'value' may not be 0; use "-" for stdin */
FLAC__bool FLAC__file_decoder_set_write_callback(FLAC__FileDecoder *decoder, FLAC__StreamDecoderWriteStatus (*value)(const FLAC__FileDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 *buffer[], void *client_data));
FLAC__bool FLAC__file_decoder_set_metadata_callback(FLAC__FileDecoder *decoder, void (*value)(const FLAC__FileDecoder *decoder, const FLAC__StreamMetaData *metadata, void *client_data));
FLAC__bool FLAC__file_decoder_set_error_callback(FLAC__FileDecoder *decoder, void (*value)(const FLAC__FileDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data));
FLAC__bool FLAC__file_decoder_set_client_data(FLAC__FileDecoder *decoder, void *value);
/*
 * See the comments for the equivalent functions in stream_decoder.h
 */
FLAC__bool FLAC__file_decoder_set_metadata_respond(FLAC__FileDecoder *decoder, FLAC__MetaDataType type);
FLAC__bool FLAC__file_decoder_set_metadata_respond_application(FLAC__FileDecoder *decoder, const FLAC__byte id[4]);
FLAC__bool FLAC__file_decoder_set_metadata_respond_all(FLAC__FileDecoder *decoder);
FLAC__bool FLAC__file_decoder_set_metadata_ignore(FLAC__FileDecoder *decoder, FLAC__MetaDataType type);
FLAC__bool FLAC__file_decoder_set_metadata_ignore_application(FLAC__FileDecoder *decoder, const FLAC__byte id[4]);
FLAC__bool FLAC__file_decoder_set_metadata_ignore_all(FLAC__FileDecoder *decoder);

/*
 * Various "get" methods
 */
FLAC__FileDecoderState FLAC__file_decoder_get_state(const FLAC__FileDecoder *decoder);
FLAC__bool FLAC__file_decoder_get_md5_checking(const FLAC__FileDecoder *decoder);

/*
 * Initialize the instance; should be called after construction and
 * 'set' calls but before any of the 'process' or 'seek' calls.  Will
 * set and return the decoder state, which will be FLAC__FILE_DECODER_OK
 * if initialization succeeded.
 */
FLAC__FileDecoderState FLAC__file_decoder_init(FLAC__FileDecoder *decoder);

/*
 * Flush the decoding buffer, release resources, and return the decoder
 * state to FLAC__FILE_DECODER_UNINITIALIZED.  Only returns false if
 * md5_checking is set AND the stored MD5 sum is non-zero AND the stored
 * MD5 sum and computed MD5 sum do not match.
 */
FLAC__bool FLAC__file_decoder_finish(FLAC__FileDecoder *decoder);

/*
 * Methods for decoding the data
 */
FLAC__bool FLAC__file_decoder_process_whole_file(FLAC__FileDecoder *decoder);
FLAC__bool FLAC__file_decoder_process_metadata(FLAC__FileDecoder *decoder);
FLAC__bool FLAC__file_decoder_process_one_frame(FLAC__FileDecoder *decoder);
FLAC__bool FLAC__file_decoder_process_remaining_frames(FLAC__FileDecoder *decoder);

FLAC__bool FLAC__file_decoder_seek_absolute(FLAC__FileDecoder *decoder, FLAC__uint64 sample);

#ifdef __cplusplus
}
#endif

#endif
