/* libOggFLAC - Free Lossless Audio Codec + Ogg library
 * Copyright (C) 2002,2003,2004,2005  Josh Coalson
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

#ifndef OggFLAC__SEEKABLE_STREAM_DECODER_H
#define OggFLAC__SEEKABLE_STREAM_DECODER_H

#include "export.h"
#include "stream_decoder.h"

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
 * The interface here is nearly identical to FLAC's seekable stream decoder,
 * including the callbacks, with the addition of
 * OggFLAC__seekable_stream_decoder_set_serial_number().  See the
 * \link flac_seekable_stream_decoder FLAC seekable stream decoder module \endlink
 * for full documentation.
 *
 * \{
 */


/** State values for an OggFLAC__SeekableStreamDecoder
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
	/**< Memory allocation failed. */

	OggFLAC__SEEKABLE_STREAM_DECODER_STREAM_DECODER_ERROR,
	/**< An error occurred in the underlying stream decoder;
	 * check OggFLAC__seekable_stream_decoder_get_stream_decoder_state().
	 */

	OggFLAC__SEEKABLE_STREAM_DECODER_READ_ERROR,
	/**< The read callback returned an error. */

	OggFLAC__SEEKABLE_STREAM_DECODER_SEEK_ERROR,
	/**< An error occurred while seeking or the seek or tell
	 * callback returned an error.
	 */

	OggFLAC__SEEKABLE_STREAM_DECODER_ALREADY_INITIALIZED,
	/**< OggFLAC__seekable_stream_decoder_init() was called when the decoder was
	 * already initialized, usually because
	 * OggFLAC__seekable_stream_decoder_finish() was not called.
	 */

	OggFLAC__SEEKABLE_STREAM_DECODER_INVALID_CALLBACK,
	/**< The decoder was initialized before setting all the required callbacks. */

	OggFLAC__SEEKABLE_STREAM_DECODER_UNINITIALIZED
	/**< The decoder is in the uninitialized state. */

} OggFLAC__SeekableStreamDecoderState;

/** Maps an OggFLAC__SeekableStreamDecoderState to a C string.
 *
 *  Using an OggFLAC__SeekableStreamDecoderState as the index to this array
 *  will give the string equivalent.  The contents should not be modified.
 */
extern OggFLAC_API const char * const OggFLAC__SeekableStreamDecoderStateString[];


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
extern OggFLAC_API const char * const OggFLAC__SeekableStreamDecoderReadStatusString[];


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
extern OggFLAC_API const char * const OggFLAC__SeekableStreamDecoderSeekStatusString[];


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
extern OggFLAC_API const char * const OggFLAC__SeekableStreamDecoderTellStatusString[];


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
extern OggFLAC_API const char * const OggFLAC__SeekableStreamDecoderLengthStatusString[];


/***********************************************************************
 *
 * class OggFLAC__SeekableStreamDecoder : public FLAC__StreamDecoder
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

/** Signature for the read callback.
 *  See OggFLAC__seekable_stream_decoder_set_read_callback()
 *  and OggFLAC__StreamDecoderReadCallback for more info.
 *
 * \param  decoder  The decoder instance calling the callback.
 * \param  buffer   A pointer to a location for the callee to store
 *                  data to be decoded.
 * \param  bytes    A pointer to the size of the buffer.
 * \param  client_data  The callee's client data set through
 *                      OggFLAC__seekable_stream_decoder_set_client_data().
 * \retval FLAC__SeekableStreamDecoderReadStatus
 *    The callee's return status.
 */
typedef OggFLAC__SeekableStreamDecoderReadStatus (*OggFLAC__SeekableStreamDecoderReadCallback)(const OggFLAC__SeekableStreamDecoder *decoder, FLAC__byte buffer[], unsigned *bytes, void *client_data);

/** Signature for the seek callback.
 *  See OggFLAC__seekable_stream_decoder_set_seek_callback() for more info.
 *
 * \param  decoder  The decoder instance calling the callback.
 * \param  absolute_byte_offset  The offset from the beginning of the stream
 *                               to seek to.
 * \param  client_data  The callee's client data set through
 *                      OggFLAC__seekable_stream_decoder_set_client_data().
 * \retval FLAC__SeekableStreamDecoderSeekStatus
 *    The callee's return status.
 */
