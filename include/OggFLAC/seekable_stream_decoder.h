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

#ifndef OggFLAC__SEEKABLE_STREAM_DECODER_H
#define OggFLAC__SEEKABLE_STREAM_DECODER_H

#include "FLAC/seekable_stream_decoder.h"

#ifdef __cplusplus
extern "C" {
#endif


/** \file include/OggFLAC/seekable_stream_decoder.h
 *
 *  \brief
 *  This module contains the functions which implement the seekable stream
 *  decoder.
 *
 *  See the detailed documentation in the
 *  \link oggflac_seekable_stream_decoder seekable stream decoder \endlink module.
 */

/** \defgroup oggflac_seekable_stream_decoder OggFLAC/seekable_stream_decoder.h: seekable stream decoder interface
 *  \ingroup oggflac_decoder
 *
 *  \brief
 *  This module contains the functions which implement the seekable stream
 *  decoder.
 *
 * The basic usage of this decoder is as follows:
 * - The program creates an instance of a decoder using
 *   OggFLAC__seekable_stream_decoder_new().
 * - The program overrides the default settings and sets callbacks for
 *   reading, writing, seeking, error reporting, and metadata reporting
 *   using OggFLAC__seekable_stream_decoder_set_*() functions.
 * - The program initializes the instance to validate the settings and
 *   prepare for decoding using OggFLAC__seekable_stream_decoder_init().
 * - The program calls the OggFLAC__seekable_stream_decoder_process_*()
 *   functions to decode data, which subsequently calls the callbacks.
 * - The program finishes the decoding with
 *   OggFLAC__seekable_stream_decoder_finish(), which flushes the input and
 *   output and resets the decoder to the uninitialized state.
 * - The instance may be used again or deleted with
 *   OggFLAC__seekable_stream_decoder_delete().
 *
 * The seekable stream decoder is a wrapper around the
 * \link oggflac_stream_decoder stream decoder \endlink which also provides
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
 * OggFLAC__seekable_stream_decoder_seek_absolute() method.  At any point after
 * the seekable stream decoder has been initialized, the user can call this
 * function to seek to an exact sample within the stream.  Subsequently, the
 * first time the write callback is called it will be passed a (possibly
 * partial) block starting at that sample.
 *
 * The seekable stream decoder also provides MD5 signature checking.  If
 * this is turned on before initialization,
 * OggFLAC__seekable_stream_decoder_finish() will report when the decoded MD5
 * signature does not match the one stored in the STREAMINFO block.  MD5
 * checking is automatically turned off (until the next
 * OggFLAC__seekable_stream_decoder_reset()) if there is no signature in the
 * STREAMINFO block or when a seek is attempted.
 *
 * Make sure to read the detailed description of the
 * \link oggflac_stream_decoder stream decoder module \endlink since the
 * seekable stream decoder inherits much of its behavior.
 *
 * \note
 * The "set" functions may only be called when the decoder is in the
 * state OggFLAC__SEEKABLE_STREAM_DECODER_UNINITIALIZED, i.e. after
 * OggFLAC__seekable_stream_decoder_new() or
 * OggFLAC__seekable_stream_decoder_finish(), but before
 * OggFLAC__seekable_stream_decoder_init().  If this is the case they will
 * return \c true, otherwise \c false.
 *
 * \note
 * OggFLAC__stream_decoder_finish() resets all settings to the constructor
 * defaults, including the callbacks.
 *
 * \{
 */


/** State values for a OggFLAC__SeekableStreamDecoder
 *
 *  The decoder's state can be obtained by calling OggFLAC__seekable_stream_decoder_get_state().
 */
typedef enum {

    OggFLAC__SEEKABLE_STREAM_DECODER_OK = 0,
	/**< The decoder is in the normal OK state. */

	OggFLAC__SEEKABLE_STREAM_DECODER_SEEKING,
	/**< The decoder is in the process of seeking. */

	OggFLAC__SEEKABLE_STREAM_DECODER_END_OF_STREAM,
	/**< The decoder has reached the end of the stream. */

	OggFLAC__SEEKABLE_STREAM_DECODER_MEMORY_ALLOCATION_ERROR,
	/**< An error occurred allocating memory. */

	OggFLAC__SEEKABLE_STREAM_DECODER_STREAM_DECODER_ERROR,
	/**< An error occurred in the underlying stream decoder. */

	OggFLAC__SEEKABLE_STREAM_DECODER_READ_ERROR,
	/**< The read callback returned an error. */

	OggFLAC__SEEKABLE_STREAM_DECODER_SEEK_ERROR,
	/**< An error occurred while seeking or the seek or tell
	 * callback returned an error.
	 */

    OggFLAC__SEEKABLE_STREAM_DECODER_ALREADY_INITIALIZED,
	/**< OggFLAC__seekable_stream_decoder_init() was called when the
	 * decoder was already initialized, usually because
	 * OggFLAC__seekable_stream_decoder_finish() was not called.
	 */

    OggFLAC__SEEKABLE_STREAM_DECODER_INVALID_CALLBACK,
	/**< OggFLAC__seekable_stream_decoder_init() was called without all
	 * callbacks being set.
	 */

    OggFLAC__SEEKABLE_STREAM_DECODER_UNINITIALIZED
	/**< The decoder is in the uninitialized state. */

} OggFLAC__SeekableStreamDecoderState;

/** Maps a OggFLAC__SeekableStreamDecoderState to a C string.
 *
 *  Using a OggFLAC__SeekableStreamDecoderState as the index to this array
 *  will give the string equivalent.  The contents should not be modified.
 */
extern const char * const OggFLAC__SeekableStreamDecoderStateString[];


/** Return values for the OggFLAC__SeekableStreamDecoder read callback.
 */
typedef enum {

	OggFLAC__SEEKABLE_STREAM_DECODER_READ_STATUS_OK,
	/**< The read was OK and decoding can continue. */

	OggFLAC__SEEKABLE_STREAM_DECODER_READ_STATUS_ERROR
	/**< An unrecoverable error occurred.  The decoder will return from the process call. */

} OggFLAC__SeekableStreamDecoderReadStatus;

/** Maps a OggFLAC__SeekableStreamDecoderReadStatus to a C string.
 *
 *  Using a OggFLAC__SeekableStreamDecoderReadStatus as the index to this array
 *  will give the string equivalent.  The contents should not be modified.
 */
extern const char * const OggFLAC__SeekableStreamDecoderReadStatusString[];


/** Return values for the OggFLAC__SeekableStreamDecoder seek callback.
 */
typedef enum {

	OggFLAC__SEEKABLE_STREAM_DECODER_SEEK_STATUS_OK,
	/**< The seek was OK and decoding can continue. */

	OggFLAC__SEEKABLE_STREAM_DECODER_SEEK_STATUS_ERROR
	/**< An unrecoverable error occurred.  The decoder will return from the process call. */

} OggFLAC__SeekableStreamDecoderSeekStatus;

/** Maps a OggFLAC__SeekableStreamDecoderSeekStatus to a C string.
 *
 *  Using a OggFLAC__SeekableStreamDecoderSeekStatus as the index to this array
 *  will give the string equivalent.  The contents should not be modified.
 */
extern const char * const OggFLAC__SeekableStreamDecoderSeekStatusString[];


/** Return values for the OggFLAC__SeekableStreamDecoder tell callback.
 */
typedef enum {

	OggFLAC__SEEKABLE_STREAM_DECODER_TELL_STATUS_OK,
	/**< The tell was OK and decoding can continue. */

	OggFLAC__SEEKABLE_STREAM_DECODER_TELL_STATUS_ERROR
	/**< An unrecoverable error occurred.  The decoder will return from the process call. */

} OggFLAC__SeekableStreamDecoderTellStatus;

/** Maps a OggFLAC__SeekableStreamDecoderTellStatus to a C string.
 *
 *  Using a OggFLAC__SeekableStreamDecoderTellStatus as the index to this array
 *  will give the string equivalent.  The contents should not be modified.
 */
extern const char * const OggFLAC__SeekableStreamDecoderTellStatusString[];


/** Return values for the OggFLAC__SeekableStreamDecoder length callback.
 */
typedef enum {

	OggFLAC__SEEKABLE_STREAM_DECODER_LENGTH_STATUS_OK,
	/**< The length call was OK and decoding can continue. */

	OggFLAC__SEEKABLE_STREAM_DECODER_LENGTH_STATUS_ERROR
	/**< An unrecoverable error occurred.  The decoder will return from the process call. */

} OggFLAC__SeekableStreamDecoderLengthStatus;

/** Maps a OggFLAC__SeekableStreamDecoderLengthStatus to a C string.
 *
 *  Using a OggFLAC__SeekableStreamDecoderLengthStatus as the index to this array
 *  will give the string equivalent.  The contents should not be modified.
 */
extern const char * const OggFLAC__SeekableStreamDecoderLengthStatusString[];


/***********************************************************************
 *
 * class OggFLAC__SeekableStreamDecoder : public OggFLAC__StreamDecoder
 *
 ***********************************************************************/

struct OggFLAC__SeekableStreamDecoderProtected;
struct OggFLAC__SeekableStreamDecoderPrivate;
/** The opaque structure definition for the seekable stream decoder type.
 *  See the
 *  \link oggflac_seekable_stream_decoder seekable stream decoder module \endlink
 *  for a detailed description.
 */
typedef struct {
	struct OggFLAC__SeekableStreamDecoderProtected *protected_; /* avoid the C++ keyword 'protected' */
	struct OggFLAC__SeekableStreamDecoderPrivate *private_; /* avoid the C++ keyword 'private' */
} OggFLAC__SeekableStreamDecoder;

/*@@@ document */
typedef OggFLAC__SeekableStreamDecoderReadStatus (*OggFLAC__SeekableStreamDecoderReadCallback)(const OggFLAC__SeekableStreamDecoder *decoder, FLAC__byte buffer[], unsigned *bytes, void *client_data);
typedef OggFLAC__SeekableStreamDecoderSeekStatus (*OggFLAC__SeekableStreamDecoderSeekCallback)(const OggFLAC__SeekableStreamDecoder *decoder, FLAC__uint64 absolute_byte_offset, void *client_data);
typedef OggFLAC__SeekableStreamDecoderTellStatus (*OggFLAC__SeekableStreamDecoderTellCallback)(const OggFLAC__SeekableStreamDecoder *decoder, FLAC__uint64 *absolute_byte_offset, void *client_data);
typedef OggFLAC__SeekableStreamDecoderLengthStatus (*OggFLAC__SeekableStreamDecoderLengthCallback)(const OggFLAC__SeekableStreamDecoder *decoder, FLAC__uint64 *stream_length, void *client_data);
typedef FLAC__bool (*OggFLAC__SeekableStreamDecoderEofCallback)(const OggFLAC__SeekableStreamDecoder *decoder, void *client_data);
typedef OggFLAC__StreamDecoderWriteStatus (*OggFLAC__SeekableStreamDecoderWriteCallback)(const OggFLAC__SeekableStreamDecoder *decoder, const OggFLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data);
typedef void (*OggFLAC__SeekableStreamDecoderMetadataCallback)(const OggFLAC__SeekableStreamDecoder *decoder, const OggFLAC__StreamMetadata *metadata, void *client_data);
typedef void (*OggFLAC__SeekableStreamDecoderErrorCallback)(const OggFLAC__SeekableStreamDecoder *decoder, OggFLAC__StreamDecoderErrorStatus status, void *client_data);


/***********************************************************************
 *
 * Class constructor/destructor
 *
 ***********************************************************************/

/** Create a new seekable stream decoder instance.  The instance is created
 *  with default settings; see the individual
 *  OggFLAC__seekable_stream_decoder_set_*() functions for each setting's
 *  default.
 *
 * \retval OggFLAC__SeekableStreamDecoder*
 *    \c NULL if there was an error allocating memory, else the new instance.
 */
OggFLAC__SeekableStreamDecoder *OggFLAC__seekable_stream_decoder_new();

/** Free a decoder instance.  Deletes the object pointed to by \a decoder.
 *
 * \param decoder  A pointer to an existing decoder.
 * \assert
 *    \code decoder != NULL \endcode
 */
void OggFLAC__seekable_stream_decoder_delete(OggFLAC__SeekableStreamDecoder *decoder);


/***********************************************************************
 *
 * Public class method prototypes
 *
 ***********************************************************************/

/** Set the "MD5 signature checking" flag.  If \c true, the decoder will
 *  compute the MD5 signature of the unencoded audio data while decoding
 *  and compare it to the signature from the STREAMINFO block, if it
 *  exists, during OggFLAC__seekable_stream_decoder_finish().
 *
 *  MD5 signature checking will be turned off (until the next
 *  OggFLAC__seekable_stream_decoder_reset()) if there is no signature in
 *  the STREAMINFO block or when a seek is attempted.
 *
 * \default \c false
 * \param  decoder  A decoder instance to set.
 * \param  value    Flag value (see above).
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the decoder is already initialized, else \c true.
 */
FLAC__bool OggFLAC__seekable_stream_decoder_set_md5_checking(OggFLAC__SeekableStreamDecoder *decoder, FLAC__bool value);

/** Set the read callback.
 *  This is inherited from OggFLAC__StreamDecoder; see
 *  OggFLAC__stream_decoder_set_read_callback().
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
FLAC__bool OggFLAC__seekable_stream_decoder_set_read_callback(OggFLAC__SeekableStreamDecoder *decoder, OggFLAC__SeekableStreamDecoderReadCallback value);

/** Set the seek callback.
 *  The supplied function will be called when the decoder needs to seek
 *  the input stream.  The decoder will pass the absolute byte offset
 *  to seek to, 0 meaning the beginning of the stream.
 *
 * \note
 * The callback is mandatory and must be set before initialization.
 *
 * \default \c NULL
 * \param  decoder  An decoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code decoder != NULL \endcode
 *    \code value != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the decoder is already initialized, else \c true.
 */
FLAC__bool OggFLAC__seekable_stream_decoder_set_seek_callback(OggFLAC__SeekableStreamDecoder *decoder, OggFLAC__SeekableStreamDecoderSeekCallback value);

/** Set the tell callback.
 *  The supplied function will be called when the decoder wants to know
 *  the current position of the stream.  The callback should return the
 *  byte offset from the beginning of the stream.
 *
 * \note
 * The callback is mandatory and must be set before initialization.
 *
 * \default \c NULL
 * \param  decoder  An decoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code decoder != NULL \endcode
 *    \code value != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the decoder is already initialized, else \c true.
 */
FLAC__bool OggFLAC__seekable_stream_decoder_set_tell_callback(OggFLAC__SeekableStreamDecoder *decoder, OggFLAC__SeekableStreamDecoderTellCallback value);

/** Set the length callback.
 *  The supplied function will be called when the decoder wants to know
 *  the total length of the stream in bytes.
 *
 * \note
 * The callback is mandatory and must be set before initialization.
 *
 * \default \c NULL
 * \param  decoder  An decoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code decoder != NULL \endcode
 *    \code value != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the decoder is already initialized, else \c true.
 */
FLAC__bool OggFLAC__seekable_stream_decoder_set_length_callback(OggFLAC__SeekableStreamDecoder *decoder, OggFLAC__SeekableStreamDecoderLengthCallback value);

/** Set the eof callback.
 *  The supplied function will be called when the decoder needs to know
 *  if the end of the stream has been reached.
 *
 * \note
 * The callback is mandatory and must be set before initialization.
 *
 * \default \c NULL
 * \param  decoder  An decoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code decoder != NULL \endcode
 *    \code value != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the decoder is already initialized, else \c true.
 */
FLAC__bool OggFLAC__seekable_stream_decoder_set_eof_callback(OggFLAC__SeekableStreamDecoder *decoder, OggFLAC__SeekableStreamDecoderEofCallback value);

/** Set the write callback.
 *  This is inherited from OggFLAC__StreamDecoder; see
 *  OggFLAC__stream_decoder_set_write_callback().
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
FLAC__bool OggFLAC__seekable_stream_decoder_set_write_callback(OggFLAC__SeekableStreamDecoder *decoder, OggFLAC__SeekableStreamDecoderWriteCallback value);

/** Set the metadata callback.
 *  This is inherited from OggFLAC__StreamDecoder; see
 *  OggFLAC__stream_decoder_set_metadata_callback().
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
FLAC__bool OggFLAC__seekable_stream_decoder_set_metadata_callback(OggFLAC__SeekableStreamDecoder *decoder, OggFLAC__SeekableStreamDecoderMetadataCallback value);

/** Set the error callback.
 *  This is inherited from OggFLAC__StreamDecoder; see
 *  OggFLAC__stream_decoder_set_error_callback().
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
FLAC__bool OggFLAC__seekable_stream_decoder_set_error_callback(OggFLAC__SeekableStreamDecoder *decoder, OggFLAC__SeekableStreamDecoderErrorCallback value);

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
FLAC__bool OggFLAC__seekable_stream_decoder_set_client_data(OggFLAC__SeekableStreamDecoder *decoder, void *value);

/** This is inherited from OggFLAC__StreamDecoder; see
 *  OggFLAC__stream_decoder_set_metadata_respond().
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
FLAC__bool OggFLAC__seekable_stream_decoder_set_metadata_respond(OggFLAC__SeekableStreamDecoder *decoder, OggFLAC__MetadataType type);

/** This is inherited from OggFLAC__StreamDecoder; see
 *  OggFLAC__stream_decoder_set_metadata_respond_application().
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
FLAC__bool OggFLAC__seekable_stream_decoder_set_metadata_respond_application(OggFLAC__SeekableStreamDecoder *decoder, const FLAC__byte id[4]);

/** This is inherited from OggFLAC__StreamDecoder; see
 *  OggFLAC__stream_decoder_set_metadata_respond_all().
 *
 * \default By default, only the \c STREAMINFO block is returned via the
 *          metadata callback.
 * \param  decoder  A decoder instance to set.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the decoder is already initialized, else \c true.
 */
FLAC__bool OggFLAC__seekable_stream_decoder_set_metadata_respond_all(OggFLAC__SeekableStreamDecoder *decoder);

/** This is inherited from OggFLAC__StreamDecoder; see
 *  OggFLAC__stream_decoder_set_metadata_ignore().
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
FLAC__bool OggFLAC__seekable_stream_decoder_set_metadata_ignore(OggFLAC__SeekableStreamDecoder *decoder, OggFLAC__MetadataType type);

/** This is inherited from OggFLAC__StreamDecoder; see
 *  OggFLAC__stream_decoder_set_metadata_ignore_application().
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
FLAC__bool OggFLAC__seekable_stream_decoder_set_metadata_ignore_application(OggFLAC__SeekableStreamDecoder *decoder, const FLAC__byte id[4]);

/** This is inherited from OggFLAC__StreamDecoder; see
 *  OggFLAC__stream_decoder_set_metadata_ignore_all().
 *
 * \default By default, only the \c STREAMINFO block is returned via the
 *          metadata callback.
 * \param  decoder  A decoder instance to set.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the decoder is already initialized, else \c true.
 */
FLAC__bool OggFLAC__seekable_stream_decoder_set_metadata_ignore_all(OggFLAC__SeekableStreamDecoder *decoder);

/** Get the current decoder state.
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval OggFLAC__SeekableStreamDecoderState
 *    The current decoder state.
 */
OggFLAC__SeekableStreamDecoderState OggFLAC__seekable_stream_decoder_get_state(const OggFLAC__SeekableStreamDecoder *decoder);

/** Get the state of the underlying stream decoder.
 *  Useful when the seekable stream decoder state is
 *  \c OggFLAC__SEEKABLE_STREAM_DECODER_STREAM_DECODER_ERROR.
 *
 * \param  decoder  An decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval OggFLAC__StreamDecoderState
 *    The stream decoder state.
 */
OggFLAC__StreamDecoderState OggFLAC__seekable_stream_decoder_get_stream_decoder_state(const OggFLAC__SeekableStreamDecoder *decoder);

/** Get the "MD5 signature checking" flag.
 *  This is the value of the setting, not whether or not the decoder is
 *  currently checking the MD5 (remember, it can be turned off automatically
 *  by a seek).  When the decoder is reset the flag will be restored to the
 *  value returned by this function.
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    See above.
 */
FLAC__bool OggFLAC__seekable_stream_decoder_get_md5_checking(const OggFLAC__SeekableStreamDecoder *decoder);

/** This is inherited from OggFLAC__StreamDecoder; see
 *  OggFLAC__stream_decoder_get_channels().
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval unsigned
 *    See above.
 */
unsigned OggFLAC__seekable_stream_decoder_get_channels(const OggFLAC__SeekableStreamDecoder *decoder);

/** This is inherited from OggFLAC__StreamDecoder; see
 *  OggFLAC__stream_decoder_get_channel_assignment().
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval OggFLAC__ChannelAssignment
 *    See above.
 */
OggFLAC__ChannelAssignment OggFLAC__seekable_stream_decoder_get_channel_assignment(const OggFLAC__SeekableStreamDecoder *decoder);

/** This is inherited from OggFLAC__StreamDecoder; see
 *  OggFLAC__stream_decoder_get_bits_per_sample().
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval unsigned
 *    See above.
 */
unsigned OggFLAC__seekable_stream_decoder_get_bits_per_sample(const OggFLAC__SeekableStreamDecoder *decoder);

/** This is inherited from OggFLAC__StreamDecoder; see
 *  OggFLAC__stream_decoder_get_sample_rate().
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval unsigned
 *    See above.
 */
unsigned OggFLAC__seekable_stream_decoder_get_sample_rate(const OggFLAC__SeekableStreamDecoder *decoder);

/** This is inherited from OggFLAC__StreamDecoder; see
 *  OggFLAC__stream_decoder_get_blocksize().
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval unsigned
 *    See above.
 */
unsigned OggFLAC__seekable_stream_decoder_get_blocksize(const OggFLAC__SeekableStreamDecoder *decoder);

/** Initialize the decoder instance.
 *  Should be called after OggFLAC__seekable_stream_decoder_new() and
 *  OggFLAC__seekable_stream_decoder_set_*() but before any of the
 *  OggFLAC__seekable_stream_decoder_process_*() functions.  Will set and return
 *  the decoder state, which will be OggFLAC__SEEKABLE_STREAM_DECODER_OK
 *  if initialization succeeded.
 *
 * \param  decoder  An uninitialized decoder instance.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval OggFLAC__SeekableStreamDecoderState
 *    \c OggFLAC__SEEKABLE_STREAM_DECODER_OK if initialization was
 *    successful; see OggFLAC__SeekableStreamDecoderState for the meanings
 *    of other return values.
 */
OggFLAC__SeekableStreamDecoderState OggFLAC__seekable_stream_decoder_init(OggFLAC__SeekableStreamDecoder *decoder);

/** Finish the decoding process.
 *  Flushes the decoding buffer, releases resources, resets the decoder
 *  settings to their defaults, and returns the decoder state to
 *  OggFLAC__SEEKABLE_STREAM_DECODER_UNINITIALIZED.
 *
 *  In the event of a prematurely-terminated decode, it is not strictly
 *  necessary to call this immediately before
 *  OggFLAC__seekable_stream_decoder_delete() but it is good practice to match
 *  every OggFLAC__seekable_stream_decoder_init() with a
 *  OggFLAC__seekable_stream_decoder_finish().
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
FLAC__bool OggFLAC__seekable_stream_decoder_finish(OggFLAC__SeekableStreamDecoder *decoder);

/** Flush the stream input.
 *  The decoder's input buffer will be cleared and the state set to
 *  \c OggFLAC__SEEKABLE_STREAM_DECODER_OK.  This will also turn off MD5
 *  checking.
 *
 * \param  decoder  A decoder instance.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    \c true if successful, else \c false if a memory allocation
 *    or stream decoder error occurs.
 */
FLAC__bool OggFLAC__seekable_stream_decoder_flush(OggFLAC__SeekableStreamDecoder *decoder);

/** Reset the decoding process.
 *  The decoder's input buffer will be cleared and the state set to
 *  \c OggFLAC__SEEKABLE_STREAM_DECODER_OK.  This is similar to
 *  OggFLAC__seekable_stream_decoder_finish() except that the settings are
 *  preserved; there is no need to call OggFLAC__seekable_stream_decoder_init()
 *  before decoding again.  MD5 checking will be restored to its original
 *  setting.
 *
 * \param  decoder  A decoder instance.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    \c true if successful, else \c false if a memory allocation
 *    or stream decoder error occurs.
 */
FLAC__bool OggFLAC__seekable_stream_decoder_reset(OggFLAC__SeekableStreamDecoder *decoder);

/** This is inherited from OggFLAC__StreamDecoder; see
 *  OggFLAC__stream_decoder_process_single().
 *
 * \param  decoder  A decoder instance.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    See above.
 */
FLAC__bool OggFLAC__seekable_stream_decoder_process_single(OggFLAC__SeekableStreamDecoder *decoder);

/** This is inherited from OggFLAC__StreamDecoder; see
 *  OggFLAC__stream_decoder_process_until_end_of_metadata().
 *
 * \param  decoder  A decoder instance.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    See above.
 */
FLAC__bool OggFLAC__seekable_stream_decoder_process_until_end_of_metadata(OggFLAC__SeekableStreamDecoder *decoder);

/** This is inherited from OggFLAC__StreamDecoder; see
 *  OggFLAC__stream_decoder_process_until_end_of_stream().
 *
 * \param  decoder  A decoder instance.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    See above.
 */
FLAC__bool OggFLAC__seekable_stream_decoder_process_until_end_of_stream(OggFLAC__SeekableStreamDecoder *decoder);

/** Flush the input and seek to an absolute sample.
 *  Decoding will resume at the given sample.  Note that because of
 *  this, the next write callback may contain a partial block.
 *
 * \param  decoder  A decoder instance.
 * \param  sample   The target sample number to seek to.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    \c true if successful, else \c false.
 */
FLAC__bool OggFLAC__seekable_stream_decoder_seek_absolute(OggFLAC__SeekableStreamDecoder *decoder, FLAC__uint64 sample);

/* \} */

#ifdef __cplusplus
}
#endif

#endif
