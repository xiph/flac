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

#include "seekable_stream_decoder.h"

#ifdef __cplusplus
extern "C" {
#endif


/** \file include/FLAC/file_decoder.h
 *
 *  \brief
 *  This module contains the functions which implement the file
 *  decoder.
 *
 *  See the detailed documentation in the
 *  \link flac_file_decoder file decoder \endlink module.
 */

/** \defgroup flac_file_decoder FLAC/file_decoder.h: file decoder interface
 *  \ingroup flac_decoder
 *
 *  \brief
 *  This module contains the functions which implement the file
 *  decoder.
 *
 * The basic usage of this decoder is as follows:
 * - The program creates an instance of a decoder using
 *   FLAC__file_decoder_new().
 * - The program overrides the default settings and sets callbacks for
 *   writing, error reporting, and metadata reporting using
 *   FLAC__file_decoder_set_*() functions.
 * - The program initializes the instance to validate the settings and
 *   prepare for decoding using FLAC__file_decoder_init().
 * - The program calls the FLAC__file_decoder_process_*() functions
 *   to decode data, which subsequently calls the callbacks.
 * - The program finishes the decoding with FLAC__file_decoder_finish(),
 *   which flushes the input and output and resets the decoder to the
 *   uninitialized state.
 * - The instance may be used again or deleted with
 *   FLAC__file_decoder_delete().
 *
 * The file decoder is a trivial wrapper around the
 * \link flac_seekable_stream_decoder seekable stream decoder \endlink
 * meant to simplfy the process of decoding from a standard file.  The
 * file decoder supplies all but the Write/Metadata/Error callbacks.
 * The user needs only to provide the path to the file and the file
 * decoder handles the rest.
 *
 * Like the seekable stream decoder, seeking is exposed through the
 * FLAC__file_decoder_seek_absolute() method.  At any point after the file
 * decoder has been initialized, the user can call this function to seek to
 * an exact sample within the file.  Subsequently, the first time the write
 * callback is called it will be passed a (possibly partial) block starting
 * at that sample.
 *
 * The file decoder also inherits MD5 signature checking from the seekable
 * stream decoder.  If this is turned on before initialization,
 * FLAC__file_decoder_finish() will report when the decoded MD5 signature
 * does not match the one stored in the STREAMINFO block.  MD5 checking is
 * automatically turned off if there is no signature in the STREAMINFO
 * block or when a seek is attempted.
 *
 * Make sure to read the detailed descriptions of the 
 * \link flac_seekable_stream_decoder seekable stream decoder module \endlink
 * and \link flac_stream_decoder stream decoder module \endlink
 * since the file decoder inherits much of its behavior from them.
 *
 * \note
 * The "set" functions may only be called when the decoder is in the
 * state FLAC__FILE_DECODER_UNINITIALIZED, i.e. after
 * FLAC__file_decoder_new() or FLAC__file_decoder_finish(), but
 * before FLAC__file_decoder_init().  If this is the case they will
 * return \c true, otherwise \c false.
 *
 * \note
 * FLAC__file_decoder_finish() resets all settings to the constructor
 * defaults, including the callbacks.
 *
 * \{
 */


/** State values for a FLAC__FileDecoder
 *
 *  The decoder's state can be obtained by calling FLAC__file_decoder_get_state().
 */
typedef enum {

    FLAC__FILE_DECODER_OK = 0,
	/**< The decoder is in the normal OK state. */

	FLAC__FILE_DECODER_END_OF_FILE,
	/**< The decoder has reached the end of the file. */

    FLAC__FILE_DECODER_ERROR_OPENING_FILE,
	/**< An error occurred opening the input file. */

    FLAC__FILE_DECODER_MEMORY_ALLOCATION_ERROR,
	/**< An error occurred allocating memory. */

	FLAC__FILE_DECODER_SEEK_ERROR,
	/**< An error occurred while seeking. */

	FLAC__FILE_DECODER_SEEKABLE_STREAM_DECODER_ERROR,
	/**< An error occurred in the underlying seekable stream decoder. */

    FLAC__FILE_DECODER_ALREADY_INITIALIZED,
	/**< FLAC__file_decoder_init() was called when the decoder was already
	 * initialized, usually because FLAC__file_decoder_finish() was not
     * called.
	 */

    FLAC__FILE_DECODER_INVALID_CALLBACK,
	/**< FLAC__file_decoder_init() was called without all callbacks
	 * being set.
	 */

    FLAC__FILE_DECODER_UNINITIALIZED
	/**< The decoder is in the uninitialized state. */

} FLAC__FileDecoderState;

/** Maps a FLAC__FileDecoderState to a C string.
 *
 *  Using a FLAC__FileDecoderState as the index to this array
 *  will give the string equivalent.  The contents should not be modified.
 */
extern const char * const FLAC__FileDecoderStateString[];


/***********************************************************************
 *
 * class FLAC__FileDecoder : public FLAC__StreamDecoder
 *
 ***********************************************************************/

struct FLAC__FileDecoderProtected;
struct FLAC__FileDecoderPrivate;
/** The opaque structure definition for the file decoder type.  See the
 *  \link flac_file_decoder file decoder module \endlink for a detailed
 *  description.
 */
typedef struct {
	struct FLAC__FileDecoderProtected *protected_; /* avoid the C++ keyword 'protected' */
	struct FLAC__FileDecoderPrivate *private_; /* avoid the C++ keyword 'private' */
} FLAC__FileDecoder;

/* @@@@ document */
typedef FLAC__StreamDecoderWriteStatus (*FLAC__FileDecoderWriteCallback)(const FLAC__FileDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data);
typedef void (*FLAC__FileDecoderMetadataCallback)(const FLAC__FileDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data);
typedef void (*FLAC__FileDecoderErrorCallback)(const FLAC__FileDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data);


/***********************************************************************
 *
 * Class constructor/destructor
 *
 ***********************************************************************/

/** Create a new file decoder instance.  The instance is created with
 *  default settings; see the individual FLAC__file_decoder_set_*()
 *  functions for each setting's default.
 *
 * \retval FLAC__FileDecoder*
 *    \c NULL if there was an error allocating memory, else the new instance.
 */
FLAC__FileDecoder *FLAC__file_decoder_new();

/** Free a decoder instance.  Deletes the object pointed to by \a decoder.
 *
 * \param decoder  A pointer to an existing decoder.
 * \assert
 *    \code decoder != NULL \endcode
 */
void FLAC__file_decoder_delete(FLAC__FileDecoder *decoder);


/***********************************************************************
 *
 * Public class method prototypes
 *
 ***********************************************************************/

/** Set the "MD5 signature checking" flag.
 *  This is inherited from FLAC__SeekableStreamDecoder; see
 *  FLAC__seekable_stream_decoder_set_md5_checking().
 *
 * \default \c false
 * \param  decoder  A decoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the decoder is already initialized, else \c true.
 */
FLAC__bool FLAC__file_decoder_set_md5_checking(FLAC__FileDecoder *decoder, FLAC__bool value);

/** Set the input file name to decode.
 *
 * \default \c "-"
 * \param  decoder  A decoder instance to set.
 * \param  value    The input file name, or "-" for \c stdin.
 * \assert
 *    \code decoder != NULL \endcode
 *    \code value != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the decoder is already initialized, or there was a memory
 *    allocation error, else \c true.
 */
FLAC__bool FLAC__file_decoder_set_filename(FLAC__FileDecoder *decoder, const char *value);

/** Set the write callback.
 *  This is inherited from FLAC__SeekableStreamDecoder; see
 *  FLAC__seekable_stream_decoder_set_write_callback().
 *
 * \note
 * The callback is mandatory and must be set before initialization.
 *
 * \default \c NULL
 * \param  decoder  A decoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code decoder != NULL \endcode
 *    \code value != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the decoder is already initialized, else \c true.
 */
FLAC__bool FLAC__file_decoder_set_write_callback(FLAC__FileDecoder *decoder, FLAC__FileDecoderWriteCallback value);

/** Set the metadata callback.
 *  This is inherited from FLAC__SeekableStreamDecoder; see
 *  FLAC__seekable_stream_decoder_set_metadata_callback().
 *
 * \note
 * The callback is mandatory and must be set before initialization.
 *
 * \default \c NULL
 * \param  decoder  A decoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code decoder != NULL \endcode
 *    \code value != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the decoder is already initialized, else \c true.
 */
FLAC__bool FLAC__file_decoder_set_metadata_callback(FLAC__FileDecoder *decoder, FLAC__FileDecoderMetadataCallback value);

/** Set the error callback.
 *  This is inherited from FLAC__SeekableStreamDecoder; see
 *  FLAC__seekable_stream_decoder_set_error_callback().
 *
 * \note
 * The callback is mandatory and must be set before initialization.
 *
 * \default \c NULL
 * \param  decoder  A decoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code decoder != NULL \endcode
 *    \code value != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the decoder is already initialized, else \c true.
 */
FLAC__bool FLAC__file_decoder_set_error_callback(FLAC__FileDecoder *decoder, FLAC__FileDecoderErrorCallback value);

/** Set the client data to be passed back to callbacks.
 *  This value will be supplied to callbacks in their \a client_data
 *  argument.
 *
 * \default \c NULL
 * \param  decoder  An decoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the decoder is already initialized, else \c true.
 */
FLAC__bool FLAC__file_decoder_set_client_data(FLAC__FileDecoder *decoder, void *value);

/** This is inherited from FLAC__SeekableStreamDecoder; see
 *  FLAC__seekable_stream_decoder_set_metadata_respond().
 *
 * \default By default, only the \c STREAMINFO block is returned via the
 *          metadata callback.
 * \param  decoder  A decoder instance to set.
 * \param  type     See above.
 * \assert
 *    \code decoder != NULL \endcode
 *    \a type is valid
 * \retval FLAC__bool
 *    \c false if the decoder is already initialized, else \c true.
 */
FLAC__bool FLAC__file_decoder_set_metadata_respond(FLAC__FileDecoder *decoder, FLAC__MetadataType type);

/** This is inherited from FLAC__SeekableStreamDecoder; see
 *  FLAC__seekable_stream_decoder_set_metadata_respond_application().
 *
 * \default By default, only the \c STREAMINFO block is returned via the
 *          metadata callback.
 * \param  decoder  A decoder instance to set.
 * \param  id       See above.
 * \assert
 *    \code decoder != NULL \endcode
 *    \code id != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the decoder is already initialized, else \c true.
 */
FLAC__bool FLAC__file_decoder_set_metadata_respond_application(FLAC__FileDecoder *decoder, const FLAC__byte id[4]);

/** This is inherited from FLAC__SeekableStreamDecoder; see
 *  FLAC__seekable_stream_decoder_set_metadata_respond_all().
 *
 * \default By default, only the \c STREAMINFO block is returned via the
 *          metadata callback.
 * \param  decoder  A decoder instance to set.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the decoder is already initialized, else \c true.
 */
FLAC__bool FLAC__file_decoder_set_metadata_respond_all(FLAC__FileDecoder *decoder);

/** This is inherited from FLAC__SeekableStreamDecoder; see
 *  FLAC__seekable_stream_decoder_set_metadata_ignore().
 *
 * \default By default, only the \c STREAMINFO block is returned via the
 *          metadata callback.
 * \param  decoder  A decoder instance to set.
 * \param  type     See above.
 * \assert
 *    \code decoder != NULL \endcode
 *    \a type is valid
 * \retval FLAC__bool
 *    \c false if the decoder is already initialized, else \c true.
 */
FLAC__bool FLAC__file_decoder_set_metadata_ignore(FLAC__FileDecoder *decoder, FLAC__MetadataType type);

/** This is inherited from FLAC__SeekableStreamDecoder; see
 *  FLAC__seekable_stream_decoder_set_metadata_ignore_application().
 *
 * \default By default, only the \c STREAMINFO block is returned via the
 *          metadata callback.
 * \param  decoder  A decoder instance to set.
 * \param  id       See above.
 * \assert
 *    \code decoder != NULL \endcode
 *    \code id != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the decoder is already initialized, else \c true.
 */
FLAC__bool FLAC__file_decoder_set_metadata_ignore_application(FLAC__FileDecoder *decoder, const FLAC__byte id[4]);

/** This is inherited from FLAC__SeekableStreamDecoder; see
 *  FLAC__seekable_stream_decoder_set_metadata_ignore_all().
 *
 * \default By default, only the \c STREAMINFO block is returned via the
 *          metadata callback.
 * \param  decoder  A decoder instance to set.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the decoder is already initialized, else \c true.
 */
FLAC__bool FLAC__file_decoder_set_metadata_ignore_all(FLAC__FileDecoder *decoder);

/** Get the current decoder state.
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__FileDecoderState
 *    The current decoder state.
 */
FLAC__FileDecoderState FLAC__file_decoder_get_state(const FLAC__FileDecoder *decoder);

/** Get the state of the underlying seekable stream decoder.
 *  Useful when the file decoder state is
 *  \c FLAC__FILE_DECODER_SEEKABLE_STREAM_DECODER_ERROR.
 *
 * \param  decoder  An decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__SeekableStreamDecoderState
 *    The seekable stream decoder state.
 */
FLAC__SeekableStreamDecoderState FLAC__file_decoder_get_seekable_stream_decoder_state(const FLAC__FileDecoder *decoder);

/** Get the state of the underlying stream decoder.
 *  Useful when the file decoder state is
 *  \c FLAC__FILE_DECODER_SEEKABLE_STREAM_DECODER_ERROR and the seekable stream
 *  decoder state is \c FLAC__SEEKABLE_STREAM_DECODER_STREAM_DECODER_ERROR.
 *
 * \param  decoder  An decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__StreamDecoderState
 *    The seekable stream decoder state.
 */
FLAC__StreamDecoderState FLAC__file_decoder_get_stream_decoder_state(const FLAC__FileDecoder *decoder);

/** Get the "MD5 signature checking" flag.
 *  This is inherited from FLAC__SeekableStreamDecoder; see
 *  FLAC__seekable_stream_decoder_get_md5_checking().
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    See above.
 */
FLAC__bool FLAC__file_decoder_get_md5_checking(const FLAC__FileDecoder *decoder);

/** This is inherited from FLAC__SeekableStreamDecoder; see
 *  FLAC__seekable_stream_decoder_get_channels().
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval unsigned
 *    See above.
 */
unsigned FLAC__file_decoder_get_channels(const FLAC__FileDecoder *decoder);

/** This is inherited from FLAC__SeekableStreamDecoder; see
 *  FLAC__seekable_stream_decoder_get_channel_assignment().
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__ChannelAssignment
 *    See above.
 */
FLAC__ChannelAssignment FLAC__file_decoder_get_channel_assignment(const FLAC__FileDecoder *decoder);

/** This is inherited from FLAC__SeekableStreamDecoder; see
 *  FLAC__seekable_stream_decoder_get_bits_per_sample().
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval unsigned
 *    See above.
 */
unsigned FLAC__file_decoder_get_bits_per_sample(const FLAC__FileDecoder *decoder);

/** This is inherited from FLAC__SeekableStreamDecoder; see
 *  FLAC__seekable_stream_decoder_get_sample_rate().
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval unsigned
 *    See above.
 */
unsigned FLAC__file_decoder_get_sample_rate(const FLAC__FileDecoder *decoder);

/** This is inherited from FLAC__SeekableStreamDecoder; see
 *  FLAC__seekable_stream_decoder_get_blocksize().
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval unsigned
 *    See above.
 */
unsigned FLAC__file_decoder_get_blocksize(const FLAC__FileDecoder *decoder);

/** Initialize the decoder instance.
 *  Should be called after FLAC__file_decoder_new() and
 *  FLAC__file_decoder_set_*() but before any of the
 *  FLAC__file_decoder_process_*() functions.  Will set and return
 *  the decoder state, which will be FLAC__FILE_DECODER_OK if
 *  initialization succeeded.
 *
 * \param  decoder  An uninitialized decoder instance.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__FileDecoderState
 *    \c FLAC__FILE_DECODER_OK if initialization was successful; see
 *    FLAC__FileDecoderState for the meanings of other return values.
 */
FLAC__FileDecoderState FLAC__file_decoder_init(FLAC__FileDecoder *decoder);

/** Finish the decoding process.
 *  Flushes the decoding buffer, releases resources, resets the decoder
 *  settings to their defaults, and returns the decoder state to
 *  FLAC__FILE_DECODER_UNINITIALIZED.
 *
 *  In the event of a prematurely-terminated decode, it is not strictly
 *  necessary to call this immediately before FLAC__file_decoder_delete()
 *  but it is good practice to match every FLAC__file_decoder_init() with
 *  a FLAC__file_decoder_finish().
 *
 * \param  decoder  An uninitialized decoder instance.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if MD5 checking is on AND a STREAMINFO block was available
 *    AND the MD5 signature in the STREAMINFO block was non-zero AND the
 *    signature does not match the one computed by the decoder; else
 *    \c true.
 */
FLAC__bool FLAC__file_decoder_finish(FLAC__FileDecoder *decoder);

/** This is inherited from FLAC__SeekableStreamDecoder; see
 *  FLAC__seekable_stream_decoder_process_single().
 *
 * \param  decoder  A decoder instance.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    See above.
 */
FLAC__bool FLAC__file_decoder_process_single(FLAC__FileDecoder *decoder);

/** This is inherited from FLAC__SeekableStreamDecoder; see
 *  FLAC__seekable_stream_decoder_process_until_end_of_metadata().
 *
 * \param  decoder  A decoder instance.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    See above.
 */
FLAC__bool FLAC__file_decoder_process_until_end_of_metadata(FLAC__FileDecoder *decoder);

/** This is inherited from FLAC__SeekableStreamDecoder; see
 *  FLAC__seekable_stream_decoder_process_until_end_of_stream().
 *
 * \param  decoder  A decoder instance.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    See above.
 */
FLAC__bool FLAC__file_decoder_process_until_end_of_file(FLAC__FileDecoder *decoder);

/** This is inherited from FLAC__SeekableStreamDecoder; see
 *  FLAC__seekable_stream_decoder_process_remaining_frames().
 *
 * \param  decoder  A decoder instance.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    See above.
 */
FLAC__bool FLAC__file_decoder_process_remaining_frames(FLAC__FileDecoder *decoder);

/** Flush the input and seek to an absolute sample.
 *  This is inherited from FLAC__SeekableStreamDecoder; see
 *  FLAC__seekable_stream_decoder_seek_absolute().
 *
 * \param  decoder  A decoder instance.
 * \param  sample   The target sample number to seek to.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    \c true if successful, else \c false.
 */
FLAC__bool FLAC__file_decoder_seek_absolute(FLAC__FileDecoder *decoder, FLAC__uint64 sample);

/* \} */

#ifdef __cplusplus
}
#endif

#endif