typedef OggFLAC__SeekableStreamDecoderSeekStatus (*OggFLAC__SeekableStreamDecoderSeekCallback)(const OggFLAC__SeekableStreamDecoder *decoder, FLAC__uint64 absolute_byte_offset, void *client_data);

/** Signature for the tell callback.
 *  See OggFLAC__seekable_stream_decoder_set_tell_callback() for more info.
 *
 * \param  decoder  The decoder instance calling the callback.
 * \param  absolute_byte_offset  A pointer to storage for the current offset
 *                               from the beginning of the stream.
 * \param  client_data  The callee's client data set through
 *                      OggFLAC__seekable_stream_decoder_set_client_data().
 * \retval FLAC__SeekableStreamDecoderTellStatus
 *    The callee's return status.
 */
typedef OggFLAC__SeekableStreamDecoderTellStatus (*OggFLAC__SeekableStreamDecoderTellCallback)(const OggFLAC__SeekableStreamDecoder *decoder, FLAC__uint64 *absolute_byte_offset, void *client_data);

/** Signature for the length callback.
 *  See OggFLAC__seekable_stream_decoder_set_length_callback() for more info.
 *
 * \param  decoder  The decoder instance calling the callback.
 * \param  stream_length  A pointer to storage for the length of the stream
 *                        in bytes.
 * \param  client_data  The callee's client data set through
 *                      OggFLAC__seekable_stream_decoder_set_client_data().
 * \retval FLAC__SeekableStreamDecoderLengthStatus
 *    The callee's return status.
 */
typedef OggFLAC__SeekableStreamDecoderLengthStatus (*OggFLAC__SeekableStreamDecoderLengthCallback)(const OggFLAC__SeekableStreamDecoder *decoder, FLAC__uint64 *stream_length, void *client_data);

/** Signature for the EOF callback.
 *  See OggFLAC__seekable_stream_decoder_set_eof_callback() for more info.
 *
 * \param  decoder  The decoder instance calling the callback.
 * \param  client_data  The callee's client data set through
 *                      OggFLAC__seekable_stream_decoder_set_client_data().
 * \retval FLAC__bool
 *    \c true if the currently at the end of the stream, else \c false.
 */
typedef FLAC__bool (*OggFLAC__SeekableStreamDecoderEofCallback)(const OggFLAC__SeekableStreamDecoder *decoder, void *client_data);

/** Signature for the write callback.
 *  See OggFLAC__seekable_stream_decoder_set_write_callback()
 *  and OggFLAC__StreamDecoderWriteCallback for more info.
 *
 * \param  decoder  The decoder instance calling the callback.
 * \param  frame    The description of the decoded frame.
 * \param  buffer   An array of pointers to decoded channels of data.
 * \param  client_data  The callee's client data set through
 *                      OggFLAC__seekable_stream_decoder_set_client_data().
 * \retval FLAC__StreamDecoderWriteStatus
 *    The callee's return status.
 */
typedef FLAC__StreamDecoderWriteStatus (*OggFLAC__SeekableStreamDecoderWriteCallback)(const OggFLAC__SeekableStreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data);

/** Signature for the metadata callback.
 *  See OggFLAC__seekable_stream_decoder_set_metadata_callback()
 *  and OggFLAC__StreamDecoderMetadataCallback for more info.
 *
 * \param  decoder  The decoder instance calling the callback.
 * \param  metadata The decoded metadata block.
 * \param  client_data  The callee's client data set through
 *                      OggFLAC__seekable_stream_decoder_set_client_data().
 */
typedef void (*OggFLAC__SeekableStreamDecoderMetadataCallback)(const OggFLAC__SeekableStreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data);

/** Signature for the error callback.
 *  See OggFLAC__seekable_stream_decoder_set_error_callback()
 *  and OggFLAC__StreamDecoderErrorCallback for more info.
 *
 * \param  decoder  The decoder instance calling the callback.
 * \param  status   The error encountered by the decoder.
 * \param  client_data  The callee's client data set through
 *                      OggFLAC__seekable_stream_decoder_set_client_data().
 */
typedef void (*OggFLAC__SeekableStreamDecoderErrorCallback)(const OggFLAC__SeekableStreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data);


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
OggFLAC_API OggFLAC__SeekableStreamDecoder *OggFLAC__seekable_stream_decoder_new();

/** Free a decoder instance.  Deletes the object pointed to by \a decoder.
 *
 * \param decoder  A pointer to an existing decoder.
 * \assert
 *    \code decoder != NULL \endcode
 */
OggFLAC_API void OggFLAC__seekable_stream_decoder_delete(OggFLAC__SeekableStreamDecoder *decoder);


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
 * \param  value    Flag value (see above).
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the decoder is already initialized, else \c true.
 */
OggFLAC_API FLAC__bool OggFLAC__seekable_stream_decoder_set_md5_checking(OggFLAC__SeekableStreamDecoder *decoder, FLAC__bool value);

/** Set the read callback.
 *  This is inherited from FLAC__SeekableStreamDecoder; see
 *  FLAC__seekable_stream_decoder_set_read_callback().
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
OggFLAC_API FLAC__bool OggFLAC__seekable_stream_decoder_set_read_callback(OggFLAC__SeekableStreamDecoder *decoder, OggFLAC__SeekableStreamDecoderReadCallback value);

/** Set the seek callback.
 *  This is inherited from FLAC__SeekableStreamDecoder; see
 *  FLAC__seekable_stream_decoder_set_seek_callback().
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
OggFLAC_API FLAC__bool OggFLAC__seekable_stream_decoder_set_seek_callback(OggFLAC__SeekableStreamDecoder *decoder, OggFLAC__SeekableStreamDecoderSeekCallback value);

/** Set the tell callback.
 *  This is inherited from FLAC__SeekableStreamDecoder; see
 *  FLAC__seekable_stream_decoder_set_tell_callback().
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
OggFLAC_API FLAC__bool OggFLAC__seekable_stream_decoder_set_tell_callback(OggFLAC__SeekableStreamDecoder *decoder, OggFLAC__SeekableStreamDecoderTellCallback value);

/** Set the length callback.
 *  This is inherited from FLAC__SeekableStreamDecoder; see
 *  FLAC__seekable_stream_decoder_set_length_callback().
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
OggFLAC_API FLAC__bool OggFLAC__seekable_stream_decoder_set_length_callback(OggFLAC__SeekableStreamDecoder *decoder, OggFLAC__SeekableStreamDecoderLengthCallback value);

/** Set the eof callback.
 *  This is inherited from FLAC__SeekableStreamDecoder; see
 *  FLAC__seekable_stream_decoder_set_eof_callback().
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
OggFLAC_API FLAC__bool OggFLAC__seekable_stream_decoder_set_eof_callback(OggFLAC__SeekableStreamDecoder *decoder, OggFLAC__SeekableStreamDecoderEofCallback value);

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
OggFLAC_API FLAC__bool OggFLAC__seekable_stream_decoder_set_write_callback(OggFLAC__SeekableStreamDecoder *decoder, OggFLAC__SeekableStreamDecoderWriteCallback value);

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
OggFLAC_API FLAC__bool OggFLAC__seekable_stream_decoder_set_metadata_callback(OggFLAC__SeekableStreamDecoder *decoder, OggFLAC__SeekableStreamDecoderMetadataCallback value);

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
OggFLAC_API FLAC__bool OggFLAC__seekable_stream_decoder_set_error_callback(OggFLAC__SeekableStreamDecoder *decoder, OggFLAC__SeekableStreamDecoderErrorCallback value);

/** Set the client data to be passed back to callbacks.
 *  This value will be supplied to callbacks in their \a client_data
 *  argument.
 *
 * \default \c NULL
 * \param  decoder  A decoder instance to set.
 * \param  value    See above.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the decoder is already initialized, else \c true.
 */
OggFLAC_API FLAC__bool OggFLAC__seekable_stream_decoder_set_client_data(OggFLAC__SeekableStreamDecoder *decoder, void *value);

/** Set the serial number for the Ogg stream.
 * The default behavior is to use the serial number of the first Ogg
 * page.  Setting a serial number here will explicitly specify which
 * stream is to be decoded.
 *
 * \default \c use serial number of first page
 * \param  decoder        A decoder instance to set.
 * \param  serial_number  See above.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    \c false if the decoder is already initialized, else \c true.
 */
OggFLAC_API FLAC__bool OggFLAC__seekable_stream_decoder_set_serial_number(OggFLAC__SeekableStreamDecoder *decoder, long serial_number);

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
OggFLAC_API FLAC__bool OggFLAC__seekable_stream_decoder_set_metadata_respond(OggFLAC__SeekableStreamDecoder *decoder, FLAC__MetadataType type);

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
OggFLAC_API FLAC__bool OggFLAC__seekable_stream_decoder_set_metadata_respond_application(OggFLAC__SeekableStreamDecoder *decoder, const FLAC__byte id[4]);

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
OggFLAC_API FLAC__bool OggFLAC__seekable_stream_decoder_set_metadata_respond_all(OggFLAC__SeekableStreamDecoder *decoder);

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
OggFLAC_API FLAC__bool OggFLAC__seekable_stream_decoder_set_metadata_ignore(OggFLAC__SeekableStreamDecoder *decoder, FLAC__MetadataType type);

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
OggFLAC_API FLAC__bool OggFLAC__seekable_stream_decoder_set_metadata_ignore_application(OggFLAC__SeekableStreamDecoder *decoder, const FLAC__byte id[4]);

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
OggFLAC_API FLAC__bool OggFLAC__seekable_stream_decoder_set_metadata_ignore_all(OggFLAC__SeekableStreamDecoder *decoder);

/** Get the current decoder state.
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval OggFLAC__SeekableStreamDecoderState
 *    The current decoder state.
 */
OggFLAC_API OggFLAC__SeekableStreamDecoderState OggFLAC__seekable_stream_decoder_get_state(const OggFLAC__SeekableStreamDecoder *decoder);

/** Get the state of the underlying stream decoder.
 *  Useful when the seekable stream decoder state is
 *  \c OggFLAC__SEEKABLE_STREAM_DECODER_STREAM_DECODER_ERROR.
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval OggFLAC__StreamDecoderState
 *    The stream decoder state.
 */
OggFLAC_API OggFLAC__StreamDecoderState OggFLAC__seekable_stream_decoder_get_stream_decoder_state(const OggFLAC__SeekableStreamDecoder *decoder);

/** Get the state of the underlying stream decoder's FLAC stream decoder.
 *  Useful when the seekable stream decoder state is
 *  \c OggFLAC__SEEKABLE_STREAM_DECODER_STREAM_DECODER_ERROR and the
 *  stream decoder state is \c OggFLAC__STREAM_DECODER_FLAC_STREAM_DECODER_ERROR
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__StreamDecoderState
 *    The FLAC stream decoder state.
 */
OggFLAC_API FLAC__StreamDecoderState OggFLAC__seekable_stream_decoder_get_FLAC_stream_decoder_state(const OggFLAC__SeekableStreamDecoder *decoder);

/** Get the current decoder state as a C string.
 *  This version automatically resolves
 *  \c OggFLAC__SEEKABLE_STREAM_DECODER_STREAM_DECODER_ERROR
 *  by getting the stream decoder's state.
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval const char *
 *    The decoder state as a C string.  Do not modify the contents.
 */
OggFLAC_API const char *OggFLAC__seekable_stream_decoder_get_resolved_state_string(const OggFLAC__SeekableStreamDecoder *decoder);

/** This is inherited from FLAC__SeekableStreamDecoder; see
 *  FLAC__seekable_stream_decoder_get_md5_checking().
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    See above.
 */
OggFLAC_API FLAC__bool OggFLAC__seekable_stream_decoder_get_md5_checking(const OggFLAC__SeekableStreamDecoder *decoder);

/** This is inherited from FLAC__SeekableStreamDecoder; see
 *  FLAC__seekable_stream_decoder_get_channels().
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval unsigned
 *    See above.
 */
OggFLAC_API unsigned OggFLAC__seekable_stream_decoder_get_channels(const OggFLAC__SeekableStreamDecoder *decoder);

/** This is inherited from FLAC__SeekableStreamDecoder; see
 *  FLAC__seekable_stream_decoder_get_channel_assignment().
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval OggFLAC__ChannelAssignment
 *    See above.
 */
OggFLAC_API FLAC__ChannelAssignment OggFLAC__seekable_stream_decoder_get_channel_assignment(const OggFLAC__SeekableStreamDecoder *decoder);

/** This is inherited from FLAC__SeekableStreamDecoder; see
 *  FLAC__seekable_stream_decoder_get_bits_per_sample().
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval unsigned
 *    See above.
 */
OggFLAC_API unsigned OggFLAC__seekable_stream_decoder_get_bits_per_sample(const OggFLAC__SeekableStreamDecoder *decoder);

/** This is inherited from FLAC__SeekableStreamDecoder; see
 *  FLAC__seekable_stream_decoder_get_sample_rate().
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval unsigned
 *    See above.
 */
OggFLAC_API unsigned OggFLAC__seekable_stream_decoder_get_sample_rate(const OggFLAC__SeekableStreamDecoder *decoder);

/** This is inherited from FLAC__SeekableStreamDecoder; see
 *  FLAC__seekable_stream_decoder_get_blocksize().
 *
 * \param  decoder  A decoder instance to query.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval unsigned
 *    See above.
 */
OggFLAC_API unsigned OggFLAC__seekable_stream_decoder_get_blocksize(const OggFLAC__SeekableStreamDecoder *decoder);

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
OggFLAC_API OggFLAC__SeekableStreamDecoderState OggFLAC__seekable_stream_decoder_init(OggFLAC__SeekableStreamDecoder *decoder);

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
OggFLAC_API FLAC__bool OggFLAC__seekable_stream_decoder_finish(OggFLAC__SeekableStreamDecoder *decoder);

/** This is inherited from FLAC__SeekableStreamDecoder; see
 *  FLAC__seekable_stream_decoder_flush().
 *
 * \param  decoder  A decoder instance.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    \c true if successful, else \c false if a memory allocation
 *    or stream decoder error occurs.
 */
OggFLAC_API FLAC__bool OggFLAC__seekable_stream_decoder_flush(OggFLAC__SeekableStreamDecoder *decoder);

/** This is inherited from FLAC__SeekableStreamDecoder; see
 *  FLAC__seekable_stream_decoder_reset().
 *
 * \param  decoder  A decoder instance.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    \c true if successful, else \c false if a memory allocation
 *    or stream decoder error occurs.
 */
OggFLAC_API FLAC__bool OggFLAC__seekable_stream_decoder_reset(OggFLAC__SeekableStreamDecoder *decoder);

/** This is inherited from FLAC__SeekableStreamDecoder; see
 *  FLAC__seekable_stream_decoder_process_single().
 *
 * \param  decoder  A decoder instance.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    See above.
 */
OggFLAC_API FLAC__bool OggFLAC__seekable_stream_decoder_process_single(OggFLAC__SeekableStreamDecoder *decoder);

/** This is inherited from FLAC__SeekableStreamDecoder; see
 *  FLAC__seekable_stream_decoder_process_until_end_of_metadata().
 *
 * \param  decoder  A decoder instance.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    See above.
 */
OggFLAC_API FLAC__bool OggFLAC__seekable_stream_decoder_process_until_end_of_metadata(OggFLAC__SeekableStreamDecoder *decoder);

/** This is inherited from FLAC__SeekableStreamDecoder; see
 *  FLAC__seekable_stream_decoder_process_until_end_of_stream().
 *
 * \param  decoder  A decoder instance.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    See above.
 */
OggFLAC_API FLAC__bool OggFLAC__seekable_stream_decoder_process_until_end_of_stream(OggFLAC__SeekableStreamDecoder *decoder);

/** This is inherited from FLAC__SeekableStreamDecoder; see
 *  FLAC__seekable_stream_decoder_seek_absolute().
 *
 * \param  decoder  A decoder instance.
 * \param  sample   The target sample number to seek to.
 * \assert
 *    \code decoder != NULL \endcode
 * \retval FLAC__bool
 *    \c true if successful, else \c false.
 */
OggFLAC_API FLAC__bool OggFLAC__seekable_stream_decoder_seek_absolute(OggFLAC__SeekableStreamDecoder *decoder, FLAC__uint64 sample);

/* \} */

#ifdef __cplusplus
}
#endif

#endif
